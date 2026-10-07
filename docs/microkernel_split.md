# Hadeed to Ribat microkernel split

## IPC primitive

| Option | Tradeoff |
|---|---|
| Shared-memory queues with lock-free rings | High bulk throughput, but ownership, cache coherence, cancellation and memory ordering leak into every server protocol; a compromised server can corrupt shared metadata unless mappings are extremely constrained |
| Synchronous capability-addressed IPC with optional grant pages | One kernel transfer path gives explicit sender/receiver identity, bounded copy semantics and cancellation points; bulk data needs an explicit page-grant extension |

**Pick: synchronous endpoint IPC plus immutable page grants.** `sys_ipc_call(endpoint_cap, send_desc, recv_desc)` blocks the caller until reply, cancellation, timeout or peer death. The kernel validates endpoint and grant capabilities, copies up to 256 bytes of inline metadata, and maps page grants read-only or copy-on-write into the receiver. Replies carry an attempt status. Notifications are separate nonblocking counters for IRQ and timer events. The object store transfers verified immutable pages by grant; it does not move bulk objects through copied inline IPC.

## Component split

| Component | Current location in Hadeed plan | Target location in Ribat | Ring | IPC interface |
|---|---|---|---:|---|
| Boot, page-table bootstrap | boot/stage1 | microkernel bootstrap | 0 | none before kernel IPC exists |
| Frame allocator | `kernel/e820.c`, buddy plan | microkernel | 0 | `FRAME_ALLOC`, `FRAME_FREE` internal kernel API |
| Address-space/page mapping | `kernel/vm.c` plan | microkernel | 0 | `AS_CREATE`, `MAP_GRANT`, `UNMAP` |
| Thread scheduler/context switch | not yet implemented | microkernel | 0 | `THREAD_CREATE`, `YIELD`, `IPC_CALL` |
| GDT/IDT/TSS/interrupt entry | Hadeed kernel plan | microkernel | 0 | IRQ notification endpoint |
| APIC/PIC/PIT/HPET routing | Hadeed kernel plan | microkernel | 0 | `IRQ_BIND`, `IRQ_ACK`, `TIMER_ARM` |
| Capability validation | planned syscall boundary | microkernel reference monitor | 0 | validates opaque kernel capability slots |
| VGA console | `kernel/vga.c` plan | console server | 3 | `CONSOLE_WRITE`, input stream endpoint |
| Keyboard driver | `kernel/keyboard.c` plan | input driver server | 3 | IRQ notification → `KEY_EVENT` stream |
| NIC driver and DMA rings | absent | NIC driver server | 3 | DMA-frame grants + `RX_FRAME`, `TX_FRAME` |
| Block driver | absent | block driver server | 3 | `BLOCK_READ`, `BLOCK_WRITE` page grants |
| Filesystem | absent | object-store server only; no path filesystem in initial fabric | 3 | `GET`, `PUT`, `PIN`, `REPLICATE` |
| Network protocol | absent | transport and membership servers | 3 | `SEND_FRAME`, `RECV_FRAME`, `MEMBERSHIP` |
| Niyah loader/runtime | future Hadeed user process | runtime server | 3 | `CALL`, `REPLY`, `IMPORT_OBJECT` |
| Shell | `kernel/shell.c` plan | diagnostic shell server | 3 | console + capability-mediated service calls |

Ring 0 retains only mechanisms: protection domains, virtual memory, scheduling, IPC, capability-slot enforcement, IRQ delivery, timers, DMA/IOMMU mediation, and panic/exception containment. Policy and all replaceable drivers/services move to ring 3.
