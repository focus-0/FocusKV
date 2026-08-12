# FocusKV — Distributed LSM Key-Value Storage Engine

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)]()
[![Build System](https://img.shields.io/badge/build_system-Bazel-orange.svg)]()
[![Architecture](https://img.shields.io/badge/architecture-LSM--Tree%20%7C%20Raft-red.svg)]()

**FocusKV** is a persistent, distributed key-value storage engine built from scratch in modern C++ (C++17/C++20). The system is engineered around a Log-Structured Merge (LSM) tree architecture optimized for high-velocity write workloads, crash-resilient storage, POSIX socket networking, and Raft distributed consensus.

---

## 🏗️ System Architecture

```
                               +-----------------------------+
                               |     Client Application      |
                               +--------------+--------------+
                                              |
                                    TCP Wire Protocol (SET/GET/DEL/TRACE)
                                              v
+-----------------------------------------------------------------------------------+
| Subproject 2: Network Reactor (POSIX TCP Socket Server)                            |
+-----------------------------------------------------------------------------------+
                                              |
                                     Quorum Replication
                                              v
+-----------------------------------------------------------------------------------+
| Subproject 3: Distributed Consensus Layer (Raft Protocol)                          |
| - Leader Election  |  Log Replication  |  Majority Quorum  |  Node Failover       |
+-----------------------------------------------------------------------------------+
                                              |
                                      Apply State Transition
                                              v
+-----------------------------------------------------------------------------------+
| Subproject 1: Core Storage Engine (LSM-Tree)                                      |
|                                                                                   |
|  +-------------------+      +-------------------+      +-----------------------+  |
|  | MemTable          | ---> | WAL (Disk)        | ---> | SSTable (Level 0..N)  |  |
|  | (SkipList+Arena)  |      | (Sequential Log)  |      | (Data Blocks + Index) |  |
|  +-------------------+      +-------------------+      +-----------------------+  |
|                                                                    ^              |
|                                                          Background Compaction    |
+-----------------------------------------------------------------------------------+
                                              |
                                      Execution Telemetry
                                              v
+-----------------------------------------------------------------------------------+
| Subproject 4: Visibility & Benchmarking                                           |
| - TraceGet (LSM Query Inspector)  |  db_bench (Throughput & Latency Suite)        |
+-----------------------------------------------------------------------------------+
```

---

## 🚀 Core Subprojects

### 1. Storage Engine Core ("The Ground Truth")
- **MemTable (In-Memory Buffer):** Lock-free SkipList backing an in-memory buffer. Uses a custom **Arena Allocator** (4KB chunk pre-allocation) to bypass `malloc`/`new` overhead, eliminate memory fragmentation, and optimize CPU cache locality. Enforces manual `Ref()`/`Unref()` lifecycle tracking for concurrent snapshot safety during flushes.
- **Write-Ahead Log (WAL):** Ensures durability (ACID) by sequentially writing write operations to disk before updating memory. Replays log records upon startup for instant crash recovery.
- **Sorted String Tables (SSTables):** Immutable on-disk files organized into 4KB data blocks with an in-memory sparse index for fast binary search block lookups.
- **Leveled Compaction:** Background multi-way merge sort worker that merges overlapping SSTables across level hierarchies ($L_0 \rightarrow L_N$), reclaiming deleted tombstones and bounding read amplification.

### 2. Network Server ("The Scale")
- Standard POSIX TCP socket server supporting concurrent client connections.
- Text-based protocol parser supporting `SET <key> <val>`, `GET <key>`, `DEL <key>`, and `TRACE <key>`.

### 3. Raft Distributed Consensus ("The Trust")
- 3-node cluster consensus machine supporting randomized election timeouts (150ms–300ms), `RequestVote`, and `AppendEntries` RPCs.
- Majority quorum commit: Writes are committed only after $\ge 2/3$ nodes acknowledge the write, ensuring data safety even if one node crashes.

### 4. Visibility & Benchmarking ("The Placement Proof")
- **`TraceGet` (LSM Query Inspector):** Exposes an internal query tracer returning per-stage execution telemetry (`MemTable` $\rightarrow$ `SSTables`) with microsecond-level latency breakdowns.
- **`db_bench`:** Performance benchmarking utility measuring sequential/random write throughput (ops/sec), $p_{50}/p_{99}$ latency, and Write Amplification Factor (WAF).

---

## 🎯 Key Design Decisions & Trade-offs

| Engineering Choice | Alternative Considered | Why FocusKV Made This Choice |
|---|---|---|
| **LSM Tree** | B+ Tree | B+ Trees require in-place updates causing random disk I/O. LSM Trees convert random writes into sequential writes in memory first, making them dramatically faster for write-heavy workloads. |
| **SkipList** | Red-Black Tree | Both achieve $O(\log N)$ search/insert time, but SkipLists avoid complex rebalancing rotations and are vastly simpler to make thread-safe without heavy mutex locks. |
| **Arena Allocator** | System `malloc`/`new` | Frequent small allocations lead to heap fragmentation and OS kernel syscall overhead. Arena pre-allocates 4KB blocks in contiguous memory. |
| **Raft Consensus** | Multi-Paxos | Paxos is notoriously complex to implement correctly. Raft provides formal safety guarantees with a understandable leader-driven consensus model. |

---

## 📁 Codebase Layout

```
FocusKV/
├── WORKSPACE
├── MODULE.bazel                     # Modern Bazel dependency management
├── BUILD                            # Root build targets (utils, arena, skiplist, storage, etc.)
├── src/
│   ├── utils/                       # Slice, Status, coding utilities
│   ├── storage/                     # MemTable, WAL, SSTable, Compactor, QueryTracer, DBImpl
│   ├── network/                     # TCP Socket Server & Command Parser
│   └── raft/                        # Raft Node State Machine & Consensus RPCs
├── tests/                           # Unit tests for all core components
└── benchmarks/                      # db_bench benchmark binary
```

---

## 🛠️ Building & Running

### Prerequisites
- Modern C++ Compiler (GCC 10+ or Clang 12+ supporting C++17)
- [Bazel](https://bazel.build/) 6.0+

### Build All Targets
```bash
bazel build //...
```

### Run Unit Tests
```bash
bazel test //tests/...
```

### Run Benchmarks
```bash
bazel run -c opt //benchmarks:db_bench
```

---

## 💻 C++ API Usage Example

```cpp
#include <iostream>
#include "src/storage/db.h"

int main() {
    focuskv::DB* db = nullptr;
    focuskv::Options options;
    options.create_if_missing = true;

    // 1. Open Database
    focuskv::Status s = focuskv::DB::Open(options, "/tmp/focuskv_demo", &db);
    if (!s.ok()) {
        std::cerr << "Failed to open DB: " << s.ToString() << std::endl;
        return 1;
    }

    // 2. Put Key-Value
    db->Put("user:100", "Ayush");

    // 3. Get Key-Value
    std::string val;
    s = db->Get("user:100", &val);
    if (s.ok()) {
        std::cout << "Retrieved: " << val << std::endl; // Output: Ayush
    }

    // 4. Trace Key Lookup Path (LSM Query Inspector)
    focuskv::ExecutionTrace trace;
    db->TraceGet("user:100", &trace);
    std::cout << "Total latency: " << trace.total_latency_us << " us" << std::endl;

    delete db;
    return 0;
}
```

---

## 📜 License

Distributed under the MIT License.
