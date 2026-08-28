"""Accepted runs to the numbers the thesis quotes.

The rule is that no number is typed into prose twice. A sentence says \\R{coroute.h1.rps}
and this file defines what that means, so re-running the campaign updates the sentence
and a measurement that does not exist prints a red, greppable [?key] instead of a
plausible figure nobody checked.

Two properties matter more than the arithmetic.

Keys are names, not positions. The previous aggregator recovered identity by parsing
filenames and produced tables addressed by row offset, so inserting a row silently
changed what a sentence claimed. A key here is built from the factors it names.

A key that two cells both claim is refused. If runs at backlog 128 and backlog 16384
both want to be coroute.h1.rps, emitting either would make the sentence mean "averaged
over a factor I forgot to mention". The aggregator raises and says which factor
separates them, so the fix is to name it in the key rather than to discover the problem
during a defence.

    python3 -m benchmark.harness.results2tex runs.jsonl doc/thesis/generated/results.tex
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Sequence

from benchmark.harness import schema, stats


# Factors that name a cell by default. Anything else varying within a key is an
# ambiguity the caller has to resolve by adding it here.
DEFAULT_KEY_FACTORS = ("system", "protocol")

# What gets a key, and how it is read out of a record. Latency percentiles come from
# the summary dict rather than being separate fields, so the set is data rather than
# code and adding p999 costs a line.
METRICS: dict[str, str] = {
    "rps": "requests_per_second",
    "bps": "bytes_per_second",
    "errors": "requests_non_2xx",
    "cpu": "server_cpu_seconds",
    "mem": "server_memory_peak_bytes",
    "fwd_in": "quic_forwarded_in",
    "fwd_out": "quic_forwarded_out",
    "rx": "quic_datagrams_received",
}

LATENCY_KEYS = ("p50", "p90", "p99", "p999")


class AmbiguousKey(RuntimeError):
    """Two different cells want the same key."""

    def __init__(self, key: str, differing: dict[str, set[Any]]) -> None:
        self.key = key
        self.differing = differing
        detail = "; ".join(
            f"{factor} takes {sorted(map(str, values))}" for factor, values in differing.items()
        )
        super().__init__(
            f"key {key!r} is claimed by runs that differ in: {detail}. "
            "Quoting it would mean averaging over a factor the sentence does not name. "
            "Add that factor to key_factors."
        )


@dataclass(frozen=True)
class Aggregate:
    key: str
    metric: str
    interval: stats.Interval


def _short_protocol(protocol: str) -> str:
    """h1, h2, h3 rather than http1.1, so keys stay readable in prose."""
    return {"http1.1": "h1", "http/1.1": "h1", "http2": "h2", "http3": "h3"}.get(
        protocol.lower(), protocol.lower().replace(".", "").replace("/", "")
    )


def _key_part(factor: str, value: Any) -> str:
    if factor == "protocol":
        return _short_protocol(str(value))
    if isinstance(value, bool):
        return f"{factor}-{'on' if value else 'off'}"
    return str(value).replace(".", "").replace("_", "").lower()


def cell_key(record: schema.RunRecord, key_factors: Sequence[str]) -> str:
    return ".".join(_key_part(f, getattr(record, f)) for f in key_factors)


# Factors that could plausibly separate two runs. Checked when a key is claimed twice,
# so the error can name the culprit instead of saying only that something differs.
_CANDIDATE_FACTORS = (
    "system", "protocol", "tls", "io_backend", "protocol_detection", "workers",
    "connections", "streams_per_connection", "payload_bytes", "backlog",
    "offered_rate", "netem_profile",
)


def group(
    records: Iterable[schema.RunRecord],
    key_factors: Sequence[str] = DEFAULT_KEY_FACTORS,
) -> dict[str, list[schema.RunRecord]]:
    """Groups accepted runs by key, refusing any key two cells both claim."""
    grouped: dict[str, list[schema.RunRecord]] = {}
    for record in records:
        if not record.accepted:
            continue
        grouped.setdefault(cell_key(record, key_factors), []).append(record)

    for key, members in grouped.items():
        differing = {}
        for factor in _CANDIDATE_FACTORS:
            if factor in key_factors:
                continue
            values = {getattr(m, factor) for m in members}
            if len(values) > 1:
                differing[factor] = values
        if differing:
            raise AmbiguousKey(key, differing)
    return grouped


def aggregate(
    records: Iterable[schema.RunRecord],
    key_factors: Sequence[str] = DEFAULT_KEY_FACTORS,
    *,
    confidence: float = 0.95,
    resamples: int = 10_000,
) -> dict[str, Aggregate]:
    """One interval per key and metric."""
    out: dict[str, Aggregate] = {}
    for key, members in group(records, key_factors).items():
        for suffix, attribute in METRICS.items():
            values = [getattr(m, attribute) for m in members]
            values = [float(v) for v in values if v is not None]
            if not values:
                continue
            interval = stats.bca(values, confidence=confidence, resamples=resamples)
            out[f"{key}.{suffix}"] = Aggregate(key, suffix, interval)

        for percentile in LATENCY_KEYS:
            values = [m.latency_ms.get(percentile) for m in members]
            values = [float(v) for v in values if v is not None]
            if not values:
                continue
            interval = stats.bca(values, confidence=confidence, resamples=resamples)
            out[f"{key}.{percentile}"] = Aggregate(key, percentile, interval)

        # n belongs next to every figure. A median of three runs and a median of thirty
        # are different claims, and only one of them says so on its own.
        out[f"{key}.n"] = Aggregate(
            key, "n", stats.Interval(point=float(len(members)), low=None, high=None,
                                     n=len(members), method="count"),
        )
    return out


# How many decimal places each metric deserves. Rounding happens here rather than in
# siunitx because significance depends on what the number means, and only this side
# knows: a throughput of 125250 is not usefully quoted as 125000, while a run count with
# a decimal point is simply wrong.
_DECIMALS: dict[str, int] = {
    "n": 0, "errors": 0, "fwd_in": 0, "fwd_out": 0, "rx": 0,
    "rps": 0, "bps": 0, "mem": 0,
    "cpu": 2,
    "p50": 2, "p90": 2, "p99": 2, "p999": 2,
}


def _format(value: float, metric: str) -> str:
    """Wrapped in \\num so siunitx handles grouping and the document looks uniform."""
    decimals = _DECIMALS.get(metric, 3)
    if decimals == 0:
        return f"\\num{{{int(round(value))}}}"
    return f"\\num{{{value:.{decimals}f}}}"


def render(aggregates: dict[str, Aggregate]) -> str:
    """The LaTeX that defines every key.

    Redefines \\R rather than defining it, because preamble.tex already provides the
    red-fallback version. The redefinition keeps that fallback for keys this file does
    not define, so a sentence quoting a measurement that was never taken still shows up
    in the PDF instead of quietly rendering as nothing.
    """
    lines = [
        "% Generated by benchmark/harness/results2tex.py. Do not edit.",
        "%",
        "% Every number the thesis quotes is defined here and referenced as \\R{key}.",
        "% A key with no measurement behind it renders as a red [?key], which is",
        "% greppable, rather than as a plausible number nobody checked.",
        "",
        "\\makeatletter",
        "\\renewcommand{\\R}[1]{%",
        "  \\ifcsname res@#1\\endcsname\\csname res@#1\\endcsname",
        "  \\else\\textcolor{red}{\\textbf{[?#1]}}\\fi}",
        "",
    ]
    for key in sorted(aggregates):
        entry = aggregates[key]
        lines.append(f"\\expandafter\\def\\csname res@{key}\\endcsname{{"
                     f"{_format(entry.interval.point, entry.metric)}}}")
        # Bounds are separate keys rather than a formatted range, so prose can quote the
        # figure alone where the interval would be noise and cite both where it matters.
        if entry.interval.has_interval:
            lines.append(f"\\expandafter\\def\\csname res@{key}.lo\\endcsname{{"
                         f"{_format(entry.interval.low, entry.metric)}}}")
            lines.append(f"\\expandafter\\def\\csname res@{key}.hi\\endcsname{{"
                         f"{_format(entry.interval.high, entry.metric)}}}")
    lines.append("\\makeatother")
    lines.append("")
    return "\n".join(lines)


def write(records: Iterable[schema.RunRecord], out: Path,
          key_factors: Sequence[str] = DEFAULT_KEY_FACTORS) -> dict[str, Aggregate]:
    aggregates = aggregate(records, key_factors)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(render(aggregates), encoding="utf-8")
    return aggregates


def main(argv: Sequence[str]) -> int:
    if len(argv) < 3:
        print(__doc__)
        return 2
    runs = Path(argv[1])
    out = Path(argv[2])
    key_factors = tuple(argv[3].split(",")) if len(argv) > 3 else DEFAULT_KEY_FACTORS

    records = list(schema.read(runs))
    accepted = schema.accepted_only(iter(records))
    print(f"{len(records)} runs, {len(accepted)} accepted")
    if not accepted:
        print("nothing to emit; every run was rejected")
        return 1

    aggregates = write(accepted, out, key_factors)
    print(f"wrote {len(aggregates)} keys to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
