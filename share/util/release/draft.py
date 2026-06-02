#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Create a draft GitHub release for a tagged release."""

from __future__ import annotations

import sys
from subprocess import run

from _common import load_release_notes


def create_draft_release(tag: str, release_notes: str) -> None:
    release_tag = tag.split("-rc")[0]
    run(
        ["gh", "release", "create", tag, "--draft", "--title", release_tag, "-F", "-"],
        input=release_notes,
        text=True,
        check=True,
    )


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: draft.py <tag>   e.g. draft.py v3.4.7", file=sys.stderr)
        sys.exit(1)
    tag = sys.argv[1]
    _release_date, release_notes, _release_version = load_release_notes(tag)
    create_draft_release(tag, release_notes)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)

