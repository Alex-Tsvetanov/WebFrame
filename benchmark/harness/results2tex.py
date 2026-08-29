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
    "fwd-in": "quic_forwarded_in",
    "fwd-out": "quic_forwarded_out",
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
    # No underscores anywhere in a key. Prose writes \R{key}, and an underscore there
    # has to be escaped as \_, which is not expandable inside the \csname that \R
    # builds: the document stops with "Missing \endcsname inserted" rather than with a
    # red marker. A hyphen expands to itself and needs no escape.
    factor = factor.replace("_", "-")
    if factor == "protocol":
        return _short_protocol(str(value))
    if isinstance(value, bool):
        return f"{factor}-{'on' if value else 'off'}"
    # An integral float renders as an integer before the dot is stripped. Otherwise an
    # offered rate of 40000.0 becomes the key part "400000", which reads as four hundred
    # thousand: a key that silently misnames its own cell, which is the failure this
    # module exists to prevent.
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
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
    "n": 0, "errors": 0, "fwd-in": 0, "fwd-out": 0, "rx": 0,
    "rps": 0, "bps": 0, "mem": 0,
    "cpu": 2,
    "p50": 2, "p90": 2, "p99": 2, "p999": 2,
    # Admissibility signals. Pacing lag is a whole number of microseconds against a
    # limit of a thousand; the two fractions are quoted to four places because the
    # interesting part of an achieved share of 0.9998 is the tail of it.
    "pacing": 0, "gencpu": 4, "share": 4,
    # Percentages of a median. Two places, because the difference being argued about is
    # around one percent and one place would round half the table to zero.
    "pct": 2,
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


def campaign_counts(name: str,
                    all_records: Sequence[schema.RunRecord]) -> dict[str, Aggregate]:
    """How many runs were made and how many survived admissibility.

    These belong in the key namespace with everything else. A rejection count quoted
    from memory in one chapter and recomputed in another is exactly the disagreement
    the key scheme exists to make impossible, and it is the number a reader checks
    first: a campaign that discarded a third of its runs is a different campaign.

    Named per campaign, because two campaigns with different environment fingerprints
    are two populations and the harness refuses to pool their measurements. Their run
    counts must not be pooled in the prose either.
    """
    accepted = sum(1 for r in all_records if r.accepted)
    counted: dict[str, tuple[float, str]] = {
        f"campaign.{name}.runs": (len(all_records), "n"),
        f"campaign.{name}.accepted": (accepted, "n"),
        f"campaign.{name}.rejected": (len(all_records) - accepted, "n"),
        f"campaign.{name}.non2xx": (sum(r.requests_non_2xx for r in all_records), "n"),
        f"campaign.{name}.socket-errors": (sum(r.socket_errors for r in all_records), "n"),
    }

    # The worst value each admissibility signal reached. A campaign that rejected nothing
    # has to say how close it came, or "zero rejections" reads as "the gates were loose"
    # rather than as "the runs were clean".
    for label, field, worst in (("pacing", "generator_pacing_p99_us", max),
                                ("gencpu", "generator_cpu_fraction", max)):
        values = [getattr(r, field) for r in all_records if getattr(r, field) is not None]
        if values:
            counted[f"campaign.{name}.{label}-worst"] = (worst(values), label)
    shares = [r.generator_achieved_share for r in all_records
              if r.generator_achieved_share is not None]
    if shares:
        counted[f"campaign.{name}.share-worst"] = (min(shares), "share")

    return {
        key: Aggregate(key, metric,
                       stats.Interval(point=float(value), low=None, high=None,
                                      n=len(all_records), method="count"))
        for key, (value, metric) in counted.items()
    }


