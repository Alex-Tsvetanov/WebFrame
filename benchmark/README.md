# HTTP Server Benchmark Suite

Comprehensive benchmarking suite for comparing Coroute v2 against popular HTTP server frameworks.

## Servers Tested

| Server | Language | Port | Type |
|--------|----------|------|------|
| Coroute v2 | C++20 | 8080 | Coroutine-based |
| Drogon | C++17 | 8081 | Async callback |
| Crow | C++14 | 8082 | Header-only |
| Oat++ | C++11 | 8083 | Zero-copy |
| Express.js | Node.js | 8084 | Event-driven |
| Flask | Python | 8085 | WSGI |

## Metrics Collected

- **Latency** - Response time (median, p50, p99)
- **Throughput** - Requests per second
- **Memory Usage** - Idle, peak, and average RSS/Working Set
- **CPU Usage** - Average processor utilization
- **Syscall Profiling** - System call distribution (Linux only)
- **Stressed Network** - Performance under high connection count

## Why Median?

This benchmark uses **median** values rather than averages because:

1. **Robustness to outliers** - Network benchmarks often have occasional spikes
2. **Better representation** - Median shows "typical" performance
3. **Scientific validity** - Preferred in performance research for skewed distributions

## Prerequisites

### Linux Prerequisites

```bash
# Ubuntu/Debian
sudo apt install wrk curl strace bc jq cmake build-essential

# Optional: perf for advanced CPU profiling
sudo apt install linux-tools-common linux-tools-generic

# For Node.js servers
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install nodejs

# For Python servers
sudo apt install python3 python3-venv python3-pip

# For C++ competitors (optional - requires vcpkg)
# vcpkg install drogon crow oatpp
```

### Windows Prerequisites

```powershell
# Install Chocolatey or Scoop first, then:

# Using Scoop (recommended)
scoop install bombardier cmake nodejs python

# Or using Chocolatey
choco install bombardier cmake nodejs python

# For C++ competitors (optional)
# Install vcpkg and run: vcpkg install drogon crow oatpp
```

## Usage

### Linux Usage

```bash
cd v2/benchmark

# Run full benchmark suite
./benchmark.sh

# Customize parameters
BENCHMARK_RUNS=10 TEST_DURATION=60 ./benchmark.sh

# Enable network simulation (requires sudo for tc)
sudo ENABLE_NETWORK_SIM=true ./benchmark.sh

# Environment variables:
# - BENCHMARK_RUNS: Number of iterations per server (default: 5)
# - WARMUP_DURATION: Warmup time in seconds (default: 5)
# - TEST_DURATION: Test duration in seconds (default: 30)
# - CONNECTIONS: Normal load connections (default: 100)
# - THREADS: wrk threads (default: nproc)
# - STRESSED_CONNECTIONS: High load connections (default: 1000)
# - STRESSED_THREADS: High load threads (default: nproc)
#
# Network simulation (requires tc/netem):
# - ENABLE_NETWORK_SIM: Enable network simulation (default: false)
# - NETWORK_DELAY_MS: Base delay in milliseconds (default: 50)
# - NETWORK_JITTER_MS: Jitter in milliseconds (default: 10)
# - NETWORK_LOSS_PERCENT: Packet loss percentage (default: 1)
```

### Windows Usage

```powershell
cd v2\benchmark

# Run full benchmark suite
.\benchmark-windows.ps1

# Customize parameters
.\benchmark-windows.ps1 -BenchmarkRuns 10 -TestDuration 60

# Parameters:
# -ResultsDir: Output directory (default: results\YYYYMMDD_HHMMSS)
# -BenchmarkRuns: Number of iterations per server (default: 5)
# -TestDuration: Test duration in seconds (default: 30)
# -Connections: Normal load connections (default: 100)
# -StressedConnections: High load connections (default: 1000)
# -Threads: Number of threads for benchmark tool (default: CPU count)
```

### GitHub Actions

The benchmark suite runs automatically via GitHub Actions with a comprehensive matrix strategy:

