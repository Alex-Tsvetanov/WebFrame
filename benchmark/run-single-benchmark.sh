#!/bin/bash
# =============================================================================
# Single Framework Benchmark Script for Linux
# Runs one specific test type for one framework
# =============================================================================

set -e

# Default values
FRAMEWORK=""
TEST_TYPE=""
RUNS=5
DURATION=30
LOW_CONNECTIONS=100
HIGH_CONNECTIONS=512
OUTPUT_DIR="results"

# Server ports
declare -A SERVER_PORTS=(
    ["coroute"]=8080
    ["drogon"]=8081
    ["crow"]=8082
    ["oatpp"]=8083
    ["express"]=8084
    ["flask"]=8085
)

# Server executables
declare -A SERVER_EXECUTABLES=(
    ["coroute"]="build/coroute/examples/hello_world"
    ["drogon"]="build/drogon/drogon_hello_world"
    ["crow"]="build/crow/crow_hello_world"
    ["oatpp"]="build/oatpp/oatpp_hello_world"
)

# =============================================================================
# Argument parsing
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --framework)
            FRAMEWORK="$2"
            shift 2
            ;;
        --test-type)
            TEST_TYPE="$2"
            shift 2
            ;;
        --runs)
            RUNS="$2"
            shift 2
            ;;
        --duration)
            DURATION="$2"
            shift 2
            ;;
        --low-connections)
            LOW_CONNECTIONS="$2"
            shift 2
            ;;
        --high-connections)
            HIGH_CONNECTIONS="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Validate required arguments
if [[ -z "$FRAMEWORK" || -z "$TEST_TYPE" ]]; then
    echo "Usage: $0 --framework <name> --test-type <type> [options]"
    echo "  --framework: coroute, drogon, crow, oatpp, express, flask"
    echo "  --test-type: low_normal, high_normal, low_stressed, high_stressed, profile"
    exit 1
fi

# =============================================================================
# Utility functions
# =============================================================================

log_info() { echo "[INFO] $1"; }
log_success() { echo "[SUCCESS] $1"; }
log_error() { echo "[ERROR] $1" >&2; }

# Check required tools
check_dependencies() {
    local missing=()
    
    if ! command -v wrk &> /dev/null; then
        missing+=("wrk")
    fi
    if ! command -v curl &> /dev/null; then
        missing+=("curl")
    fi
    if ! command -v bc &> /dev/null; then
        missing+=("bc")
    fi
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing[*]}"
        log_error "Install with: sudo apt-get install -y ${missing[*]}"
        exit 1
    fi
    
    log_info "All dependencies found"
}

get_port() {
    echo "${SERVER_PORTS[$FRAMEWORK]}"
}

wait_for_server() {
    local port=$1
    local max_wait=${2:-60}  # 30 seconds default
    local waited=0
    
    log_info "Waiting for server on port $port..."
    while [[ $waited -lt $max_wait ]]; do
        local http_code=$(curl -s -o /dev/null -w "%{http_code}" --connect-timeout 2 "http://localhost:$port/" 2>/dev/null || echo "000")
        if [[ "$http_code" =~ ^[23] ]]; then
            log_info "Server responded with HTTP $http_code after $waited attempts"
            return 0
        fi
        sleep 0.5
        ((waited++))
        if [[ $((waited % 10)) -eq 0 ]]; then
            log_info "Still waiting... ($waited attempts, last code: $http_code)"
        fi
    done
    log_error "Server did not respond after $max_wait attempts"
    return 1
}

kill_server_on_port() {
    local port=$1
    local pids=$(lsof -ti:$port 2>/dev/null || true)
    if [[ -n "$pids" ]]; then
        echo "$pids" | xargs kill -9 2>/dev/null || true
    fi
    sleep 1
}

get_memory_kb() {
    local pid=$1
    if [[ -f "/proc/$pid/status" ]]; then
        grep VmRSS /proc/$pid/status 2>/dev/null | awk '{print $2}' || echo "0"
    else
        echo "0"
    fi
}

