# Cascade Context Dump: Coroute Project

**Generated:** 2025-12-08
**Purpose:** Context transfer for Linux session to execute thesis upgrades per `prompt.txt`

---

## 1. Project Overview

**Coroute** is a high-performance C++20 HTTP server library using stackless coroutines (`co_await`). The thesis demonstrates that modern C++ coroutines can achieve performance competitive with established frameworks (Drogon, Oat++, Crow) while providing a cleaner programming model.

### Key Architectural Features

- **Stackless coroutines** via `Task<T>` promise type (~23 bytes per coroutine frame)
- **Platform-specific async I/O**: IOCP (Windows), io_uring (Linux target)
- **DFA-based routing**: O(N) URL matching where N = URL length (not route count)
- **Type-safe route parameters**: Compile-time extraction of `{userId}`, `{postId}` etc.
- **Zero-copy file serving**: `TransmitFile` (Windows) / `sendfile` (Linux)

### Repository Structure

```
c:\Users\alext\Documents\GitHub\Server++\v2\
├── include/coroute/     # Headers
├── src/                 # Implementation
├── examples/            # Benchmark servers (hello_world, json_api, etc.)
├── doc/
│   ├── 1.Text/          # LaTeX chapters (Bulgarian)
│   ├── 2.Figures/       # Images
│   ├── 4.General/       # Settings.tex (main), Titlepage.tex
│   └── prompt.txt       # Linux upgrade instructions
└── build/               # CMake output
```

---

## 2. Thesis Structure (LaTeX, Bulgarian Language)

Main file: `doc/4.General/Settings.tex`
Compile: `pdflatex -interaction=nonstopmode "4.General/Settings.tex"` from `doc/` directory

### Chapters

| File | Content |
|------|---------|
| `00_abstract.tex` | Abstract (EN + BG) |
| `01_introduction.tex` | Problem statement, goals |
| `02_existing_solutions.tex` | Drogon, Crow, Oat++, Beast comparison |
| `03_theoretical_background.tex` | HTTP/1.1, HTTP/2, coroutines, IOCP, io_uring theory |
| `04_architecture.tex` | Coroute design, request lifecycle diagram |
| `05_protocols.tex` | HTTP parsing, WebSocket |
| `06_type_safe_params.tex` | DFA routing, compile-time type extraction |
| `07_example_app.tex` | Usage examples |
| `08_testing.tex` | **Benchmarks** - this is the main target for Linux updates |
| `09_conclusion.tex` | Future work |

### Known LaTeX Issue

```
! Package tikz Error: You need to load a decoration library.
```

This occurs at `04_architecture.tex:125` due to `decoration={brace, ...}`.
**Fix:** Add `\usetikzlibrary{decorations.pathreplacing}` to Settings.tex preamble.

---

## 3. Benchmark Philosophy & Current Results

### Methodology

- **Tool (Windows):** `winrk` (wrk port for Windows) - 12 threads, 30s duration
- **Tool (Linux):** Native `wrk` - same parameters
- **Concurrency levels:** 128, 256, 512, 1024 connections
- **Scenarios:** Hello World, JSON API, Static File, Parameterized Route

### Benchmark Methodology Update (2025-12-08)

**IMPORTANT:** Metrics changed from req/s to **total completed requests** for cross-platform consistency. This avoids discrepancies between `wrk` (Linux) and `winrk` (Windows) calculation methods.

### Current Windows Results (winrk, 12 threads, 30s) - Total Requests

| Framework | 128c | 256c | 512c | 1024c |
|-----------|------|------|------|-------|
| **Oat++** | 25.7M | 50.1M | 67.5M | 83.9M |
| **Coroute** | 21.6M | 32.6M | 62.8M | 75.0M |
| **Drogon** | 19.0M | 34.9M | 50.2M | 72.1M |
| Express.js | 0.58M | 0.59M | 0.59M | 0.58M |
| Flask | 0.14M | 0.28M | 0.50M | 0.15M* |
| Crow | 0.16M | 0.16M | 0.16M | 0.16M |

*Flask had 8.7% errors at 1024c

### Linux io_uring Results (wrk, 12 threads, 30s) - Total Requests

