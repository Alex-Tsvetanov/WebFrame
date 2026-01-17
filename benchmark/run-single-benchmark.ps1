#Requires -Version 5.1
<#
.SYNOPSIS
    Single Framework Benchmark Script for Windows
    Runs one specific test type for one framework

.PARAMETER Framework
    Framework to benchmark: coroute, drogon, crow, oatpp, express, flask

.PARAMETER TestType
    Test type: low_normal, high_normal, low_stressed, high_stressed, profile

.PARAMETER Runs
    Number of benchmark iterations (default: 5)

.PARAMETER Duration
    Duration of each test in seconds (default: 30)

.PARAMETER LowConnections
    Low connection count (default: 100)

.PARAMETER HighConnections
    High connection count (default: 512)

.PARAMETER OutputDir
    Output directory for results (default: results)
#>

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("coroute", "drogon", "crow", "oatpp", "express", "flask")]
    [string]$Framework,
    
    [Parameter(Mandatory = $true)]
    [ValidateSet("low_normal", "high_normal", "low_stressed", "high_stressed", "profile")]
    [string]$TestType,
    
    [int]$Runs = 5,
    [int]$Duration = 30,
    [int]$LowConnections = 100,
    [int]$HighConnections = 512,
    [string]$OutputDir = "results"
)

$ErrorActionPreference = "Continue"
$ProgressPreference = "SilentlyContinue"

# =============================================================================
# Configuration
# =============================================================================

$ServerPorts = @{
    "coroute" = 8080
    "drogon"  = 8081
    "crow"    = 8082
    "oatpp"   = 8083
    "express" = 8084
    "flask"   = 8085
}

$ServerExecutables = @{
    "coroute" = "build\coroute\examples\Release\hello_world.exe"
    "drogon"  = "build\drogon\Release\drogon_hello_world.exe"
    "crow"    = "build\crow\Release\crow_hello_world.exe"
    "oatpp"   = "build\oatpp\Release\oatpp_hello_world.exe"
}

# =============================================================================
# Utility Functions
# =============================================================================

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "HH:mm:ss"
    $color = switch ($Level) {
        "INFO" { "Cyan" }
        "SUCCESS" { "Green" }
        "WARN" { "Yellow" }
        "ERROR" { "Red" }
        default { "White" }
    }
    Write-Host "[$timestamp] [$Level] $Message" -ForegroundColor $color
}

function Get-Port {
    return $ServerPorts[$Framework]
}

function Test-ServerRunning {
    param([int]$Port, [int]$MaxWait = 30)
    
    $waited = 0
    while ($waited -lt $MaxWait) {
        try {
            $response = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/" -TimeoutSec 2 -ErrorAction SilentlyContinue
            if ($response.StatusCode -eq 200) {
                return $true
            }
        }
        catch {}
        Start-Sleep -Milliseconds 500
        $waited++
    }
    return $false
}

function Stop-ServerOnPort {
    param([int]$Port)
    
    $processes = Get-NetTCPConnection -LocalPort $Port -ErrorAction SilentlyContinue | 
    Select-Object -ExpandProperty OwningProcess -Unique
    
    foreach ($procId in $processes) {
        try {
            Stop-Process -Id $procId -Force -ErrorAction SilentlyContinue
        }
        catch {}
    }
    Start-Sleep -Seconds 1
}

function Get-ProcessMemoryKB {
    param([int]$ProcessId)
    
    try {
        $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
        if ($process) {
            return [math]::Round($process.WorkingSet64 / 1KB, 0)
        }
    }
    catch {}
    return 0
}

function Get-Median {
    param([double[]]$Values)
    
    if ($Values.Count -eq 0) { return 0 }
    
    $sorted = $Values | Sort-Object
    $mid = [math]::Floor($sorted.Count / 2)
    
    if ($sorted.Count % 2 -eq 0) {
        return ($sorted[$mid - 1] + $sorted[$mid]) / 2
    }
    else {
        return $sorted[$mid]
    }
}