calc_median() {
    local arr=("$@")
    local n=${#arr[@]}
    if [[ $n -eq 0 ]]; then
        echo "0"
        return
    fi
    
    IFS=$'\n' sorted=($(sort -n <<<"${arr[*]}")); unset IFS
    local mid=$((n / 2))
    
    if [[ $((n % 2)) -eq 0 ]]; then
        echo "scale=2; (${sorted[$mid-1]} + ${sorted[$mid]}) / 2" | bc
    else
        echo "${sorted[$mid]}"
    fi
}

# =============================================================================
# Server management
# =============================================================================

# Global to store server PID (avoids stdout capture issues)
SERVER_PID=""

start_server() {
    local port=$(get_port)
    kill_server_on_port $port
    
    log_info "Starting $FRAMEWORK server on port $port..."
    
    case $FRAMEWORK in
        coroute|drogon|crow|oatpp)
            local exe="${SERVER_EXECUTABLES[$FRAMEWORK]}"
            if [[ ! -x "$exe" ]]; then
                log_error "Executable not found or not executable: $exe"
                return 1
            fi
            log_info "Running: $exe"
            "$exe" > /tmp/server_${FRAMEWORK}.log 2>&1 &
            ;;
        express)
            (cd competitors/express && node cluster.js > /tmp/server_${FRAMEWORK}.log 2>&1) &
            ;;
        flask)
            (cd competitors/flask && python -m waitress --port=$port --threads=$(nproc) hello_world:app > /tmp/server_${FRAMEWORK}.log 2>&1) &
            ;;
    esac
    
    SERVER_PID=$!
    log_info "Server process started with PID: $SERVER_PID"
    
    # Give the server a moment to start
    sleep 1
    
    # Check if process is still running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        log_error "Server process died immediately. Log output:"
        cat /tmp/server_${FRAMEWORK}.log 2>/dev/null || true
        return 1
    fi
    
    if wait_for_server $port; then
        log_success "Server $FRAMEWORK started on port $port (PID: $SERVER_PID)"
        return 0
    else
        log_error "Failed to start $FRAMEWORK - server not responding"
        log_error "Server log output:"
        cat /tmp/server_${FRAMEWORK}.log 2>/dev/null || true
        kill $SERVER_PID 2>/dev/null || true
        return 1
    fi
}

stop_server() {
    local pid=$1
    kill $pid 2>/dev/null || true
    wait $pid 2>/dev/null || true
    kill_server_on_port $(get_port) || true
    return 0
}

# =============================================================================
# Network simulation
# =============================================================================

setup_network_stress() {
    log_info "Setting up network stress (50ms delay, 1% loss)..."
    sudo tc qdisc del dev lo root 2>/dev/null || true
    sudo tc qdisc add dev lo root netem delay 50ms 10ms distribution normal loss 1%
}

teardown_network_stress() {
    log_info "Removing network stress..."
    sudo tc qdisc del dev lo root 2>/dev/null || true
    return 0
}

# =============================================================================
# Benchmark functions
# =============================================================================

run_wrk_benchmark() {
    local url=$1
    local duration=$2
    local connections=$3
    local output_file=$4
    
    local threads=$(nproc)
    [[ $threads -gt $connections ]] && threads=$connections
    
    wrk -t$threads -c$connections -d${duration}s --latency "$url" > "$output_file" 2>&1
}

parse_wrk_output() {
    local file=$1
    
    local latency=$(grep -E "^\s+Latency" "$file" | awk '{print $2}' | head -1)
    local throughput=$(grep "Requests/sec:" "$file" | awk '{print $2}')
    
    # Convert latency to microseconds
    local latency_us=0
    if [[ "$latency" =~ ([0-9.]+)us ]]; then
        latency_us="${BASH_REMATCH[1]}"
    elif [[ "$latency" =~ ([0-9.]+)ms ]]; then
        latency_us=$(echo "${BASH_REMATCH[1]} * 1000" | bc)
    elif [[ "$latency" =~ ([0-9.]+)s ]]; then
        latency_us=$(echo "${BASH_REMATCH[1]} * 1000000" | bc)
    fi
    
    echo "$latency_us,$throughput"
}

run_benchmark_iteration() {
    local url=$1
    local connections=$2
    local run_num=$3
    local output_prefix=$4
    
    local wrk_output="${output_prefix}_run${run_num}.txt"
    
    # Start memory monitoring
    local mem_samples=()
    local mem_monitor_file=$(mktemp)
    (
        while true; do
            local mem=$(get_memory_kb $SERVER_PID)
            echo "$mem" >> "$mem_monitor_file"
            sleep 0.5
        done
    ) &
    local monitor_pid=$!
    
    # Run benchmark
    run_wrk_benchmark "$url" "$DURATION" "$connections" "$wrk_output"
    
    # Stop memory monitoring
    kill $monitor_pid 2>/dev/null || true
    wait $monitor_pid 2>/dev/null || true
    
    # Parse results
    local metrics=$(parse_wrk_output "$wrk_output")
    IFS=',' read -r latency throughput <<< "$metrics"
    
    # Get memory stats
    local mem_peak=0
    local mem_avg=0
    if [[ -f "$mem_monitor_file" ]]; then
        mem_peak=$(sort -n "$mem_monitor_file" | tail -1)
        mem_avg=$(awk '{sum+=$1; count++} END {if(count>0) print sum/count; else print 0}' "$mem_monitor_file")
    fi
    rm -f "$mem_monitor_file"
    
    # Get CPU usage
    local cpu_percent=$(ps -p $SERVER_PID -o %cpu --no-headers 2>/dev/null | tr -d ' ' || echo "0")
    
    echo "$latency,$throughput,$mem_peak,$mem_avg,$cpu_percent"
}

