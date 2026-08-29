"""Accepted runs to the CSV files the figures and tables in chapter VI read.

Two tools rather than one, because the thesis needs numbers in two shapes and only one
of them belongs in a macro.

    results2tex   scalars quoted inside a sentence. Those are the ones that rot, because
                  a sentence is where a stale number looks most authoritative.

    results2csv   whole tables and whole plots. pgfplotstable typesets a CSV directly and
                  pgfplots draws one directly, so neither needs a number to pass through
                  a person on its way into the document.

Every column carries its interval, not just its point. A median without one is a claim
about a single number rather than about a measurement, and the difference matters most
exactly where the intervals overlap.

    python -m benchmark.harness.results2csv runs.jsonl doc/thesis/data
"""

from __future__ import annotations

import csv
import sys
from pathlib import Path
from typing import Any, Callable, Iterable, Sequence

from benchmark.harness import schema, stats


# What each table varies and what it holds fixed is written out at the bottom rather
# than inferred. A table whose held-fixed factors are implicit is a table that silently
# averages over something, which is the defect results2tex raises AmbiguousKey for.


def _interval(values: Sequence[float]) -> tuple[float, float | None, float | None]:
    if not values:
        return (float("nan"), None, None)
    i = stats.bca(list(values), confidence=0.95, resamples=10_000)
    return (i.point, i.low, i.high)


def _rows_for(records: list[schema.RunRecord], vary: str,
              fixed: dict[str, Any], series: str | None) -> list[dict[str, Any]]:
    """One row per value of `vary`, one column group per value of `series`."""
    def matches(r: schema.RunRecord) -> bool:
        return all(getattr(r, k) == v for k, v in fixed.items())

    selected = [r for r in records if matches(r)]
    if not selected:
        return []

    vary_values = sorted({getattr(r, vary) for r in selected})
    series_values = sorted({getattr(r, series) for r in selected}) if series else [None]

    rows: list[dict[str, Any]] = []
    for v in vary_values:
        row: dict[str, Any] = {vary: v}
        for s in series_values:
            members = [r for r in selected
                       if getattr(r, vary) == v and (series is None or getattr(r, series) == s)]
            if not members:
                continue
            tag = "" if s is None else f"_{_tag(s)}"
            row[f"n{tag}"] = len(members)

            rps = [r.requests_per_second for r in members]
            point, low, high = _interval(rps)
            row[f"rps{tag}"] = round(point)
            row[f"rps_lo{tag}"] = round(low) if low is not None else ""
            row[f"rps_hi{tag}"] = round(high) if high is not None else ""

            for pct in ("p50", "p99", "p999"):
                vals = [r.latency_ms[pct] for r in members if pct in r.latency_ms]
                point, low, high = _interval(vals)
                row[f"{pct}{tag}"] = round(point, 3)
                row[f"{pct}_lo{tag}"] = round(low, 3) if low is not None else ""
                row[f"{pct}_hi{tag}"] = round(high, 3) if high is not None else ""

            cpu = [r.server_cpu_seconds for r in members if r.server_cpu_seconds is not None]
            if cpu:
                row[f"cpu{tag}"] = round(_interval(cpu)[0], 2)
        rows.append(row)
    return rows


def _tag(value: Any) -> str:
    if isinstance(value, bool):
        return "on" if value else "off"
    return str(value).replace(".", "").replace("_", "")


def _write(path: Path, rows: list[dict[str, Any]]) -> int:
    if not rows:
        return 0
    columns: list[str] = []
    for row in rows:
        for key in row:
            if key not in columns:
                columns.append(key)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=columns, restval="")
        writer.writeheader()
        writer.writerows(rows)
    return len(rows)


def main(argv: Sequence[str]) -> int:
    if len(argv) < 3:
        print(__doc__)
        return 2

    runs = Path(argv[1])
    out_dir = Path(argv[2])

    all_records = list(schema.read(runs))
    accepted = schema.accepted_only(iter(all_records))
    print(f"{len(all_records)} runs, {len(accepted)} accepted")
    if not accepted:
        print("nothing to emit; every run was rejected")
        return 1

    base_workers = 4
    base_payload = 0
    base_backlog = 1024
    base_rate = 40000.0

    tables: dict[str, tuple[str, dict[str, Any], str | None]] = {
        # The headline: both arms of the demultiplexing comparison across offered load.
        "h1_demux": ("offered_rate",
                     {"workers": base_workers, "payload_bytes": base_payload,
                      "backlog": base_backlog},
                     "protocol_detection"),
        # One factor at a time around the baseline, which is what the design sweeps.
        "workers": ("workers",
                    {"offered_rate": base_rate, "payload_bytes": base_payload,
                     "backlog": base_backlog, "protocol_detection": True}, None),
        "payload": ("payload_bytes",
                    {"offered_rate": base_rate, "workers": base_workers,
                     "backlog": base_backlog, "protocol_detection": True}, None),
        "backlog": ("backlog",
                    {"offered_rate": base_rate, "workers": base_workers,
                     "payload_bytes": base_payload, "protocol_detection": True}, None),
    }

    written = 0
    for name, (vary, fixed, series) in tables.items():
        rows = _rows_for(accepted, vary, fixed, series)
        count = _write(out_dir / f"{name}.csv", rows)
        print(f"  {name + '.csv':<20} {count} rows")
        written += count

    # The rejection count is part of the result, not an aside. A campaign that discarded
    # a third of its runs is a different campaign from one that discarded none.
    reasons: dict[str, int] = {}
    for record in all_records:
        for reason in record.rejection_reasons:
            head = reason.split(";")[0].strip()[:80]
            reasons[head] = reasons.get(head, 0) + 1
    rejection_rows = [{"reason": k, "count": v} for k, v in sorted(reasons.items())]
    _write(out_dir / "rejections.csv", rejection_rows)
    print(f"  {'rejections.csv':<20} {len(rejection_rows)} rows")

    print(f"\nwrote {written} data rows to {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