def hypothesis_x1(name: str, all_records: Sequence[schema.RunRecord],
                  percentile: str = "p99") -> dict[str, Aggregate]:
    """The verdict on X1 as keys, not as sentences.

    The relative difference and the resolution are what the conclusion of chapter VI
    and the same paragraph of the abstract both quote. Two documents quoting one
    measurement from memory is the disagreement the key namespace exists to prevent,
    and it is worse in an abstract than anywhere else, because the abstract is what
    gets read.

    Resolution is the half-width of the bootstrapped difference interval as a fraction
    of the baseline median. It answers "how small a difference could this design have
    seen", which is the question a non-rejection has to answer to mean anything.
    """
    cells: dict[Any, tuple[list[float], list[float]]] = {}
    for record in all_records:
        if not record.accepted or percentile not in record.latency_ms:
            continue
        arms = cells.setdefault(record.offered_rate, ([], []))
        arms[1 if record.protocol_detection else 0].append(record.latency_ms[percentile])

    relatives: list[float] = []
    resolutions: list[float] = []
    excluding_zero = 0
    for off, on in cells.values():
        if len(off) < 3 or len(on) < 3:
            continue
        c = stats.compare(off, on)
        relatives.append(100.0 * c.relative)
        if c.interval.low is not None and c.baseline:
            resolutions.append(100.0 * (c.interval.high - c.interval.low) / 2 / c.baseline)
        if c.interval.excludes(0.0):
            excluding_zero += 1

    if not relatives:
        return {}
    values = {
        f"h1detect.{name}.cells": (float(len(relatives)), "n"),
        f"h1detect.{name}.rel-min": (min(relatives), "pct"),
        f"h1detect.{name}.rel-max": (max(relatives), "pct"),
        f"h1detect.{name}.rel-abs-max": (max(abs(r) for r in relatives), "pct"),
        f"h1detect.{name}.excludes-zero": (float(excluding_zero), "n"),
    }
    if resolutions:
        values[f"h1detect.{name}.resolution-min"] = (min(resolutions), "pct")
        values[f"h1detect.{name}.resolution-max"] = (max(resolutions), "pct")
    return {
        key: Aggregate(key, metric,
                       stats.Interval(point=value, low=None, high=None,
                                      n=len(relatives), method="derived"))
        for key, (value, metric) in values.items()
    }


def tail_modes(name: str, all_records: Sequence[schema.RunRecord],
               threshold_ms: float = 1.0) -> dict[str, Aggregate]:
    """How many runs land in the upper mode of a bimodal p99.9.

    Reported because the split is the reason p99.9 is not used for comparison on this
    host. A percentile that is not used has to say how it was ruled out.
    """
    values = [r.latency_ms["p999"] for r in all_records
              if r.accepted and "p999" in r.latency_ms]
    if not values:
        return {}
    upper = sum(1 for v in values if v > threshold_ms)
    counted = {f"tail.{name}.runs": len(values), f"tail.{name}.upper-mode": upper}
    return {
        key: Aggregate(key, "n", stats.Interval(point=float(value), low=None, high=None,
                                                n=len(values), method="derived"))
        for key, value in counted.items()
    }


def write(records: Iterable[schema.RunRecord], out: Path,
          key_factors: Sequence[str] = DEFAULT_KEY_FACTORS,
          extra: dict[str, Aggregate] | None = None) -> dict[str, Aggregate]:
    aggregates = aggregate(records, key_factors)
    aggregates.update(extra or {})
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(render(aggregates), encoding="utf-8")
    return aggregates


def main(argv: Sequence[str]) -> int:
    if len(argv) < 3:
        print(__doc__)
        return 2
    runs = Path(argv[1])
    out = Path(argv[2])
    # Further jsonl files contribute their run and rejection counts and nothing else.
    # Their measurements stay out, because a different environment fingerprint means a
    # different population and merging the two would hide that behind a larger n.
    also = [Path(a) for a in argv[3:] if a.endswith(".jsonl")]
    rest = [a for a in argv[3:] if not a.endswith(".jsonl")]
    key_factors = tuple(rest[0].split(",")) if rest else DEFAULT_KEY_FACTORS

    records = list(schema.read(runs))
    accepted = schema.accepted_only(iter(records))
    print(f"{len(records)} runs, {len(accepted)} accepted")
    if not accepted:
        print("nothing to emit; every run was rejected")
        return 1

    counts = campaign_counts(runs.stem, records)
    counts.update(hypothesis_x1(runs.stem, records))
    counts.update(tail_modes(runs.stem, records))
    for path in also:
        other = list(schema.read(path))
        counts.update(campaign_counts(path.stem, other))
        counts.update(hypothesis_x1(path.stem, other))
        counts.update(tail_modes(path.stem, other))
        print(f"{len(other)} runs from {path.name}, counts and derived keys only")

    aggregates = write(accepted, out, key_factors, extra=counts)
    print(f"wrote {len(aggregates)} keys to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
