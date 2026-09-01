# Coordination inbox

One file per report from a machine session, named <host>-<UTC timestamp>.md, written in
addition to the cross-session message, so a coordinator session that restarted can read
what it missed. Append-only. Pull with rebase before pushing. Never merged into any
other branch.
