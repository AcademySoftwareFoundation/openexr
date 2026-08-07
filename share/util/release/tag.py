#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Create a signed annotated git tag using release notes from CHANGES.md."""

from __future__ import annotations

import sys
from subprocess import run

from _common import format_month_day_year, load_release_notes


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: tag.py <tag>   e.g. tag.py v3.4.7", file=sys.stderr)
        sys.exit(1)
    tag = sys.argv[1]
    release_date, release_notes, _release_version = load_release_notes(tag)
    tag_message = f"{tag} - {format_month_day_year(release_date)}\n{release_notes}\n"
    run(["git", "tag", "-s", tag, "-F", "-"], input=tag_message, text=True, check=True)
    run(["git", "tag", "-v", tag], check=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)

