"""The order runs happen in, and why it is not the obvious one.

The obvious order is all of system A, then all of system B. It is also the one order
guaranteed to be wrong. A machine warms up: fans spin up, clocks settle down, the page
cache fills. Measuring A for an hour and then B for an hour means every difference
between the first hour and the second is attributed to the difference between A and B,
and there is no way afterwards to tell the two apart.

So the design is walked in passes. Each repetition visits every cell once, systems
interleaved, and the order within a pass is shuffled. Drift still happens, but it now
falls across all systems roughly equally instead of landing on whichever was measured
last, and the repetition index makes it visible: if pass 5 is slower than pass 1 for
everything, that is thermal, and it can be seen rather than inferred.

Shuffling is seeded. A campaign that cannot be re-run in the same order is not
reproducible, and "randomised" is not an excuse for unrepeatable.
"""

from __future__ import annotations

import random
from dataclasses import dataclass
from typing import Any, Iterable, Sequence


@dataclass(frozen=True)
class Cell:
    """One point in the experimental design: a system and the factors it runs under."""

    system: str
    factors: tuple[tuple[str, Any], ...]

    @classmethod
    def of(cls, system: str, **factors: Any) -> Cell:
        # Sorted so two cells built with the same factors in a different keyword order
        # are the same cell. Otherwise the design would silently contain duplicates.
        return cls(system=system, factors=tuple(sorted(factors.items())))

    def as_dict(self) -> dict[str, Any]:
        return {"system": self.system, **dict(self.factors)}


@dataclass(frozen=True)
class ScheduledRun:
    repetition: int
    index_in_repetition: int
    cell: Cell


def plan(cells: Sequence[Cell], repetitions: int, seed: int) -> list[ScheduledRun]:
    """The full run order for a campaign.

    Every cell appears exactly `repetitions` times. Within a repetition the order is
    shuffled, and each repetition is shuffled differently, so no cell keeps the same
    slot across passes. A cell that were always measured first would always be measured
    on the coldest machine.
    """
    if repetitions < 1:
        raise ValueError("a campaign needs at least one repetition")
    if not cells:
        raise ValueError("a campaign needs at least one cell")

    unique = list(dict.fromkeys(cells))
    if len(unique) != len(cells):
        raise ValueError(
            f"the design contains {len(cells) - len(unique)} duplicate cell(s); "
            "a repeated cell is a repetition, and repetitions are counted separately"
        )

    out: list[ScheduledRun] = []
    for repetition in range(repetitions):
        # Derived from the campaign seed rather than reseeded arbitrarily, so the whole
        # order follows from one number that can be recorded and re-used.
        #
        # Seeded from a string on purpose. Seeding with the integer `seed + repetition`
        # would make campaign 1 pass 2 identical to campaign 2 pass 1, which is a
        # collision between campaigns that are supposed to be independent.
        rng = random.Random(f"{seed}:{repetition}")
        order = list(unique)
        rng.shuffle(order)
        for position, cell in enumerate(order):
            out.append(ScheduledRun(repetition=repetition, index_in_repetition=position, cell=cell))
    return out


def systems_in(cells: Iterable[Cell]) -> list[str]:
    return list(dict.fromkeys(cell.system for cell in cells))


def position_summary(schedule: Sequence[ScheduledRun]) -> dict[str, float]:
    """Mean position in a pass, per system.

    A diagnostic rather than a control. If one system's mean position is much lower than
    another's, it is systematically measured on a colder machine, and the interleaving
    has not done its job. With enough repetitions these converge on the same value.
    """
    totals: dict[str, list[int]] = {}
    for run in schedule:
        totals.setdefault(run.cell.system, []).append(run.index_in_repetition)
    return {system: sum(values) / len(values) for system, values in totals.items()}
