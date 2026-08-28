"""Checks for the statistics and the LaTeX emission.

Imported and run by selfcheck.py.
"""

from __future__ import annotations

import statistics
import tempfile
from pathlib import Path
from typing import Any, Callable

from benchmark.harness import results2tex, schema, stats


def _run(**kw: Any) -> schema.RunRecord:
    base = dict(system="coroute", protocol="http1.1", io_backend="io_uring",
                workers=6, connections=256, duration_s=30.0, accepted=True)
    base.update(kw)
    return schema.RunRecord(**base)


def run(check: Callable[[str, bool], None]) -> None:
    print("\n== bootstrap intervals, in the standard library ==")

    # A tight sample should give a tight interval containing the median.
    tight = [1000.0, 1010.0, 995.0, 1005.0, 1002.0, 998.0, 1001.0]
    interval = stats.bca(tight, resamples=2000, seed=1)
    check("the point estimate is the median", interval.point == statistics.median(tight))
    check("the interval brackets the estimate",
          interval.low <= interval.point <= interval.high)
    check("it is reported as BCa", interval.method == "BCa")

    # Two runs cannot support an interval, and inventing one would look like evidence.
    thin = stats.bca([100.0, 200.0], resamples=500, seed=1)
    check("fewer than three runs get no interval", not thin.has_interval)
    check("and say why", "n<3" in thin.method)

    # A figure that changes between two builds of the same document from the same data
    # is a figure nobody can check.
    again = stats.bca(tight, resamples=2000, seed=1)
    check("the same seed gives the same interval",
          (again.low, again.high) == (interval.low, interval.high))

    # Robustness is the reason for medians. One thermally unlucky run should move the
    # estimate very little, and the plan refuses to trim, so the statistic has to absorb it.
    with_outlier = tight + [50_000.0]
    shifted = stats.bca(with_outlier, resamples=2000, seed=1)
    check("one wild run barely moves the median",
          abs(shifted.point - interval.point) < 10.0)

    all_same = stats.bca([500.0] * 8, resamples=500, seed=1)
    check("identical runs do not crash the correction", all_same.point == 500.0)

    print("\n== a difference has to clear two bars ==")

    baseline = [1000.0, 1005.0, 995.0, 1002.0, 998.0, 1001.0]
    # Statistically clean but tiny. With enough runs a 0.4% difference becomes
    # significant and remains uninteresting.
    tiny = [1004.0, 1009.0, 999.0, 1006.0, 1002.0, 1005.0]
    verdict = stats.compare(baseline, tiny, resamples=2000, seed=1)
    check("a difference under 5% is not reportable", not verdict.reportable)
    check("and the reason names the floor", "5%" in verdict.reason)

    # Large and consistent.
    large = [1400.0, 1410.0, 1395.0, 1405.0, 1402.0, 1398.0]
    verdict = stats.compare(baseline, large, resamples=2000, seed=1)
    check("a large consistent difference is reportable", verdict.reportable)
    check("the relative size is computed", abs(verdict.relative - 0.4) < 0.02)

    # Large on average but wildly variable: the interval should contain zero.
    noisy = [400.0, 2000.0, 600.0, 1800.0, 500.0, 1900.0]
    verdict = stats.compare(baseline, noisy, resamples=2000, seed=1)
    check("a large but unreliable difference is not reportable", not verdict.reportable)
    check("because the interval contains zero", "contains zero" in verdict.reason)

    print("\n== keys are names, and an ambiguous one is refused ==")

    clean = [
        _run(requests_per_second=1000.0 + i, latency_ms={"p50": 1.0, "p99": 4.0 + i})
        for i in range(5)
    ]
    keys = results2tex.aggregate(clean, resamples=500)
    check("a key is built from the factors it names", "coroute.h1.rps" in keys)
    check("latency percentiles get keys", "coroute.h1.p99" in keys)
    check("n is emitted beside the figures", keys["coroute.h1.n"].interval.point == 5.0)

    # The property that matters. Runs at two backlogs both claim coroute.h1.rps, and
    # emitting either would make the sentence mean "averaged over a factor I forgot to
    # mention".
    mixed = clean + [_run(backlog=16384, requests_per_second=2000.0) for _ in range(3)]
    message = ""
    try:
        results2tex.aggregate(mixed, resamples=200)
        raised = False
    except results2tex.AmbiguousKey as exc:
        raised = True
        message = str(exc)
    check("a key claimed by two cells is refused", raised)
    check("the refusal names the factor that separates them", "backlog" in message)
    check("and says what to do about it", "key_factors" in message)

    # Naming the factor resolves it, and both cells then get their own key.
    resolved = results2tex.aggregate(
        mixed, key_factors=("system", "protocol", "backlog"), resamples=200)
    check("naming the factor resolves the ambiguity",
          "coroute.h1.0.rps" in resolved and "coroute.h1.16384.rps" in resolved)

    # Rejected runs must not reach the document. They are kept on disk for the
    # rejection count and excluded from every figure.
    with_rejected = clean + [_run(accepted=False, requests_per_second=99_999.0)]
    keys = results2tex.aggregate(with_rejected, resamples=500)
    check("rejected runs are excluded from the numbers",
          keys["coroute.h1.n"].interval.point == 5.0)

    print("\n== the emitted LaTeX ==")

    rendered = results2tex.render(results2tex.aggregate(clean, resamples=500))
    check("keys are defined", "\\csname res@coroute.h1.rps\\endcsname" in rendered)
    check("bounds are separate keys", "res@coroute.h1.rps.lo" in rendered)
    check("numbers are wrapped for siunitx", "\\num{" in rendered)
    # Redefined rather than defined, so keys this file does not know about keep the red
    # fallback from preamble.tex instead of rendering as nothing.
    check("R is redefined with the fallback kept", "\\renewcommand{\\R}" in rendered
          and "[?#1]" in rendered)
    check("it is marked generated", "Do not edit" in rendered)

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "runs.jsonl"
        for record in clean:
            schema.append(path, record)
        out = Path(tmp) / "generated" / "results.tex"
        aggregates = results2tex.write(schema.accepted_only(schema.read(path)), out)
        check("it writes a file", out.exists() and out.stat().st_size > 0)
        check("round-tripping through JSONL preserves the keys",
              "coroute.h1.rps" in aggregates)