| Concurrency | Total Requests | Avg Latency | p50 | p99 |
|-------------|----------------|-------------|-----|-----|
| 128 | 3.9M | 2.63 ms | 0.49 ms | 19.44 ms |
| 256 | 4.2M | 4.22 ms | 1.25 ms | 27.34 ms |
| 512 | 4.4M | 9.75 ms | 2.84 ms | 144.47 ms |
| 1024 | 4.7M | 16.33 ms | 6.55 ms | 313.79 ms |

### Key Insight

Windows shows ~16x higher throughput than Linux on localhost. This is due to IOCP optimization for local testing, NOT a bug. Both platforms show stable scaling.

### Memory Consumption (at 1024c)

- Coroute: ~12 MB
- Drogon: ~45 MB  
- Oat++: ~38 MB
- Express.js: ~975 MB (12 cluster workers)
- Flask: ~52 MB

### DFA Routing Performance

The DFA router maintains O(N) complexity regardless of route count:

- 10 routes: 0.9 μs
- 500 routes: 0.9 μs (constant!)
- std::regex at 500 routes: 24.5 μs (27x slower)

---

## 4. Charts in 08_testing.tex

### Scalability Chart (lines ~227-263)

- **X-axis:** Linear scale, `x = concurrency/100`
- **Y-axis:** req/s in millions (1 unit = 500K)
- Plots: Oat++ (red), Drogon (green), Coroute (blue)

### DFA Complexity Chart (lines ~388-432)

- **X-axis:** Linear scale, `x = routes/50`
- **Y-axis:** Time in μs
- Plots: std::regex O(N×M) vs DFA O(N)

### Memory Chart (lines ~489+)

- Logarithmic Y-axis (due to Express.js being 80x larger than C++ frameworks)
- Bar chart comparing all frameworks at 100c, 512c, 1024c

---

## 5. Linux Migration Targets (from prompt.txt)

### Phase 1: Build Verification

```bash
cd build
rm -rf *
cmake -DCOROUTE_IO_BACKEND=io_uring -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

Verify with strace:

```bash
strace -e io_uring_enter,io_uring_submit ./server 2>&1 | head -100
```

**Expected:** `io_uring_enter` calls, NOT `epoll_wait`

### Phase 2: Hostile Network Benchmarking

```bash
# Add network degradation
sudo tc qdisc add dev lo root netem delay 50ms 10ms loss 1%

# Run benchmark
wrk -t12 -c1024 -d30s --latency http://127.0.0.1:8080/

# IMPORTANT: Remove after testing
sudo tc qdisc del dev lo root
```

### Phase 3: Profiling

```bash
sudo perf record -F 99 -g -p $(pgrep server) -- sleep 30
perf report
```

Goal: Confirm coroutine overhead < 5% of CPU time

### Phase 4: Thesis Updates (IN BULGARIAN!)

1. Replace Windows/IOCP data with Linux/io_uring results
2. Add "Мрежова устойчивост" (Network Resilience) section
3. Add "Безопасност на паметта при detached корутини" subsection
4. Update Chapter 9 with HTTP/3 and Compile-Time ORM plans

---

## 6. Important Code Locations

### Task<T> Promise Type

`include/coroute/task.hpp` - The core coroutine machinery

### io_uring Backend

`src/io_uring_context.cpp` (or similar) - Linux async I/O implementation

### DFA Router

`include/coroute/router.hpp` - Compile-time route registration and matching

### HTTP Parser

`include/coroute/http_parser.hpp` - Zero-copy HTTP/1.1 parsing

---

## 7. Competitor Ports (for benchmarking)

| Framework | Default Port |
|-----------|--------------|
| Coroute | 8080 |
| Drogon | 8081 |
| Crow | 8082 |
| Oat++ | 8083 |
| Express.js | 8084 |
| Flask | 8085 |

---

## 8. Style Notes for Thesis

- **Language:** Formal academic Bulgarian
- **Big-O notation:** Use capital N (O(N), not O(n))
- **Table footnotes:** Use `\multicolumn{X}{l}{\rule{0pt}{3ex}\textsuperscript{*}...}`
- **Chart scales:** Linear X-axis even for power-of-2 values (128, 256, 512, 1024)
- **Memory charts:** Logarithmic Y-axis when comparing C++ vs interpreted languages

---

## 9. Session Continuity Checklist

When starting the Linux session:

1. Read this file first
2. Read `prompt.txt` for detailed instructions
3. Verify io_uring backend compiles and runs
4. Run strace verification BEFORE benchmarking
5. Collect data, present to user for approval
6. Generate Bulgarian text for thesis updates

---

*End of context dump*
