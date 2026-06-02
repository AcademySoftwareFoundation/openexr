#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Print git log annotated with release labels and security refs from each commit's PR."""

from __future__ import annotations

import json
import re
import sys
from subprocess import PIPE, run

from _security import pr_addressed_cves

LOG_PR_SUFFIX_RE = re.compile(r"\(#(\d+)\)\s*$")


def pr_labels(line: str) -> tuple[str, str]:
    m = LOG_PR_SUFFIX_RE.search(line)
    if not m:
        return "", ""
    pr_number = m.group(1)
    result = run(
        ["gh", "pr", "view", pr_number, "--json", "labels"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        return pr_number, ""
    try:
        data = json.loads(result.stdout or "{}")
    except json.JSONDecodeError:
        return pr_number, ""
    out = []
    for lab in data.get("labels") or []:
        name = lab.get("name") or ""
        if name.startswith("v"):
            out.append(name)
    return pr_number, " ".join(out)


def main() -> None:
    if len(sys.argv) > 1:
        print("Usage: python share/util/release/log.py", file=sys.stderr)
        sys.exit(1)
        
    result = run(
        ["git", "log", "--pretty=format:%h %s"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "git log failed\n")
        sys.exit(1)

    for line in result.stdout.splitlines():
        pr, labels = pr_labels(line)
        pr_cves, pr_oss_fuzz = pr_addressed_cves(pr)
        refs = pr_cves + [f"oss-fuzz:{i}" for i in pr_oss_fuzz]
        suffix = f" [{' '.join(refs)}]" if refs else ""
        print(f"{labels:7} {line}{suffix}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
