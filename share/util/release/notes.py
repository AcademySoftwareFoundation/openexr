#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Extract release notes from CHANGES.md for a tagged release and print to stdout."""

from __future__ import annotations

import sys

from _common import load_release_notes


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: notes.py <tag>   e.g. notes.py v3.4.7", file=sys.stderr)
        sys.exit(1)
    _release_date, release_notes, _release_version = load_release_notes(sys.argv[1])
    print(release_notes)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)

