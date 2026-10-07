# Content addressing

## Hash choice

| Option | Tradeoff |
|---|---|
| SHA-256 | 32-byte digest, compact scalar implementation, standardized by FIPS 180-4 and widely specified; lacks BLAKE3 tree-mode throughput for very large parallel object ingestion |
| BLAKE3 | Tree hashing, parallel-friendly and supports keyed/derive-key modes; separate specification and more complex implementation/test surface for a no-dependency bare-metal base |

**Pick: SHA-256.** Ribat needs a small, independently auditable scalar implementation before it needs high-throughput parallel hashing. SHA-256 has a primary normative specification in FIPS 180-4; it will be implemented and tested in-tree with published known-answer vectors. BLAKE3's tree parallelism is useful for bulk objects but is not worth widening the cryptographic trusted base at bootstrap. No custom hash is admissible: a custom construction has no cryptographic basis or independent test corpus. FIPS 180-4 specifies SHA-256 among its secure hash functions. [web:31][web:32]

## Object ID

```text
object_id = SHA256(
    "RIBAT/OBJ/1" || 0x00 ||
    type[3] || canonical_version:u16be ||
    payload_length:u64be || canonical_payload
)
```

Object IDs are exactly 32 raw bytes. Text form is lowercase hexadecimal, 64 bytes, prefixed by `r1:` only in human-facing diagnostics. Hash preimage includes type/version/length. Canonical payload encodings forbid alternate integer lengths, unordered maps, NaN values and pointer addresses. A received object is stored only when its reconstructed preimage hashes to the claimed ID.

## Storage

```text
store/
  objects/aa/bb/<64-hex-id>     immutable canonical bytes
  packs/<pack-id>               append-only pack of small immutable objects
  index/<pack-id>.idx           sorted (object_id, offset, length, crc32) entries
  refs/<pin-id>                 mutable local pin metadata, never an object address
  journal/<seq>                 crash-recovery transactions
```

Loose objects are installed by: write `tmp/<nonce>`, flush content, hash, atomically link/rename to the object path, flush containing directory, then append a journal completion record. Existing destination ID means deduplication success only after verifying existing contents. Pack compaction writes a new immutable pack and index, verifies every entry hash, publishes a journaled generation pointer, then reclaims unpinned old packs after a grace epoch.

## Deduplication

Deduplication key is full object ID. Equal canonical bytes have equal IDs; equal IDs must have equal verified canonical bytes. Local store dedup links/reuses a verified extant object; transfer dedup sends `HAVE` bitmaps or sorted prefix ranges before byte transfer. Replicas independently hash bytes; no node trusts a peer's claimed inventory alone. Mutable local placement, pins and replication preferences live outside the hash addressing layer.

## Executable manifests

`MAN` canonical payload:

```text
magic[4]="RBM1" | version:u16 | flags:u16 | module_id[32] | entry_fun_id[32]
abi_hash[32] | compiler_id[32] | required_caps_hash[32] | limits[32]
issuer_pubkey[32] | signature[64]
```

Signature preimage is `SHA256("RIBAT/MAN/1\0" || manifest fields preceding signature)`. The issuer signs that 32-byte digest with Ed25519. Receiving nodes: parse bounded canonical fields; recompute manifest hash; verify Ed25519 signature against its local trust policy; fetch module/compiler/function objects; recompute every object ID; compare ABI hash to supported runtime; only then map/load the module. Ed25519 and X25519 code are in-tree M2 work, not supplied by a dependency.
