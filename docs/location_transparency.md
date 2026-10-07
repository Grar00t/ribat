# Location transparency

## Call compilation

A Niyah distributed-call-capable function has a `FUN` descriptor object: module ID, function ordinal, ABI hash, canonical parameter/result type hashes, required capability class hashes, determinism class and resource envelope. A direct local call remains a normal System V ABI call. A function imported as fabric-callable is compiled as:

```text
ribat_call(fun_id[32], arg_graph, cap_vector, flags) -> Future<Result>
```

The call site contains only `fun_id`, type metadata and explicit capabilities; it contains no host name, node ID, IP address or placement hint. The compiler emits a canonical serializer for each static value type. Pointer-bearing values are rejected across the fabric boundary; they must be converted to immutable data objects or capabilities.

## Resolution

| Option | Tradeoff |
|---|---|
| Gossip-derived nearest/least-loaded choice | Reacts to measured state, but two nodes can make divergent placement choices and load reports are untrusted unless signed and bounded |
| Deterministic rendezvous ranking over verified membership | Every resolver derives the same candidate order from function ID and membership epoch; membership churn changes placement and does not itself measure load |

**Pick: deterministic rendezvous ranking.** For every live, eligible member record in a signed membership epoch, compute `SHA256("RIBAT/PLACE/1\0" || fun_id || node_id || epoch_hash)`. Lowest lexicographic digest wins; scheduler tries candidates in rank order. Eligibility requires manifest ABI support, declared resource class, verified function availability or replicability, and capability locality permission. Signed bounded load advisories can only skip a candidate, never alter the ranking order.

## Wire format

All multi-byte integers are big-endian. Frame header is 48 bytes before encrypted/authenticated payload:

```text
0   u32 magic = 0x52425431       // RBT1
4   u8 version = 1
5   u8 type                       // CALL=0x20, REPLY=0x21, CANCEL=0x22
6   u16 flags
8   u64 session_id
16  u64 sequence
24  u8 src_node_id[16]            // first 16 bytes of SHA256(identity public key)
40  u32 payload_len
44  u32 header_crc32
48  payload
```

`CALL` payload: `attempt_id[16] | fun_id[32] | manifest_id[32] | arg_id[32] | cap_count:u16 | caps[cap_count*48] | deadline_logical:u64 | flags:u32`. `REPLY`: `attempt_id[16] | status:u16 | result_id[32] | error_id[32]`. Payload encryption/authentication is ChaCha20-Poly1305 under a session key created by the authenticated handshake; header through byte 43 is associated data. Before M2 crypto is implemented, transport test frames have `RBT1` format but are explicitly rejected by production acceptance tests.

## Failures

Calls are identified by caller-generated 128-bit `attempt_id`. Receiver writes a durable `LOG` acceptance record before executing any effect-capable function. Retry of the same `(caller_node_id, attempt_id)` returns the cached terminal reply if present; otherwise it returns `IN_PROGRESS`. This gives at-most-once *acceptance* and at-least-once *delivery attempts*. Exactly-once execution is not offered across node death or partition. Pure/idempotent functions may be retried on the next ranked candidate. Effect-capable functions require an explicit effect capability and durable application-level idempotency key.
