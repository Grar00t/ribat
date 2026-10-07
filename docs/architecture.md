# Ribat architecture

## Scope boundary

Ribat is a new ABI and distributed-object fabric on Hadeed. It is not a compatibility layer for Linux, POSIX, IP, or existing process formats. A node is an independently booted Hadeed machine. The fabric has no required permanent coordinator; membership, placement and object replication are derived from signed records and deterministic rules.

The current Hadeed repository is an M1 boot sector plus M2 kernel skeleton, not an existing monolith to mechanically refactor. This document specifies the target microkernel split. No code in this repository claims that the split is implemented.

## Layered map

```text
+--------------------------------------------------------------------------+
| User code                                                                |
|  Niyah modules; pure function calls; capability handles; no node address |
+----------------------------- Niyah ABI ----------------------------------+
| Niyah runtime server (ring 3)                                            |
|  module loader | call stubs | serializer | future/task table | GC policy |
+----------------------------- IPC: CALL/REPLY -----------------------------+
| Ribat fabric servers (ring 3, separate protection domains)               |
|  resolver    object-store   replication   scheduler   membership          |
|  manifest    capability     transport     NIC driver      block driver    |
|       |          |              |              |               |          |
+-------+----------+--------------+--------------+---------------+----------+
| Hadeed microkernel (ring 0)                                              |
|  entry/exit | address spaces | threads | IPC | IRQ routing | timers       |
|  frame allocator | page mapper | capability validation | IOMMU policy     |
+-------------+------------------+------------------+----------------------+
| x86-64 hardware                                                        |
| CPU | APIC/IOAPIC | PIT/HPET/TSC | NIC DMA | block DMA | VGA/framebuffer |
+--------------------------------------------------------------------------+
```

## Interfaces

| Boundary | Interface | Contract |
|---|---|---|
| Niyah code → runtime | Niyah call ABI | function object hash, typed value graph, capability vector, call flags |
| Runtime → resolver | `RESOLVE` IPC | function hash + resource/capability requirements → ordered candidate node set |
| Runtime → object store | `GET_PUT` IPC | immutable object hash → verified bytes or explicit absence |
| Runtime → scheduler | `SUBMIT` IPC | call descriptor → attempt ID and result future |
| Fabric services → transport | `SEND_FRAME` IPC | authenticated frame to identity or broadcast group |
| Driver → microkernel | IRQ endpoint | DMA completion metadata; driver owns descriptor interpretation |
| Microkernel → server | synchronous IPC / notification | bounded message transfer or one-bit/counting notification |

No host backend owns emulated or fabric time. The microkernel timer service supplies monotonic local ticks. Distributed timestamps are logical counters within signed records, not claims about wall-clock truth.

## Object classes

All content objects have immutable, hash-addressed encodings: `MOD` Niyah module, `FUN` function descriptor, `DAT` typed data graph, `MAN` signed executable manifest, `CAP` sealed capability delegation, `MEM` signed membership record, `CHK` replicated state checkpoint, `LOG` append-only attempt log segment. The three-byte ASCII domain tag is inside the hashed preimage; equal payload bytes in different object classes cannot collide at the object-ID layer.

## Execution path

1. A Niyah call site emits a function object ID and a typed argument graph ID or inline canonical value.
2. Runtime validates local capabilities, serializes the argument graph canonically, then sends `RESOLVE`.
3. Resolver intersects function manifest requirements, capability locality, membership liveness and deterministic rendezvous scores.
4. Scheduler sends an authenticated `CALL` frame to the chosen node or executes through the local runtime server.
5. Receiver verifies frame authentication, manifest signature, module/function object hashes, capability attenuation chain and argument hashes before scheduling.
6. Result data is immutable content addressed; `REPLY` contains result ID or a deterministic error class.

No step accepts an opaque executable byte stream as runnable code. A byte stream first becomes an object, is hash-verified, then is accepted only through a verified signed manifest.
