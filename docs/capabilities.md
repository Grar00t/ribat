# Capability model

## Representation

A kernel capability is an unforgeable slot index plus generation in a process capability table; it names an endpoint, frame, address space, IRQ, DMA domain or sealed object. It never crosses a node as raw slot bits.

A portable Ribat capability is a 48-byte signed/sealed descriptor:

```text
0   issuer_node_id[16]
16  cap_id[16]                 // random, issuer-local table key
32  rights:u32                 // READ=1 WRITE=2 EXEC=4 DELEGATE=8 REVOKE=16
36  resource_kind:u16          // OBJECT=1, SERVICE=2, BLOCK_RANGE=3, FUNCTION=4
38  caveat_count:u16           // max 4
40  caveat_hash[8]             // compact binding; full caveats object-addressed
48  // descriptor is followed by capability object ID and authenticated session framing
```

The 48-byte wire core is accompanied by `CAP` object ID (32 bytes), issuer signature (64 bytes) and, when delegated, parent capability ID. `CAP` canonical object includes subject public-key hash, resource ID/range, rights, not-before/not-after logical epoch bounds, quota, delegation depth, caveat object ID, revocation epoch and parent ID. Attenuation can only reduce rights, quota, lifetime and delegation depth; a receiver verifies that invariant over the chain.

## Transmission

Capabilities ride in encrypted/authenticated `CALL` payloads. The receiver verifies session peer identity, issuer signature, chain, object hashes, caveats and revocation epoch before materializing a local kernel endpoint/frame capability. Bare object IDs do not grant read access: `GET` requires an object-read capability or an explicitly public-object policy encoded in the object manifest.

## Revocation

| Option | Tradeoff |
|---|---|
| Pure expiry/reissue | No global revocation lookup, but leaked long-lived capability remains usable until expiry |
| Signed issuer revocation epochs plus short leases | Immediate logical revocation after propagated epoch, but receiver needs issuer/revocation-object availability and partitioned nodes may have stale knowledge |

**Pick: signed revocation epochs plus short leases.** Every capability contains issuer-local `revocation_epoch` and an expiry expressed in issuer membership epochs. Issuer publishes immutable signed `REV` objects with monotonically increasing epoch and a compressed revoked-capability set. Receivers cache the highest verified epoch and deny older capabilities. High-risk rights use short leases and require online issuer confirmation. During partition, an unavailable revocation authority causes high-risk remote operations to fail closed; low-risk read capabilities remain valid only until their short lease expires. A completed operation cannot be rolled back by later revocation.

## Example: remote file/object access

`render_report()` has an attenuated capability for object `r1:...ab`, rights `READ`, maximum 1 MiB, non-delegable, expiry epoch 410. Its Niyah call serializer includes the portable capability descriptor and CAP object ID. The resolver selects remote node B. B verifies A's session identity, CAP signature chain and the resource/rights/quota caveats. B materializes a local read endpoint to the object-store server, reads at most 1 MiB, returns a `DAT` object ID to A, and charges the capability quota. `render_report()` cannot enumerate storage, write the object, use the descriptor to reach another object, or delegate it.