function Find-BenchmarkTool {
    # Try to find hey or wrk
    $tools = @("hey", "wrk", "ab")
    
    foreach ($tool in $tools) {
        $path = Get-Command $tool -ErrorAction SilentlyContinue
        if ($path) {
            return @{ Name = $tool; Path = $path.Source }
        }
    }
    
    # Check common locations
    $commonPaths = @(
        "$env:USERPROFILE\hey.exe",
        "C:\tools\hey.exe"
    )
    
    foreach ($p in $commonPaths) {
        if (Test-Path $p) {
            return @{ Name = "hey"; Path = $p }
        }
    }
    
    return $null
}

# =============================================================================
# Server Management
# =============================================================================

function Start-Server {
    $port = Get-Port
    Stop-ServerOnPort $port
    
    $process = $null
    
    switch ($Framework) {
        { $_ -in @("coroute", "drogon", "crow", "oatpp") } {
            $exe = $ServerExecutables[$Framework]
            if (-not (Test-Path $exe)) {
                Write-Log "Executable not found: $exe" -Level "ERROR"
                return $null
            }
            $process = Start-Process -FilePath $exe -PassThru -WindowStyle Hidden
        }
        "express" {
            $process = Start-Process -FilePath "node" -ArgumentList "competitors\express\cluster.js" -PassThru -WindowStyle Hidden
        }
        "flask" {
            $waitressArgs = "-m waitress --port=$port --threads=$env:NUMBER_OF_PROCESSORS hello_world:app"
            $process = Start-Process -FilePath "python" -ArgumentList $waitressArgs -WorkingDirectory "competitors\flask" -PassThru -WindowStyle Hidden
        }
    }
    
    if ($process -and (Test-ServerRunning -Port $port)) {
        Write-Log "$Framework started on port $port (PID: $($process.Id))" -Level "SUCCESS"
        return $process
    }
    
    if ($process) {
        try { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue } catch {}
    }
    
    Write-Log "Failed to start $Framework" -Level "ERROR"
    return $null
}

function Stop-Server {
    param($Process)
    
    if ($Process) {
        try {
            Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
        }
        catch {}
    }
    Stop-ServerOnPort (Get-Port)
}

# =============================================================================
# Benchmark Functions
# =============================================================================

function Run-Benchmark {
    param(
        [string]$Url,
        [int]$BenchDuration,
        [int]$Connections,
        [string]$OutputFile,
        [hashtable]$Tool
    )
    
    $result = @{
        Throughput = 0
        LatencyUs  = 0
    }
    
    try {
        switch ($Tool.Name) {
            "hey" {
                $totalRequests = $Connections * $BenchDuration * 100
                $output = & $Tool.Path -n $totalRequests -c $Connections -t $BenchDuration $Url 2>&1
                $output | Out-File -FilePath $OutputFile -Encoding UTF8
                
                foreach ($line in $output) {
                    if ($line -match "Requests/sec:\s+([\d.]+)") {
                        $result.Throughput = [double]$Matches[1]
                    }
                    if ($line -match "Average:\s+([\d.]+)\s*secs") {
                        $result.LatencyUs = [double]$Matches[1] * 1000000
                    }
                }
            }
            "wrk" {
                $output = & $Tool.Path -t4 -c$Connections -d"${BenchDuration}s" --latency $Url 2>&1
                $output | Out-File -FilePath $OutputFile -Encoding UTF8
                
                foreach ($line in $output) {
                    if ($line -match "Requests/sec:\s+([\d.]+)") {
                        $result.Throughput = [double]$Matches[1]
                    }
                    if ($line -match "Latency\s+([\d.]+)(us|ms|s)") {
                        $val = [double]$Matches[1]
                        $unit = $Matches[2]
                        $result.LatencyUs = switch ($unit) {
                            "us" { $val }
                            "ms" { $val * 1000 }
                            "s" { $val * 1000000 }
                        }
                    }
                }
            }
        }
    }
    catch {
        Write-Log "Benchmark error: $_" -Level "ERROR"
    }
    
    return $result
}

