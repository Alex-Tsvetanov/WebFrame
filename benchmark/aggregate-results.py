#!/usr/bin/env python3
"""
Aggregate Benchmark Results

Combines results from multiple benchmark jobs into a comprehensive report.
Generates summary tables, charts, and markdown report for PR comments.

Usage:
    python aggregate-results.py --input-dir all-results/ --output-dir final-report/
"""

import argparse
import csv
import json
import os
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
from collections import defaultdict

try:
    import pandas as pd
    HAS_PANDAS = True
except ImportError:
    HAS_PANDAS = False

try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


# =============================================================================
# Data Loading
# =============================================================================

def parse_artifact_name(name: str) -> Optional[Dict[str, str]]:
    """Parse artifact name like 'results-ubuntu-22.04-coroute-low_normal'."""
    if not name.startswith('results-'):
        return None
    
    parts = name[8:].split('-')  # Remove 'results-' prefix
    if len(parts) < 3:
        return None
    
    # Handle OS names with dots (ubuntu-22.04, windows-2022)
    os_name = parts[0]
    if len(parts) > 3 and parts[1].replace('.', '').isdigit():
        os_name = f"{parts[0]}-{parts[1]}"
        parts = [os_name] + parts[2:]
    
    if len(parts) < 3:
        return None
    
    return {
        'os': parts[0],
        'framework': parts[1],
        'test_type': '_'.join(parts[2:])  # Handle test types with underscores
    }


