"""Medians and bootstrap intervals, in the standard library.

No numpy and no scipy, which is not a hardship here. The measurement machine should
carry as little as possible, and everything below is a few dozen lines of arithmetic
that would otherwise be a dependency to install on a box whose whole point is being
unchanged for weeks.

Medians rather than means, because run-level outcomes are not symmetric: one thermally
unlucky run pulls a mean and barely moves a median, and the plan already refuses to
trim, so the summary statistic has to be the robust one instead.

BCa rather than the percentile bootstrap. The percentile interval is a line of code and
is wrong in a specific way that matters here: it assumes the bootstrap distribution is
unbiased and constant-variance, and for a median from five or ten runs it is neither.
BCa corrects for both, using the fraction of replicates below the observed value for
bias and a jackknife estimate for skew.
"""

from __future__ import annotations

import random
import statistics
from dataclasses import dataclass
from typing import Callable, Sequence


@dataclass(frozen=True)
class Interval:
    """A point estimate and the interval around it."""

    point: float
    low: float | None
    high: float | None
    n: int
    method: str

    @property
    def has_interval(self) -> bool:
        return self.low is not None and self.high is not None

    def excludes(self, value: float) -> bool:
        """Whether `value` lies outside the interval.

        Used against zero for a difference. An interval that contains zero is a result
        that has not distinguished itself from no result.
        """
        if not self.has_interval:
            return False
        return value < self.low or value > self.high


def _percentile(sorted_values: Sequence[float], fraction: float) -> float:
    """Linear-interpolated percentile of an already-sorted sequence."""
    if not sorted_values:
        raise ValueError("no values")
    if len(sorted_values) == 1:
        return sorted_values[0]
    position = fraction * (len(sorted_values) - 1)
    lower = int(position)
    upper = min(lower + 1, len(sorted_values) - 1)
    weight = position - lower
    return sorted_values[lower] * (1 - weight) + sorted_values[upper] * weight


def bca(
    values: Sequence[float],
    statistic: Callable[[Sequence[float]], float] = statistics.median,
    *,
    confidence: float = 0.95,
    resamples: int = 10_000,
    seed: int = 0,
) -> Interval:
    """Bias-corrected and accelerated bootstrap interval.

    Returns an interval without bounds when there are fewer than three values. Two runs
    do not support an interval, and inventing one from them would be the most misleading
    thing this module could do: it would look like evidence.

    Seeded, because a figure that changes between two builds of the same document from
    the same data is a figure nobody can check.
    """
    values = [float(v) for v in values]
    n = len(values)
    if n == 0:
        raise ValueError("no values")

    point = float(statistic(values))
    if n < 3:
        return Interval(point=point, low=None, high=None, n=n, method="none (n<3)")

    rng = random.Random(seed)
    replicates = sorted(
        statistic([values[rng.randrange(n)] for _ in range(n)]) for _ in range(resamples)
    )

    normal = statistics.NormalDist()

    # Bias correction: how far the bootstrap distribution sits from the observed value.
    # A median from an even number of runs is routinely off-centre, which is exactly the
    # case the percentile interval handles badly.
    below = sum(1 for r in replicates if r < point)
    if below in (0, resamples):
        # Every replicate on one side. No usable correction, and forcing one would
        # invent a bound. Falls back to percentiles and says so.
        low = _percentile(replicates, (1 - confidence) / 2)
        high = _percentile(replicates, 1 - (1 - confidence) / 2)
        return Interval(point=point, low=low, high=high, n=n, method="percentile (degenerate)")

    z0 = normal.inv_cdf(below / resamples)

    # Acceleration: leave-one-out estimates capture skew, which is what makes the
    # interval asymmetric when the underlying distribution is.
    jackknife = []
    for i in range(n):
        jackknife.append(statistic(values[:i] + values[i + 1:]))
    mean_jack = statistics.fmean(jackknife)
    deviations = [mean_jack - j for j in jackknife]
    numerator = sum(d ** 3 for d in deviations)
    denominator = 6.0 * (sum(d ** 2 for d in deviations) ** 1.5)
    acceleration = numerator / denominator if denominator else 0.0

    alpha = (1 - confidence) / 2

    def adjusted(a: float) -> float:
        z = normal.inv_cdf(a)
        denom = 1 - acceleration * (z0 + z)
        if denom == 0:
            return a
        return normal.cdf(z0 + (z0 + z) / denom)

    low_fraction = min(max(adjusted(alpha), 0.0), 1.0)
    high_fraction = min(max(adjusted(1 - alpha), 0.0), 1.0)
    if low_fraction > high_fraction:
        low_fraction, high_fraction = high_fraction, low_fraction

    return Interval(
        point=point,
        low=_percentile(replicates, low_fraction),
        high=_percentile(replicates, high_fraction),
        n=n,
        method="BCa",
    )


# A difference has to clear both bars to be worth reporting. Statistical distinguishability
# is not the same as mattering: with enough runs a 0.4 percent difference becomes
# significant and remains uninteresting.
MIN_RELATIVE_DIFFERENCE = 0.05


@dataclass(frozen=True)
class Comparison:
    baseline: float
    other: float
    relative: float
    interval: Interval
    reportable: bool
    reason: str


def compare(
    baseline: Sequence[float],
    other: Sequence[float],
    *,
    statistic: Callable[[Sequence[float]], float] = statistics.median,
    confidence: float = 0.95,
    resamples: int = 10_000,
    seed: int = 0,
) -> Comparison:
    """Whether two sets of runs differ in a way worth writing down.

    Bootstraps the difference itself rather than comparing two intervals for overlap.
    Non-overlapping intervals do imply a difference, but overlapping ones do not imply
    its absence, and the overlap test is the more conservative of the two for no
    principled reason.
    """
    base_point = float(statistic(list(baseline)))
    other_point = float(statistic(list(other)))
    relative = (other_point - base_point) / base_point if base_point else 0.0

    rng = random.Random(seed)
    a, b = list(baseline), list(other)
    if len(a) < 3 or len(b) < 3:
        interval = Interval(point=other_point - base_point, low=None, high=None,
                            n=min(len(a), len(b)), method="none (n<3)")
        return Comparison(base_point, other_point, relative, interval, False,
                          "too few runs to support an interval")

    differences = sorted(
        statistic([b[rng.randrange(len(b))] for _ in range(len(b))])
        - statistic([a[rng.randrange(len(a))] for _ in range(len(a))])
        for _ in range(resamples)
    )
    alpha = (1 - confidence) / 2
    interval = Interval(
        point=other_point - base_point,
        low=_percentile(differences, alpha),
        high=_percentile(differences, 1 - alpha),
        n=min(len(a), len(b)),
        method="bootstrap difference",
    )

    excludes_zero = interval.excludes(0.0)
    large_enough = abs(relative) >= MIN_RELATIVE_DIFFERENCE

    if not excludes_zero and not large_enough:
        reason = "the interval contains zero and the difference is under 5%"
    elif not excludes_zero:
        reason = "the interval contains zero"
    elif not large_enough:
        reason = f"the difference is {abs(relative):.1%}, under the 5% floor"
    else:
        reason = f"the interval excludes zero and the difference is {abs(relative):.1%}"

    return Comparison(base_point, other_point, relative, interval,
                      excludes_zero and large_enough, reason)