function Run-BenchmarkIteration {
    param(
        [string]$Url,
        [int]$Connections,
        [int]$RunNum,
        [string]$OutputPrefix,
        [int]$ServerPid,
        [hashtable]$Tool
    )
    
    $outputFile = "${OutputPrefix}_run${RunNum}.txt"
    
    # Start memory monitoring
    $memSamples = [System.Collections.ArrayList]@()
    $monitorJob = Start-Job -ScriptBlock {
        param($processId, $duration)
        $samples = @()
        $end = (Get-Date).AddSeconds($duration + 5)
        while ((Get-Date) -lt $end) {
            try {
                $proc = Get-Process -Id $processId -ErrorAction SilentlyContinue
                if ($proc) {
                    $samples += [math]::Round($proc.WorkingSet64 / 1KB, 0)
                }
            }
            catch {}
            Start-Sleep -Milliseconds 500
        }
        return $samples
    } -ArgumentList $ServerPid, $Duration
    
    # Run benchmark
    $result = Run-Benchmark -Url $Url -BenchDuration $Duration -Connections $Connections -OutputFile $outputFile -Tool $Tool
    
    # Get memory samples
    $memSamples = Receive-Job -Job $monitorJob -Wait
    Remove-Job -Job $monitorJob -Force
    
    # Calculate memory stats
    $memPeak = 0
    $memAvg = 0
    if ($memSamples -and $memSamples.Count -gt 0) {
        $memPeak = ($memSamples | Measure-Object -Maximum).Maximum
        $memAvg = ($memSamples | Measure-Object -Average).Average
    }
    
    # Get CPU usage
    $cpuPercent = 0
    try {
        $proc = Get-Process -Id $ServerPid -ErrorAction SilentlyContinue
        if ($proc) {
            $cpuPercent = $proc.CPU
        }
    }
    catch {}
    
    return @{
        LatencyUs  = $result.LatencyUs
        Throughput = $result.Throughput
        MemPeakKB  = $memPeak
        MemAvgKB   = $memAvg
        CpuPercent = $cpuPercent
    }
}

function Run-Profile {
    param(
        [string]$Url,
        [int]$ServerPid,
        [string]$OutputPath,
        [hashtable]$Tool
    )
    
    Write-Log "Running profiling (limited on Windows)..."
    
    # On Windows, we can capture performance counters
    $perfOutput = Join-Path $OutputPath "perf_counters.csv"
    
    # Start performance counter collection
    $counters = @(
        "\Process($ServerPid)\% Processor Time",
        "\Process($ServerPid)\Working Set",
        "\Process($ServerPid)\IO Read Bytes/sec",
        "\Process($ServerPid)\IO Write Bytes/sec"
    )
    
    try {
        # Generate load while collecting counters
        $counterJob = Start-Job -ScriptBlock {
            param($counters, $duration, $output)
            Get-Counter -Counter $counters -SampleInterval 1 -MaxSamples $duration |
            Export-Counter -Path $output -FileFormat CSV -Force
        } -ArgumentList $counters, 20, $perfOutput
        
        # Generate load
        & $Tool.Path -n 10000 -c 50 -t 20 $Url 2>&1 | Out-Null
        
        Receive-Job -Job $counterJob -Wait -ErrorAction SilentlyContinue
        Remove-Job -Job $counterJob -Force -ErrorAction SilentlyContinue
    }
    catch {
        Write-Log "Performance counter collection failed: $_" -Level "WARN"
    }
    
    Write-Log "Profiling complete" -Level "SUCCESS"
}

# =============================================================================
# Main Test Logic
# =============================================================================

