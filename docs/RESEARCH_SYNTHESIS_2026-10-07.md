# Research Synthesis — 2026-10-07

## Papers read for this repository

- **IPFS-DID: A Lightweight Decentralized Identity Protocol Based on Content-Addressed Storage** — ISPDS 2025, DOI 10.1109/ISPDS67367.2025.11391175. Indexed primary metadata confirms the paper; the accessible record describes IPFS content addressing plus DHT/IPNS-style resolution. Full implementation detail beyond the indexed record remains unverified here.
- **Content-Addressing: 2025 In Review** — IPFS Foundation, 15 Jan 2026. Verified mechanisms relevant here are modular/simple CID-family specifications, DASL interoperability work, and constrained subsets such as DRISL. The supplied claim that this review documents BLAKE3 adoption is not present in the official indexed article and is therefore unverified.
- **Enabling Cloud-Scale Distributed Capabilities** — HCDS 2025, DOI 10.1145/3723851.3723854. Mechanism: guarded capability sets, a smaller guard derivation tree, owner-based sharding, 64-bit owner/guard versions, selective recursive revocation, and delegation without a guard-tree update.
- **FiDe: Reliable and Fast Crash Failure Detection to Boost Datacenter Coordination** — USENIX ATC 2025. Mechanism: dedicated failure-detector processes combined with OS instrumentation and traffic engineering to bound packet-processing delay; the sub-30us result depends on that deployment environment.
- **uKharon: A Membership Service for Microsecond Applications** — USENIX ATC 2022, not a 2024–2026 paper. Mechanism: multi-level failure detection and membership changes using one-sided RDMA/CAS.
- **On timing side channels in "constant-time implementations"** — Journal of Cryptographic Engineering 2026, DOI 10.1007/s13389-026-00394-y. Demonstrates source-level constant-time X25519 compiling to data-dependent control flow on Xtensa because of ISA/compiler idioms.
- **A Compact SHA256 Accelerator in 22nm for Energy Bounded Use-Cases with 8.2GHash/J** — ISCAS 2025, DOI 10.1109/ISCAS56072.2025.11043597. Hardware-only 22nm CMOS SHA256 accelerator.

## Changes committed

- `93e2d978088ceb2b8bb1327b7c8de923d07cf451` — `ribat: add versioned capability guards (cloud-scale-cap-2025)`
  - Adds owner and guard 64-bit version checks.
  - Adds guarded capability slots with parent links and recursive subtree revocation.
  - Reusing a revoked guard increments its version so stale capabilities stay invalid.
  - Delegation is local and only attenuates rights; it does not mutate the guard tree.
  - Owner restart increments owner version and invalidates prior capabilities.
  - Adds regression tests for delegation, attenuation, selective revocation, descendant revocation, slot reuse, stale-capability rejection, owner restart, and version exhaustion.

## Papers read but rejected

- **IPFS-DID** — rejected for code now: its distinguishing mechanism requires DHT/IPNS-style mutable name resolution; Ribat has no transport/discovery implementation on which to implement that mechanism faithfully.
- **Content-Addressing: 2025 In Review** — no code change: Ribat already has an internal content-derived object ID; the review's verified modularity/interoperability direction does not expose a current correctness defect. BLAKE3 was not added because the supplied attribution to this review is unverified.
- **FiDe** — rejected for code now: its reliability/latency result depends on dedicated detector execution, OS instrumentation, and network traffic engineering; a generic timeout state machine would not implement the paper's mechanism.
- **uKharon** — rejected: publication is 2022 and the mechanism requires one-sided RDMA/CAS plus RDMA-capable transport/hardware not present in Ribat.
- **Constant-time side-channel paper** — rejected for code now: the demonstrated failure is Xtensa/X25519-specific; Ribat currently contains SHA256/object-ID code, not X25519, and no observed source-level secret-dependent branch justifies an assembly rewrite.
- **SHA256 accelerator** — rejected: reported gains are from ASIC datapath/pipeline/latch structures and cannot be reproduced as a C/x86-64 software change.
- **Morello-Cerise, watchpoint IPC, HSEB, nCPU, Onramp** — rejected for this repository: hardware/OS/compiler mechanisms do not map to Ribat's current distributed-fabric code.

## Papers requiring operator decision

- None.
