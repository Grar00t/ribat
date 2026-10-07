# Failure model

## Offered guarantees

| Property | Offered guarantee | Explicit non-guarantee |
|---|---|---|
| Object integrity | A returned object is accepted only if bytes hash to its full claimed object ID; executable objects additionally require a valid trusted manifest signature | Hash verification does not prove availability, freshness of mutable metadata, or authorization by itself |
| Immutable storage | Locally committed object is crash-recoverable after journal replay; replication factor `R` is tracked as verified independent acknowledgements | Durability is not guaranteed if fewer than R verified replicas survive or if every replica's media corrupts |
| Calls | At-most-once acceptance per durable `(caller_node_id, attempt_id)` record; at-least-once delivery attempts | Exactly-once execution/result across crash or partition is not offered |
| Pure work | Retry on ranked alternatives after timeout/failure; equal object inputs must yield the declared deterministic output object | Runtime does not prove that arbitrary native code is pure or deterministic |
| Effect-capable work | Requires explicit effect capability and durable application idempotency key | No distributed transaction or automatic rollback of external effects |
| Membership | Signed records and quorum of pinned authorities determine eligibility for placement | No universal membership truth during partition; a node cannot distinguish dead from unreachable by timeout alone |

Failure detector state is `ALIVE`, `SUSPECT`, `DEAD_LOCAL`. It uses authenticated heartbeat sequence progression and local monotonic timers. `DEAD_LOCAL` only drives retry/placement locally; it is not a signed global fact. Membership removal needs the configured threshold of pinned membership-authority signatures or an explicit local operator action.

## Node death mid-call

1. Caller writes local `LOG` attempt record before sending `CALL`.
2. Receiver writes durable acceptance record keyed by `(caller_node_id, attempt_id)` before invoking effect-capable work.
3. Caller misses authenticated progress/terminal reply by deadline, transitions peer to `SUSPECT`, then tries next rendezvous candidate only if function descriptor is pure/idempotent.
4. If old receiver returns later, caller accepts only the terminal result whose `attempt_id`, input IDs, capability epoch and manifest ID match the outstanding attempt.
5. For effect-capable call, caller reports `UNKNOWN_OUTCOME` after timeout rather than retrying automatically. A query capability may inspect the durable attempt record to resolve outcome.

## Network partition

1. Each side continues local reads of already verified immutable objects and local execution not requiring unavailable capabilities.
2. Resolver uses last verified membership epoch but marks remote candidates unreachable after detector expiry.
3. New membership/revocation records require their configured signature threshold; neither side treats unilateral timeouts as a global eviction.
4. Replicated mutable metadata has single-writer ownership capability. If owner is unavailable, writes fail; no multi-writer merge protocol is implied.
5. On reconnection, nodes exchange signed membership/revocation records, object inventories by prefix, and durable attempt logs. Objects deduplicate by hash; attempt records reconcile by `(caller,attempt_id)` and terminal-state hash.

Availability is intentionally sacrificed for capability revocation and single-writer mutable state. Immutable reads and pure computation remain available when the requisite objects and valid capabilities are local.

## Disk corruption

1. On read, recompute object ID hash before returning bytes.
2. Mismatch quarantines file/pack extent, records `CORRUPT(object_id, medium, offset)` in journal and removes local replica acknowledgement.
3. Query replica inventory for verified peers; request exact ID, stream bytes to temp, recompute hash, atomically reinstall and journal repair.
4. If no verified replica exists, return `OBJECT_LOST`; do not fabricate contents from metadata or a partial pack.
5. Pack corruption triggers index validation and per-entry object-hash validation. Good entries are repacked into a new generation; bad IDs follow repair/lost handling.

Metadata is recoverable only when its signed immutable objects have a surviving verified replica. The journal is an optimization for local recovery, not a substitute for replicated object content.
