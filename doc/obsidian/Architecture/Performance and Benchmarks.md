---
title: Performance and Benchmarks
tags:
  - coroute
  - performance
  - benchmark
  - comparison
---

# Performance and Benchmarks

> [!abstract]
> Coroute v2 is designed for high-performance cross-platform execution. This note summarizes the benchmarking strategy and comparative performance against industry-standard frameworks.

## Benchmarking Suite

Coroute includes a comprehensive automated benchmarking suite located in `[[benchmark/]]`. It compares Coroute against:

- **C++**: Drogon, Crow, Oat++
- **Node.js**: Express.js
- **Python**: Flask

### Metrics Collected

- **Throughput**: Requests per second (higher is better)
- **Latency**: Response time in microseconds (lower is better)
- **Memory**: Peak and average RSS (lower is better)
- **Stressed Network**: Performance under 50ms delay and 1% packet loss

## Comparative Results (Representative)

Values below represent typical performance observed on Linux (Ubuntu 22.04) under a "High Connections" (512) load.

### Throughput (req/s) - Normal Network

| Framework | Requests per second |
| :--- | :--- |
| **Coroute v2** | **125,000** |
| Drogon | 118,000 |
| Oat++ | 92,000 |
| Express.js | 28,000 |
| Flask | 5,000 |

### Peak Memory Usage (KB)

| Framework | Peak Memory (KB) |
| :--- | :--- |
| **Coroute v2** | **45,200** |
| Oat++ | 48,000 |
| Drogon | 52,100 |
| Express.js | 85,000 |
| Flask | 110,000 |

## DFA Routing Performance

Coroute's DFA-based router provides $O(N)$ match time relative to path length, making it immune to the "route inflation" that slows down standard regex-based routers.

| Number of Routes | Coroute (DFA) | `std::regex` | Improvement |
| :--- | :--- | :--- | :--- |
| 10 | 0.12 $\mu$s | 0.45 $\mu$s | 3.75x |
| 50 | 0.15 $\mu$s | 2.34 $\mu$s | 15.6x |
| 100 | 0.18 $\mu$s | 4.89 $\mu$s | 27.2x |
| 500 | 0.23 $\mu$s | 24.56 $\mu$s | 106.8x |

## Cross-Platform Efficiency

Coroute achieves near-native performance on both Windows and Linux by utilizing platform-specific asynchronous primitives.

### Throughput (30s Load Test)
| Platform | Connections | Total Requests |
| :--- | :--- | :--- |
| **Linux (io_uring)** | 1024 | **4,715,446** |
| **Windows (IOCP)** | 1024 | **1,030,000** |

Note: Linux `io_uring` typically outperforms Windows `IOCP` in raw throughput due to lower system call overhead and more efficient kernel scheduling for network I/O.

## Key Architectural Advantages

### 1. Zero-Copy Routing
Coroute minimizes memory allocations during request matching and header parsing. The `IoContext` backends (`io_uring`, `kqueue`, `IOCP`) allow for efficient buffer management directly from the OS.

### 2. Coroutine-Based Concurrency
Unlike callback-based frameworks (Express, Drogon) or thread-per-request models (Flask), Coroute uses C++20 coroutines. This provides:
- **Low Context Switch Overhead**: Coroutines are significantly lighter than OS threads.
- **Deep Call Stacks**: Asynchronous logic looks and feels synchronous without blocking the event loop.

### 3. Native OS Backends
By using `io_uring` on Linux and `IOCP` on Windows, Coroute maximizes hardware utilization:
- **Batching Syscalls**: Reducing the frequency of kernel-user transitions.
- **Multi-Accept**: Scaling connection handling across multiple threads without lock contention.

## Running Benchmarks

To reproduce these results, use the provided scripts:

```bash
# On Linux
cd benchmark
./run-single-benchmark.sh

# On Windows (PowerShell)
cd benchmark
.\run-single-benchmark.ps1
```

Detailed reports are generated in `benchmark/results/`.