function Run-Test {
    $port = Get-Port
    $url = "http://127.0.0.1:$port/"
    
    # Create output directory
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
    
    # Find benchmark tool
    $tool = Find-BenchmarkTool
    if (-not $tool) {
        Write-Log "No benchmark tool found (hey or wrk). Please install one." -Level "ERROR"
        exit 1
    }
    Write-Log "Using benchmark tool: $($tool.Name)"
    
    # Determine connections based on test type
    $connections = $LowConnections
    $isStressed = $false
    $isProfile = $false
    
    switch ($TestType) {
        "low_normal" { $connections = $LowConnections }
        "high_normal" { $connections = $HighConnections }
        "low_stressed" { $connections = $LowConnections; $isStressed = $true }
        "high_stressed" { $connections = $HighConnections; $isStressed = $true }
        "profile" { $isProfile = $true }
    }
    
    Write-Log "Starting benchmark: $Framework - $TestType"
    Write-Log "  Connections: $connections"
    Write-Log "  Duration: ${Duration}s"
    Write-Log "  Runs: $Runs"
    
    # Note: Network stress simulation not available on Windows without admin tools
    if ($isStressed) {
        Write-Log "Note: Network stress simulation limited on Windows (no tc equivalent)" -Level "WARN"
    }
    
    # Start server
    $process = Start-Server
    if (-not $process) {
        exit 1
    }
    
    $outputPrefix = Join-Path $OutputDir "${Framework}_${TestType}"
    
    if ($isProfile) {
        # Profile mode
        Run-Profile -Url $url -ServerPid $process.Id -OutputPath $OutputDir -Tool $tool
        
        # Write minimal results
        @"
framework,test_type,metric,value
$Framework,$TestType,profiled,1
"@ | Out-File -FilePath (Join-Path $OutputDir "results.csv") -Encoding UTF8
    }
    else {
        # Benchmark mode
        $latencies = @()
        $throughputs = @()
        $memPeaks = @()
        $memAvgs = @()
        $cpuUsages = @()
        
        # Warmup
        Write-Log "Warming up..."
        & $tool.Path -n 1000 -c 10 -t 5 $url 2>&1 | Out-Null
        
        # Run iterations
        for ($run = 1; $run -le $Runs; $run++) {
            Write-Log "Run $run/$Runs..."
            
            $result = Run-BenchmarkIteration -Url $url -Connections $connections -RunNum $run -OutputPrefix $outputPrefix -ServerPid $process.Id -Tool $tool
            
            $latencies += $result.LatencyUs
            $throughputs += $result.Throughput
            $memPeaks += $result.MemPeakKB
            $memAvgs += $result.MemAvgKB
            $cpuUsages += $result.CpuPercent
            
            Write-Log "  Throughput: $([math]::Round($result.Throughput, 0)) req/s, Latency: $([math]::Round($result.LatencyUs, 2))us"
            
            Start-Sleep -Seconds 2
        }
        
        # Calculate medians
        $medianLatency = Get-Median $latencies
        $medianThroughput = Get-Median $throughputs
        $medianMemPeak = Get-Median $memPeaks
        $medianMemAvg = Get-Median $memAvgs
        $medianCpu = Get-Median $cpuUsages
        
        # Write results
        @"
framework,test_type,latency_us,throughput_req_s,mem_peak_kb,mem_avg_kb,cpu_percent
$Framework,$TestType,$medianLatency,$medianThroughput,$medianMemPeak,$medianMemAvg,$medianCpu
"@ | Out-File -FilePath (Join-Path $OutputDir "results.csv") -Encoding UTF8
        
        # Write raw data
        $rawData = @("run,latency_us,throughput_req_s,mem_peak_kb,mem_avg_kb,cpu_percent")
        for ($i = 0; $i -lt $Runs; $i++) {
            $rawData += "$($i+1),$($latencies[$i]),$($throughputs[$i]),$($memPeaks[$i]),$($memAvgs[$i]),$($cpuUsages[$i])"
        }
        $rawData | Out-File -FilePath (Join-Path $OutputDir "raw_data.csv") -Encoding UTF8
        
        Write-Log "Results:" -Level "SUCCESS"
        Write-Log "  Latency (median): $([math]::Round($medianLatency, 2))us" -Level "SUCCESS"
        Write-Log "  Throughput (median): $([math]::Round($medianThroughput, 0)) req/s" -Level "SUCCESS"
        Write-Log "  Memory peak (median): $([math]::Round($medianMemPeak, 0)) KB" -Level "SUCCESS"
    }
    
    # Cleanup
    Stop-Server $process
    
    Write-Log "Benchmark complete: $Framework - $TestType" -Level "SUCCESS"
}

# =============================================================================
# Entry Point
# =============================================================================

Run-Test
