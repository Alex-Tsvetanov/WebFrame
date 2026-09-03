"""Counts the test cases the sources define and checks the binary registered them.

    python3 test_census.py --binary PATH --source-dir DIR [--defines MACRO ...]

Why this exists
---------------
Eight of these test files wrap their whole body in a feature guard, so on a build with
that feature off the file compiles to nothing. The tests do not skip. They are not there.
Catch2 cannot report a test it was never given, so the suite prints its usual pass, the
exit status is zero, and the only trace is a case count nobody was reading.

That was measured, not supposed: on this repository an ordinary build registers 175 cases
and an HTTP/3 build registers 187, and asking an ordinary build for the [http3] tag prints
"No tests ran" and exits 0. Eleven tests had never run anywhere, in any configuration, and
nothing in any output had ever said so.

A build with a feature off *should* have fewer tests. The defect is that it says nothing.
So this does not forbid dormant tests, it accounts for them: every case the sources define
is attributed either to the running binary or to a named undefined macro, and the totals
have to agree. The count is printed on every run, passing or failing, because a number
that appears every time is what makes a drop visible; a check that only speaks when it
fails is one nobody notices has stopped working.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

# TEST_CASE_METHOD first: TEST_CASE would match its prefix.
_CASE = re.compile(r"^\s*(TEST_CASE_METHOD|TEST_CASE|SCENARIO)\s*\(")
_IFDEF = re.compile(r"^\s*#\s*if(n?)def\s+([A-Za-z_][A-Za-z0-9_]*)")
_IF_DEFINED = re.compile(r"^\s*#\s*if\s+(!?)defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*$")
_IF_ANY = re.compile(r"^\s*#\s*if\b")
_ELSE = re.compile(r"^\s*#\s*el(se|if)\b")
_ENDIF = re.compile(r"^\s*#\s*endif\b")


def scan(path: Path) -> list[tuple[str, list[tuple[str, bool]]]]:
    """Every test case in PATH, with the guards standing over it.

    A guard is (macro, wanted), where wanted is False for #ifndef. A conditional this
    cannot read — an #if with an expression in it — pushes (None, None) and makes every
    case under it unattributable rather than quietly counted as present, which would be
    the same mistake in a smaller place.
    """
    stack: list[tuple[str | None, bool | None]] = []
    found = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if _ENDIF.match(line):
            if stack:
                stack.pop()
            continue
        if _ELSE.match(line):
            # Anything after #else is under the negation, and an #elif chain is past
            # what this reads; either way the branch stops being attributable.
            if stack:
                stack[-1] = (None, None)
            continue
        m = _IFDEF.match(line)
        if m:
            stack.append((m.group(2), m.group(1) != "n"))
            continue
        m = _IF_DEFINED.match(line)
        if m:
            stack.append((m.group(2), m.group(1) != "!"))
            continue
        if _IF_ANY.match(line):
            stack.append((None, None))
            continue
        if _CASE.match(line) and not line.lstrip().startswith(("//", "*")):
            found.append((path.name, [g for g in stack if g[0] is not None]
                          if all(g[0] is not None for g in stack) else None))
    return found


def registered(binary: Path) -> int:
    out = subprocess.run([str(binary), "--list-tests"], capture_output=True,
                         text=True, timeout=120, check=False)
    if out.returncode != 0:
        raise SystemExit(f"census: {binary} --list-tests failed: {out.returncode}")
    # Catch2 indents each test name by two spaces and its tags by six.
    return sum(1 for line in out.stdout.splitlines()
               if line.startswith("  ") and not line.startswith("   "))


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--binary", type=Path, required=True)
    ap.add_argument("--source-dir", type=Path, required=True)
    ap.add_argument("--defines", nargs="*", default=[])
    args = ap.parse_args(argv)

    # CMake hands a target property through add_test as one semicolon-joined argument
    # rather than as a list, so both shapes are accepted; a definition may also carry a
    # value, and it is the name that reaches an #ifdef.
    defined = {item.split("=")[0]
               for argument in args.defines
               for item in argument.split(";") if item}
    cases = []
    for source in sorted(args.source_dir.glob("*.cpp")):
        cases.extend(scan(source))

    active, dormant, unreadable = [], {}, []
    for name, guards in cases:
        if guards is None:
            unreadable.append(name)
            continue
        missing = [macro for macro, wanted in guards if (macro in defined) != wanted]
        if missing:
            dormant.setdefault(missing[0], []).append(name)
        else:
            active.append(name)

    have = registered(args.binary)
    print(f"test census: {have} registered, {len(active)} expected from "
          f"{len(cases)} defined in {args.source_dir.name}/")
    for macro, names in sorted(dormant.items()):
        files = ", ".join(sorted(set(names)))
        print(f"  {len(names):4d} dormant: {macro} is not defined ({files})")
    if unreadable:
        print(f"  {len(unreadable):4d} under a conditional this cannot read "
              f"({', '.join(sorted(set(unreadable)))}); not counted either way")

    if have != len(active):
        # Both directions are failures. Fewer means tests were lost with no macro to
        # account for them, which is the thing this exists to catch. More means the
        # accounting is wrong, and an accounting nobody can trust is not a safeguard.
        print(f"census FAILED: {have} registered against {len(active)} expected; "
              f"{'tests vanished with nothing to explain them' if have < len(active) else 'more ran than the sources account for'}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
