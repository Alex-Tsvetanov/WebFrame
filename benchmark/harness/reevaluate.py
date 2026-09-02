"""Re-judges the runs in a file under the current admission rules, into a new file.

    python3 -m benchmark.harness.reevaluate RUNS.jsonl [MORE.jsonl ...] [--out-dir DIR]

This exists for one event: a pre-declared rule changed after the data was collected.
Pacing lag stopped being an admission rule and became a recorded covariate (validity.py
says why), and every run refused under the old rule is still on disk with every field
the rules read. Nothing is re-run. The current check_run is applied to the stored
fields, and the file that comes out sits beside the original, never in its place: the
original verdict is historical evidence and stays as it was.

Each record in the output carries its verdict at the time of the run as accepted_at_run
and rejection_reasons_at_run, the recomputed verdict as accepted and rejection_reasons,
admission_rules naming the rule set that produced it, and reevaluated_from naming the
file it came from. A file that already carries accepted_at_run is refused: a verdict
re-judged twice would have lost the original.

Records are handled as the raw JSON they are, not as RunRecord. A version 3 file read
through the dataclass would come back with every version 9 field at its default and
every unknown key dropped, which would be a different record claiming to be the same
one. The schema_version each writer gave its file stays; see schema.py, version 9.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

from benchmark.harness import validity

# The reason the demoted rule used to write. No record says which rule produced which
# reason, so the one that no longer exists has to be recognised by its words.
_PACING_REASON = "behind its own schedule"


def _head(reason: str) -> str:
    # Grouped the way driver.summarise groups, minus its width limit: a dropped pacing
    # reason is worth reading whole, because the figure in it is the run's lag.
    return reason.split(";")[0].split("(")[0].strip()


def reevaluate(source: Path, out_dir: Path | None = None) -> dict[str, Any]:
    """Writes SOURCE's records, re-judged, to <out_dir>/<name> or <stem>.reevaluated.jsonl.

    Returns the summary that main() prints. Raises rather than overwriting: an output
    that already exists, or a source that was re-evaluated before, is a mistake to
    stop on, not to paper over.
    """
    # Into another directory under the same name, or beside the source under a name
    # that says what it is. The same name matters: results2tex names its campaign.*
    # keys after the file's stem, so a re-evaluated transport.jsonl in a sibling
    # directory feeds the thesis' keys and transport.reevaluated.jsonl would not.
    out = out_dir / source.name if out_dir else source.with_name(f"{source.stem}.reevaluated.jsonl")
    if out.exists():
        raise FileExistsError(f"{out} exists; a re-evaluation is never written in place")

    records: list[dict[str, Any]] = []
    malformed = 0
    with source.open(encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            try:
                records.append(json.loads(line))
            except json.JSONDecodeError:
                # What a campaign killed mid-write leaves behind. Skipped the way
                # schema.read skips it, and counted, because a run that vanished from
                # the re-evaluation must not vanish from the report of it.
                malformed += 1
    if any("accepted_at_run" in r for r in records):
        raise ValueError(f"{source} already carries accepted_at_run; not re-evaluating twice")

    summary: dict[str, Any] = {
        "file": source.name, "runs": len(records), "malformed_lines": malformed,
        "accepted_before": 0, "accepted_after": 0,
        "newly_accepted": 0, "newly_refused": 0,
        "dropped": Counter(), "still_refused": Counter(), "kept": Counter(),
    }
    for record in records:
        before = list(record.get("rejection_reasons") or [])
        accepted_before = bool(record.get("accepted", not before))
        fresh = validity.check_run(record).reasons
        # A reason the current rules reproduce is theirs to keep or drop. The pacing
        # reason is dropped by name. Anything else was not written by a rule that reads
        # stored fields (the driver's own failure strings: RunFailed, TimeoutExpired, a
        # server that would not stop) and cannot be re-derived, so the original verdict
        # on it stands, ahead of the rest, where the driver put it.
        kept = [r for r in before if r not in fresh and _PACING_REASON not in r]
        after = kept + fresh

        record["accepted_at_run"] = accepted_before
        record["rejection_reasons_at_run"] = before
        record["rejection_reasons"] = after
        record["accepted"] = not after
        record["admission_rules"] = validity.ADMISSION_RULES
        # Two path segments, never the absolute path: this file may be published and
        # the directory it was read from may not be.
        record["reevaluated_from"] = f"{source.parent.name}/{source.name}"

        summary["accepted_before"] += accepted_before
        summary["accepted_after"] += not after
        if accepted_before and after:
            summary["newly_refused"] += 1
        if not accepted_before and not after:
            summary["newly_accepted"] += 1
            summary["dropped"].update(_head(r) for r in before)
        if after:
            summary["still_refused"].update(_head(r) for r in after)
        summary["kept"].update(_head(r) for r in kept)

    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8") as handle:
        for record in records:
            handle.write(json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n")
    summary["out"] = str(out)
    return summary


def describe(summary: dict[str, Any]) -> str:
    lines = [
        f"{summary['file']}: {summary['runs']} runs; accepted "
        f"{summary['accepted_before']} -> {summary['accepted_after']}; "
        f"newly accepted {summary['newly_accepted']}; "
        f"newly refused {summary['newly_refused']}; "
        f"still refused {summary['runs'] - summary['accepted_after']}"
    ]
    if summary["malformed_lines"]:
        lines.append(f"  skipped {summary['malformed_lines']} malformed line(s)")
    for head, n in summary["dropped"].most_common():
        lines.append(f"  dropped {n:4d}  {head}")
    for head, n in summary["still_refused"].most_common():
        lines.append(f"  still   {n:4d}  {head}")
    kept = summary["kept"]
    if kept:
        for head, n in kept.most_common():
            lines.append(f"  kept    {n:4d}  {head}  (not re-derivable from stored fields; "
                         "original verdict stands)")
    else:
        lines.append("  kept from the original verdict, not re-derivable from stored fields: none")
    lines.append(f"  -> {summary['out']}")
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[1],
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("runs", nargs="+", type=Path)
    parser.add_argument("--out-dir", type=Path, default=None,
                        help="where to write; default beside each source")
    args = parser.parse_args(argv)
    print(f"rules: {validity.ADMISSION_RULES}")
    for source in args.runs:
        try:
            print(describe(reevaluate(source, args.out_dir)))
        except (FileExistsError, ValueError, OSError) as exc:
            print(f"{source.name}: refused: {exc}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
