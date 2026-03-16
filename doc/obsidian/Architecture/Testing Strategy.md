---
title: Testing Strategy
tags:
  - coroute
  - architecture
  - testing
  - benchmarks
---

# Testing Strategy

> [!abstract]
> Coroute employs a multi-layered testing strategy to ensure reliability, performance, and cross-platform consistency. This includes unit tests (Catch2), integration tests, and performance benchmarks (wrk/ab).

## 1. Unit Testing
Coroute uses the **Catch2** framework for component-level verification.

- **Router Tests**: Verify DFA matching, parameter extraction order, and edge cases (empty paths, special characters).
- **Protocol Tests**: 
    - **HTTP/1.1**: Parsing logic, header limits, and chunky encoding.
    - **HTTP/2**: HPACK header compression, frame serialization, and stream state transitions.
    - **WebSocket**: Handshake key calculation and frame masking.
- **Utility Tests**: `FromString<T>` conversions, buffer pooling, and `Task<T>` execution lifecycle.

## 2. Integration Testing
Integration tests evaluate the interaction between the network backends and the application layer.

- **Accept Loop**: Verifying successful connection handover between threads.
- **Middleware Chain**: Ensuring correct execution order and short-circuiting logic.
- **Lifecycle Tests**: Validating graceful shutdown and resource cleanup (RAII).

## 3. Performance Benchmarking

### Methodology
- **Tooling**: `wrk` (Linux), `winrk` (Windows), and `Apache Bench (ab)`.
- **Environment**: Identical hardware with dual-boot (Ubuntu/Windows) to compare `io_uring` vs `IOCP` under identical conditions.
- **Metrics**: 
    - Throughput (req/s)
    - Latency (p50, p90, p99)
    - Memory Usage (RSS/Working Set)

### Comparative Analysis
Coroute is regularly benchmarked against:
- **C++**: Drogon, Oat++, Crow
- **Node.js**: Express.js (Cluster mode)
- **Python**: Flask (Waitress)

## 4. Network Resilience (Hostile Network Testing)
To simulate real-world internet conditions, Coroute is tested under simulated network degradation using:
- **Linux**: `tc qdisc` (Traffic Control)
- **Windows**: `clumsy` (WinDivert-based)

### Simulated Conditions:
- **Latency**: 50ms base delay + jitter.
- **Packet Loss**: 1% consistent drop rate.

### Findings:
- Coroutine scheduling remains stable without starvation.
- Latency tracks perfectly with RTT (50ms in + 50ms out).
- Memory usage scales linearly without leaks even during high timeout event rates.

## 5. Build-Time Verification
Tests are integrated into the `CMake` build system:
```bash
mkdir build && cd build
cmake .. -DCOROUTE_ENABLE_TESTS=ON
make
# Run tests
./tests/coroute_tests
```

## Related Notes
- [[Architecture/Performance and Benchmarks]]
- [[Architecture/Server Runtime and OS Backends]]