**Workflow Structure (63 jobs total):**

1. **Setup Jobs (2)**: Build and cache all C++ servers for Linux and Windows
2. **Benchmark Matrix (60)**: 6 frameworks × 2 OS × 5 test types
3. **Summarize (1)**: Aggregate results, generate report, comment on PR

**Test Types:**

- `low_normal` - 100 connections, normal network
- `high_normal` - 512 connections, normal network  
- `low_stressed` - 100 connections, stressed network (50ms delay, 1% loss)
- `high_stressed` - 512 connections, stressed network
- `profile` - Syscall/performance profiling

**Triggers:**

```yaml
on:
  workflow_dispatch:  # Manual trigger with configurable parameters
  pull_request:
    paths:
      - "v2/**"
      - "competitors/**"
```

**Outputs:**

- Individual results per framework/OS/test uploaded as artifacts
- Combined report with charts and tables
- ZIP archive of all results
- PR comment with summary and download link

## Output Structure

```text
v2/benchmark/
├── build/                    # Compiled binaries (gitignored)
│   ├── coroute/
│   ├── drogon/
│   ├── crow/
│   ├── oatpp/
│   └── flask_venv/
├── results/                  # Benchmark results (gitignored)
│   └── YYYYMMDD_HHMMSS/     # Timestamped run
│       ├── report.csv        # Summary comparison
│       ├── coroute/
│       │   ├── summary.csv   # Median metrics
│       │   ├── raw_data.csv  # All run data
│       │   ├── wrk_run*.txt  # Raw wrk output
│       │   ├── resources_run*.csv
│       │   └── strace.txt    # Syscall profile (Linux)
│       ├── drogon/
│       ├── crow/
│       └── ...
├── benchmark.sh              # Linux script
├── benchmark.ps1             # Windows script
├── analyze.py                # Report generator
└── README.md
```

## Report Format

### summary.csv

```csv
metric,value,unit
latency_median,45.23,us
throughput_median,125000,req/s
memory_idle,12.5,KB
memory_peak,45.2,KB
memory_avg,28.3,KB
cpu_avg,85.2,%
stressed_latency_median,120.5,us
stressed_throughput_median,95000,req/s
```

### report.csv (comparison)

```csv
server,latency_us,throughput_req_s,mem_idle_kb,mem_peak_kb,mem_avg_kb,cpu_percent,stressed_latency_us,stressed_throughput_req_s
coroute,45.23,125000,12500,45200,28300,85.2,120.5,95000
drogon,52.10,118000,15800,52100,35600,82.1,145.2,88000
...
```

## Analyzing Results

```bash
# Generate HTML report with charts
python3 analyze.py results/YYYYMMDD_HHMMSS/

# Compare multiple runs
python3 analyze.py results/run1/ results/run2/ --compare
```

## Notes

1. **Isolation** - Run benchmarks on an idle system for consistent results
2. **Warmup** - Each server is warmed up before measurements
3. **Multiple runs** - 5 iterations by default, median taken
4. **Fair comparison** - All servers use equivalent routes and responses
5. **Build optimization** - All C++ servers built with `-O3` (Release mode)

## Troubleshooting

### "wrk not found" (Linux)

```bash
# Build from source if not in package manager
git clone https://github.com/wg/wrk.git
cd wrk && make && sudo cp wrk /usr/local/bin/
```

### "bombardier not found" (Windows)

```powershell
# Download from GitHub releases
# https://github.com/codesenberg/bombardier/releases
# Or use scoop: scoop install bombardier
```

### Server fails to build

- Ensure vcpkg is installed and `VCPKG_ROOT` is set
- Install required packages: `vcpkg install drogon crow oatpp`
- The benchmark will skip servers that fail to build

### Port already in use

The scripts automatically kill processes on benchmark ports before starting.
If issues persist, manually check: `netstat -tlnp | grep 808` (Linux) or
`Get-NetTCPConnection -LocalPort 808*` (Windows)