run_profile() {
    local url=$1
    local output_dir=$2
    
    log_info "Running syscall profiling on PID $SERVER_PID..."
    
    # Use strace to profile syscalls
    local strace_output="$output_dir/strace.txt"
    
    # Run strace on the server while generating load
    timeout 30 strace -c -p "$SERVER_PID" -o "$strace_output" &
    local strace_pid=$!
    
    # Generate load
    wrk -t2 -c50 -d20s "$url" > /dev/null 2>&1
    
    # Wait for strace
    wait $strace_pid 2>/dev/null || true
    
    # Also capture perf stats if available
    if command -v perf &> /dev/null; then
        log_info "Running perf profiling on PID $SERVER_PID..."
        local perf_output="$output_dir/perf.txt"
        timeout 30 perf stat -p "$SERVER_PID" -o "$perf_output" &
        local perf_pid=$!
        
        wrk -t2 -c50 -d20s "$url" > /dev/null 2>&1
        
        wait $perf_pid 2>/dev/null || true
    fi
    
    log_success "Profiling complete"
}

# =============================================================================
# Main benchmark logic
# =============================================================================

run_test() {
    local port=$(get_port)
    local url="http://localhost:$port/"
    
    mkdir -p "$OUTPUT_DIR"
    
    # Determine connections based on test type
    local connections=$LOW_CONNECTIONS
    local is_stressed=false
    local is_profile=false
    
    case $TEST_TYPE in
        low_normal)
            connections=$LOW_CONNECTIONS
            ;;
        high_normal)
            connections=$HIGH_CONNECTIONS
            ;;
        low_stressed)
            connections=$LOW_CONNECTIONS
            is_stressed=true
            ;;
        high_stressed)
            connections=$HIGH_CONNECTIONS
            is_stressed=true
            ;;
        profile)
            is_profile=true
            ;;
    esac
    
    log_info "Starting benchmark: $FRAMEWORK - $TEST_TYPE"
    log_info "  Connections: $connections"
    log_info "  Duration: ${DURATION}s"
    log_info "  Runs: $RUNS"
    
    # Start server
    start_server || exit 1
    
    # Setup network stress if needed
    if [[ "$is_stressed" == "true" ]]; then
        setup_network_stress
    fi
    
    local output_prefix="$OUTPUT_DIR/${FRAMEWORK}_${TEST_TYPE}"
    
    if [[ "$is_profile" == "true" ]]; then
        # Profile mode
        run_profile "$url" "$OUTPUT_DIR"
        
        # Write minimal results
        cat > "$OUTPUT_DIR/results.csv" << EOF
framework,test_type,metric,value
$FRAMEWORK,$TEST_TYPE,profiled,1
EOF
    else
        # Benchmark mode
        local latencies=()
        local throughputs=()
        local mem_peaks=()
        local mem_avgs=()
        local cpu_usages=()
        
        # Warmup
        log_info "Warming up..."
        wrk -t2 -c10 -d5s "$url" > /dev/null 2>&1
        
        # Run iterations
        for run in $(seq 1 $RUNS); do
            log_info "Run $run/$RUNS..."
            
            local result=$(run_benchmark_iteration "$url" "$connections" "$run" "$output_prefix")
            IFS=',' read -r lat tp mem_p mem_a cpu <<< "$result"
            
            latencies+=("$lat")
            throughputs+=("$tp")
            mem_peaks+=("$mem_p")
            mem_avgs+=("$mem_a")
            cpu_usages+=("$cpu")
            
            log_info "  Throughput: $tp req/s, Latency: ${lat}us"
            
            sleep 2
        done
        
        # Calculate medians
        local median_latency=$(calc_median "${latencies[@]}")
        local median_throughput=$(calc_median "${throughputs[@]}")
        local median_mem_peak=$(calc_median "${mem_peaks[@]}")
        local median_mem_avg=$(calc_median "${mem_avgs[@]}")
        local median_cpu=$(calc_median "${cpu_usages[@]}")
        
        # Write results
        cat > "$OUTPUT_DIR/results.csv" << EOF
framework,test_type,latency_us,throughput_req_s,mem_peak_kb,mem_avg_kb,cpu_percent
$FRAMEWORK,$TEST_TYPE,$median_latency,$median_throughput,$median_mem_peak,$median_mem_avg,$median_cpu
EOF
        
        # Write raw data
        cat > "$OUTPUT_DIR/raw_data.csv" << EOF
run,latency_us,throughput_req_s,mem_peak_kb,mem_avg_kb,cpu_percent
EOF
        for i in $(seq 0 $((RUNS - 1))); do
            echo "$((i+1)),${latencies[$i]},${throughputs[$i]},${mem_peaks[$i]},${mem_avgs[$i]},${cpu_usages[$i]}" >> "$OUTPUT_DIR/raw_data.csv"
        done
        
        log_success "Results:"
        log_success "  Latency (median): ${median_latency}us"
        log_success "  Throughput (median): ${median_throughput} req/s"
        log_success "  Memory peak (median): ${median_mem_peak} KB"
    fi
    
    # Cleanup (disable set -e to ensure cleanup completes)
    set +e
    if [[ "$is_stressed" == "true" ]]; then
        teardown_network_stress
    fi
    
    stop_server "$SERVER_PID" || true
    set -e
    
    log_success "Benchmark complete: $FRAMEWORK - $TEST_TYPE"
}

# =============================================================================
# Entry point
# =============================================================================

check_dependencies
run_test

# Explicit success exit
exit 0
