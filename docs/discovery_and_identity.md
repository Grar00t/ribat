# Discovery and identity

## Identity format

Each node generates 32 bytes from a kernel entropy source and derives an Ed25519 keypair. The private key is stored only in a sealed local key object; the public key is 32 bytes. `node_id = SHA256("RIBAT/NODE/1\0" || ed25519_public_key)[0..15]`. The full public key, not the 16-byte node ID, is the identity authority. Node ID is routing/display shorthand and collision detection always compares full keys.

There is no CA, DNS name or implicit trust. A trust store contains immutable public-key pins and signed delegation records. First enrollment is a physical or console-mediated comparison of a 32-byte fingerprint; a peer remains `UNTRUSTED` until its key is explicitly pinned or reached through a valid pinned delegation chain.

## Handshake choice

| Option | Tradeoff |
|---|---|
| Static-key authenticated key exchange without ephemeral keys | Small message/state machine, but compromise of static private key exposes recorded session traffic |
| Signed ephemeral X25519 handshake | Adds ephemeral key generation, transcript and signature verification; recorded traffic remains protected after later static-key compromise |

**Pick: signed ephemeral X25519 handshake.** Static Ed25519 identity keys authenticate the transcript. Each session creates a fresh X25519 keypair, signs the transcript hash, derives a session secret with X25519 and uses HKDF-SHA-256 to derive directional ChaCha20-Poly1305 keys. This is an explicit in-tree protocol rather than an imported framework. The Noise Protocol Framework is a primary description of handshake patterns but is not a protocol dependency. [web:40]

## Ethernet discovery and handshake bytes

Ribat starts on a raw Ethernet broadcast domain. EtherType `0x88B5` is an experimental/local-use value for development only; deployment requires an assigned EtherType or a link-local encapsulation defined by the project. There is no IP dependency in this path.

```text
Ethernet: dst[6] | src[6] | ethertype:u16be = 0x88B5

HELLO payload, 128 bytes:
0    magic[4] = "RBH1"
4    version:u16 = 1
6    flags:u16
8    epoch:u64                    // monotonically stored local boot/session epoch
16   nonce[32]                    // random per HELLO burst
48   ed25519_pub[32]
80   x25519_ephemeral_pub[32]
112  capability_advert_hash[16]   // SHA256(canonical public service advert)[0..15]
128  sha256[32]                   // actually frame payload is 160 bytes including this field
```

Correction to the fixed-size label: HELLO is **160 bytes**, not 128: fields through offset 127 total 128 bytes, followed by the 32-byte `hello_hash` at offset 128. `hello_hash = SHA256("RIBAT/HELLO/1\0" || bytes[0..127])`. The bootstrap source constructs and verifies this exact 160-byte record.

Authenticated exchange after HELLO:

```text
HELLO_I broadcast/unicast
HELLO_R unicast
AUTH_I: "RBA1" | responder_hello_hash[32] | initiator_hello_hash[32] |
        initiator_static_ed25519_pub[32] | initiator_x25519_pub[32] |
        sig_i[64]
AUTH_R: "RBA2" | initiator_hello_hash[32] | responder_hello_hash[32] |
        responder_static_ed25519_pub[32] | responder_x25519_pub[32] |
        sig_r[64]
```

`sig_i` signs `SHA256("RIBAT/AUTH-I/1\0" || HELLO_I || HELLO_R || AUTH_I fields before signature)`. `sig_r` similarly signs both HELLOs and AUTH_I. Session secret is `X25519(eph_i_private, eph_r_public)`. Derivation uses extract/salt equal to the ordered HELLO hashes and expands labels `ribat i->r` and `ribat r->i`. Replay cache key is `(peer static public key, epoch, nonce)` and records expiration under local monotonic time.

## Attack analysis

| Attack | Effect without control | Required control |
|---|---|---|
| Sybil | An attacker can generate identities and flood HELLOs or dominate an unpinned discovery list | Only pinned keys/delegated member records enter eligible membership; rate-limit and bound untrusted HELLO cache; no identity earns placement by count alone |
| MITM | Attacker can relay or substitute HELLO frames | AUTH signatures bind both HELLO hashes and ephemeral keys; pinned static public-key comparison rejects substituted identity; session keys have forward secrecy from ephemeral X25519 |
| Replay | Attacker retransmits old HELLO/AUTH/CALL frames | HELLO `(epoch,nonce)` replay cache, session sequence numbers, authenticated headers, monotonic receive windows, and attempt IDs reject duplicates |

A local broadcast domain has no confidentiality before the authenticated session is established. HELLO records intentionally reveal public keys and service-advert hashes. The peer identity is not authenticated until the AUTH signature verifies against a pinned/delegated trusted key.
