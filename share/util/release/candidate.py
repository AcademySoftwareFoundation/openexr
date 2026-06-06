#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Format a staged-release announcement message and print to stdout."""

from __future__ import annotations

import sys
from subprocess import PIPE, run

from _common import format_weekday_month_day, load_release_notes, require_repo_url


def markdown_to_html(markdown_text: str) -> str:
    result = run(
        ["pandoc", "-f", "markdown", "-t", "html"],
        input=markdown_text,
        text=True,
        capture_output=True,
        check=True,
    )
    return (
        "<style> body { line-height: 1.0; margin: 0; padding: 0; } "
        "p, h1, h2, h3, h4, h5, h6 { margin: 0; padding: 0; } </style> "
        f"{result.stdout}"
    )


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python share/util/release/candidate.py <tag> e.g. candidate.py v3.4.7-rc", file=sys.stderr)
        sys.exit(1)
    tag = sys.argv[1]
    release_date, release_notes, release_version = load_release_notes(tag)
    html_notes = markdown_to_html(release_notes)
    date_string = format_weekday_month_day(release_date)
    url = require_repo_url()
    project = url.split("/")[-1]
    if project == "openexr":
        project = "OpenEXR"
    print(
        f"{project} {release_version} is staged for release at tag "
        f'<a href="{url}/releases/tag/{tag}">{tag}</a> and will be released '
        f"officially {date_string} barring any issues. <br><br> {html_notes}"
    )


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