def load_results_csv(path: Path) -> List[Dict[str, Any]]:
    """Load a results.csv file."""
    if not path.exists():
        return []
    
    results = []
    with open(path, 'r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            # Convert numeric fields
            for key in row:
                if key not in ('framework', 'test_type', 'metric'):
                    try:
                        row[key] = float(row[key]) if row[key] else 0.0
                    except ValueError:
                        pass
            results.append(row)
    return results


def load_all_results(input_dir: Path) -> Dict[str, List[Dict[str, Any]]]:
    """Load all results from artifact directories."""
    all_results = defaultdict(list)
    
    for artifact_dir in input_dir.iterdir():
        if not artifact_dir.is_dir():
            continue
        
        meta = parse_artifact_name(artifact_dir.name)
        if not meta:
            continue
        
        # Find results.csv in the artifact directory
        results_csv = artifact_dir / 'results.csv'
        if not results_csv.exists():
            # Try subdirectories
            for subdir in artifact_dir.rglob('results.csv'):
                results_csv = subdir
                break
        
        if results_csv.exists():
            results = load_results_csv(results_csv)
            for result in results:
                result['os'] = meta['os']
                result['test_type'] = meta['test_type']
                all_results[meta['framework']].append(result)
    
    return dict(all_results)


# =============================================================================
# Data Processing
# =============================================================================

def aggregate_by_framework_and_os(results: Dict[str, List[Dict]]) -> pd.DataFrame:
    """Create a summary DataFrame grouped by framework and OS."""
    if not HAS_PANDAS:
        return None
    
    rows = []
    for framework, data in results.items():
        for entry in data:
            rows.append({
                'framework': framework,
                'os': entry.get('os', 'unknown'),
                'test_type': entry.get('test_type', 'unknown'),
                'latency_us': entry.get('latency_us', 0),
                'throughput_req_s': entry.get('throughput_req_s', 0),
                'mem_peak_kb': entry.get('mem_peak_kb', 0),
                'mem_avg_kb': entry.get('mem_avg_kb', 0),
                'cpu_percent': entry.get('cpu_percent', 0),
            })
    
    if not rows:
        return pd.DataFrame()
    
    return pd.DataFrame(rows)


def find_winners(df: pd.DataFrame) -> Dict[str, Dict[str, str]]:
    """Find winners for each metric and test type."""
    if df.empty:
        return {}
    
    winners = {}
    
    for test_type in df['test_type'].unique():
        if test_type == 'profile':
            continue
        
        subset = df[df['test_type'] == test_type]
        if subset.empty:
            continue
        
        winners[test_type] = {}
        
        # Best throughput (higher is better)
        if 'throughput_req_s' in subset.columns:
            best = subset.loc[subset['throughput_req_s'].idxmax()]
            winners[test_type]['throughput'] = f"{best['framework']} ({best['os']})"
        
        # Best latency (lower is better)
        if 'latency_us' in subset.columns:
            valid = subset[subset['latency_us'] > 0]
            if not valid.empty:
                best = valid.loc[valid['latency_us'].idxmin()]
                winners[test_type]['latency'] = f"{best['framework']} ({best['os']})"
        
        # Best memory (lower is better)
        if 'mem_peak_kb' in subset.columns:
            valid = subset[subset['mem_peak_kb'] > 0]
            if not valid.empty:
                best = valid.loc[valid['mem_peak_kb'].idxmin()]
                winners[test_type]['memory'] = f"{best['framework']} ({best['os']})"
    
    return winners


# =============================================================================
# Chart Generation
# =============================================================================

def generate_throughput_chart(df: pd.DataFrame, output_path: Path):
    """Generate throughput comparison chart."""
    if not HAS_MATPLOTLIB or df.empty:
        return
    
    # Filter to normal tests only
    normal_df = df[df['test_type'].isin(['low_normal', 'high_normal'])]
    if normal_df.empty:
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    for idx, (test_type, title) in enumerate([
        ('low_normal', 'Low Connections (100)'),
        ('high_normal', 'High Connections (512)')
    ]):
        ax = axes[idx]
        subset = normal_df[normal_df['test_type'] == test_type]
        
        if subset.empty:
            continue
        
        # Group by framework and OS
        pivot = subset.pivot_table(
            values='throughput_req_s',
            index='framework',
            columns='os',
            aggfunc='mean'
        )
        
        pivot.plot(kind='bar', ax=ax, width=0.8)
        ax.set_title(f'Throughput - {title}')
        ax.set_ylabel('Requests/second')
        ax.set_xlabel('Framework')
        ax.legend(title='OS')
        ax.tick_params(axis='x', rotation=45)
        
        # Add value labels
        for container in ax.containers:
            ax.bar_label(container, fmt='%.0f', fontsize=8, rotation=90, padding=3)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def generate_latency_chart(df: pd.DataFrame, output_path: Path):
    """Generate latency comparison chart."""
    if not HAS_MATPLOTLIB or df.empty:
        return
    
    normal_df = df[df['test_type'].isin(['low_normal', 'high_normal'])]
    if normal_df.empty:
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    for idx, (test_type, title) in enumerate([
        ('low_normal', 'Low Connections (100)'),
        ('high_normal', 'High Connections (512)')
    ]):
        ax = axes[idx]
        subset = normal_df[normal_df['test_type'] == test_type]
        
        if subset.empty:
            continue
        
        pivot = subset.pivot_table(
            values='latency_us',
            index='framework',
            columns='os',
            aggfunc='mean'
        )
        
        pivot.plot(kind='bar', ax=ax, width=0.8, color=['#3498db', '#e74c3c'])
        ax.set_title(f'Latency - {title}')
        ax.set_ylabel('Latency (μs)')
        ax.set_xlabel('Framework')
        ax.legend(title='OS')
        ax.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def generate_memory_chart(df: pd.DataFrame, output_path: Path):
    """Generate memory usage comparison chart."""
    if not HAS_MATPLOTLIB or df.empty:
        return
    
    normal_df = df[df['test_type'] == 'high_normal']
    if normal_df.empty:
        return
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    pivot = normal_df.pivot_table(
        values='mem_peak_kb',
        index='framework',
        columns='os',
        aggfunc='mean'
    )
    
    pivot.plot(kind='bar', ax=ax, width=0.8, color=['#2ecc71', '#9b59b6'])
    ax.set_title('Peak Memory Usage (High Connections)')
    ax.set_ylabel('Memory (KB)')
    ax.set_xlabel('Framework')
    ax.legend(title='OS')
    ax.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


def generate_stressed_chart(df: pd.DataFrame, output_path: Path):
    """Generate stressed network comparison chart."""
    if not HAS_MATPLOTLIB or df.empty:
        return
    
    stressed_df = df[df['test_type'].isin(['low_stressed', 'high_stressed'])]
    if stressed_df.empty:
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    for idx, (test_type, title) in enumerate([
        ('low_stressed', 'Low Connections + Network Stress'),
        ('high_stressed', 'High Connections + Network Stress')
    ]):
        ax = axes[idx]
        subset = stressed_df[stressed_df['test_type'] == test_type]
        
        if subset.empty:
            continue
        
        pivot = subset.pivot_table(
            values='throughput_req_s',
            index='framework',
            columns='os',
            aggfunc='mean'
        )
        
        pivot.plot(kind='bar', ax=ax, width=0.8, color=['#f39c12', '#1abc9c'])
        ax.set_title(f'Throughput - {title}')
        ax.set_ylabel('Requests/second')
        ax.set_xlabel('Framework')
        ax.legend(title='OS')
        ax.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()


# =============================================================================
# Report Generation
# =============================================================================

def generate_markdown_report(
    df: pd.DataFrame,
    winners: Dict[str, Dict[str, str]],
    benchmark_runs: str,
    test_duration: str,
    output_path: Path
):
    """Generate markdown summary report."""
    
    lines = [
        "# 📊 Comprehensive Benchmark Results",
        "",
        f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S UTC')}",
        f"**Benchmark runs per test:** {benchmark_runs}",
        f"**Test duration:** {test_duration}s",
        "",
        "## 🏆 Winners by Category",
        "",
    ]
    
    # Winners table
    if winners:
        lines.extend([
            "| Test Type | Best Throughput | Lowest Latency | Lowest Memory |",
            "|-----------|-----------------|----------------|---------------|",
        ])
        
        test_type_names = {
            'low_normal': '🟢 Low Connections',
            'high_normal': '🔴 High Connections',
            'low_stressed': '🌐 Low + Network Stress',
            'high_stressed': '🌐 High + Network Stress',
        }
        
        for test_type, metrics in winners.items():
            name = test_type_names.get(test_type, test_type)
            tp = metrics.get('throughput', 'N/A')
            lat = metrics.get('latency', 'N/A')
            mem = metrics.get('memory', 'N/A')
            lines.append(f"| {name} | {tp} | {lat} | {mem} |")
        
        lines.append("")
    
    # Detailed results by OS
    if HAS_PANDAS and not df.empty:
        for os_name in df['os'].unique():
            os_label = "🐧 Linux" if 'ubuntu' in os_name.lower() else "🪟 Windows"
            lines.extend([
                f"## {os_label} Results",
                "",
            ])
            
            os_df = df[df['os'] == os_name]
            
            for test_type in ['low_normal', 'high_normal', 'low_stressed', 'high_stressed']:
                test_df = os_df[os_df['test_type'] == test_type]
                if test_df.empty:
                    continue
                
                test_name = {
                    'low_normal': 'Normal Network - Low Connections (100)',
                    'high_normal': 'Normal Network - High Connections (512)',
                    'low_stressed': 'Stressed Network - Low Connections (100)',
                    'high_stressed': 'Stressed Network - High Connections (512)',
                }.get(test_type, test_type)
                
                lines.extend([
                    f"### {test_name}",
                    "",
                    "| Framework | Throughput (req/s) | Latency (μs) | Memory Peak (KB) | CPU (%) |",
                    "|-----------|-------------------|--------------|------------------|---------|",
                ])
                
                for _, row in test_df.iterrows():
                    lines.append(
                        f"| {row['framework']} | "
                        f"{row['throughput_req_s']:,.0f} | "
                        f"{row['latency_us']:,.2f} | "
                        f"{row['mem_peak_kb']:,.0f} | "
                        f"{row['cpu_percent']:.1f} |"
                    )
                
                lines.append("")
    
    # Charts section
    lines.extend([
        "## 📈 Charts",
        "",
        "Charts are available in the artifacts:",
        "- `throughput.png` - Throughput comparison",
        "- `latency.png` - Latency comparison",
        "- `memory.png` - Memory usage comparison",
        "- `stressed.png` - Stressed network comparison",
        "",
        "---",
        "*Full raw data available in the benchmark-final-report artifact.*",
    ])
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


def generate_csv_report(df: pd.DataFrame, output_path: Path):
    """Generate combined CSV report."""
    if HAS_PANDAS and not df.empty:
        df.to_csv(output_path, index=False)


# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description='Aggregate benchmark results')
    parser.add_argument('--input-dir', type=Path, required=True,
                        help='Directory containing benchmark artifacts')
    parser.add_argument('--output-dir', type=Path, required=True,
                        help='Output directory for reports')
    parser.add_argument('--benchmark-runs', type=str, default='5',
                        help='Number of benchmark runs')
    parser.add_argument('--test-duration', type=str, default='30',
                        help='Test duration in seconds')
    
    args = parser.parse_args()
    
    # Create output directory
    args.output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Loading results from: {args.input_dir}")
    
    # Load all results
    results = load_all_results(args.input_dir)
    
    if not results:
        print("No results found!")
        # Create empty report
        with open(args.output_dir / 'summary.md', 'w') as f:
            f.write("# 📊 Benchmark Results\n\nNo benchmark results were collected.\n")
        return
    
    print(f"Found results for {len(results)} frameworks")
    
    # Create DataFrame
    df = aggregate_by_framework_and_os(results)
    
    if df is None or df.empty:
        print("Could not create DataFrame")
        return
    
    print(f"Total data points: {len(df)}")
    
    # Find winners
    winners = find_winners(df)
    
    # Generate charts
    if HAS_MATPLOTLIB:
        print("Generating charts...")
        generate_throughput_chart(df, args.output_dir / 'throughput.png')
        generate_latency_chart(df, args.output_dir / 'latency.png')
        generate_memory_chart(df, args.output_dir / 'memory.png')
        generate_stressed_chart(df, args.output_dir / 'stressed.png')
    
    # Generate reports
    print("Generating reports...")
    generate_markdown_report(
        df, winners,
        args.benchmark_runs,
        args.test_duration,
        args.output_dir / 'summary.md'
    )
    generate_csv_report(df, args.output_dir / 'combined_results.csv')
    
    # Save raw JSON for further analysis
    with open(args.output_dir / 'raw_results.json', 'w') as f:
        json.dump(results, f, indent=2, default=str)
    
    print(f"Reports saved to: {args.output_dir}")
    print("Done!")


if __name__ == '__main__':
    main()
