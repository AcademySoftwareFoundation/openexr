#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""List merged PRs with a GitHub label; print cherry-pick commands (oldest merge first).

By default prints one combined ``git cherry-pick`` line plus a ``changes.py`` line.
With ``--single``, prints one ``git cherry-pick`` command per PR instead.
"""

from __future__ import annotations

import json
import sys
from subprocess import PIPE, run


def main() -> None:
    args = sys.argv[1:]
    single = False
    if "--single" in args:
        single = True
        args.remove("--single")
    if len(args) != 1:
        print(
            "Usage: cherry.py [--single] <label>\n"
            "  --single   print one git cherry-pick command per PR",
            file=sys.stderr,
        )
        sys.exit(1)
    label = args[0]

    result = run(
        [
            "gh",
            "pr",
            "list",
            "--label",
            label,
            "--state",
            "merged",
            "--limit",
            "500",
            "--json",
            "mergedAt,mergeCommit,title,number",
        ],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "gh pr list failed\n")
        sys.exit(1)

    try:
        prs = json.loads(result.stdout or "[]")
    except json.JSONDecodeError as exc:
        print(f"Invalid JSON from gh: {exc}", file=sys.stderr)
        sys.exit(1)

    merged = []
    for pr in prs:
        pr_id = pr.get("number") or "?"
        mc = pr.get("mergeCommit") or {}
        oid = mc.get("oid")
        merged_at = pr.get("mergedAt")
        title = pr.get("title") or ""
        if not oid or not merged_at:
            continue
        merged.append((merged_at, oid, title, pr_id))

    # sort by merged_at
    merged.sort(key=lambda x: x[0])

    if not merged:
        dym = f"; did you mean v{label}?" if not label.startswith('v') else ""
        print(f"No PRs with label {label}{dym}")
        return

    if single:
        for _merged_at, oid, title, pr_id in merged:
            abbrev = oid[:7]
            title_one_line = " ".join(title.split())
            print(f"git cherry-pick {abbrev} # {title_one_line} (#{pr_id})")
    else:
        print(f"git cherry-pick {' '.join(o[:7] for a, o, t, i in merged)}")

    pr_ids = " ".join(str(i) for a, o, t, i in merged)
    print(f"python share/util/release/changes.py {label} {pr_ids}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)

