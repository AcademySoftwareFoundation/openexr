#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Update website/news.rst and latest_news_title.rst for a release."""

from __future__ import annotations

import re
import sys
from datetime import datetime
from subprocess import PIPE, run

from _common import format_month_day_year, load_release_notes


def markdown_to_rst(markdown_text: str) -> str:
    markdown_text = re.sub(r"(?<!`)`([^`]+)`(?!`)", r"``\1``", markdown_text)
    markdown_text = re.sub(
        r"\[([\s\S]*?)\]\(([\s\S]*?)\)",
        r"`\1 <\2>`_",
        markdown_text,
    )
    emoji_subs = (
        (r":bug:", "🐛"),
        (r":rocket:", "🚀"),
        (r":hammer_and_wrench:", "🛠️"),
        (r":wrench:", "🔧"),
        (r":sparkles:", "✨"),
        (r":snake:", "🐍"),
        (r":package:", "📦"),
        (r":warning:", "⚠️"),
    )
    for pattern, repl in emoji_subs:
        markdown_text = re.sub(pattern, repl, markdown_text, flags=re.DOTALL)

    rst_lines = []
    for line in markdown_text.splitlines():
        if line.startswith("- "):
            line = line.replace("- ", "* ", 1)
        line = re.sub(r"\*\*(.*?)\*\*", r"**\1**", line)
        line = re.sub(r"\*(.*?)\*", r"*\1*", line)
        rst_lines.append(line)
    return "\n".join(rst_lines).strip()


def _git_show(path: str) -> str:
    result = run(
        ["git", "show", f"HEAD:{path}"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(
            result.stderr or f"git show HEAD:{path} failed\n"
        )
        sys.exit(1)
    return result.stdout


def _parse_latest_news_title(content: str) -> str:
    for line in reversed(content.splitlines()):
        if "replace:: " in line:
            title = line.split("replace:: ", 1)[1].strip()
            return title.lstrip("**").rstrip("**")
    print(
        "Could not parse previous title from website/latest_news_title.rst",
        file=sys.stderr,
    )
    sys.exit(1)


def update_news_file(
    release_notes: str, tag: str, release_date: datetime
) -> None:
    date_str = format_month_day_year(release_date)
    new_section_title = f"{date_str} - OpenEXR {tag} Released"

    old_section_title = _parse_latest_news_title(
        _git_show("website/latest_news_title.rst")
    )

    with open("website/latest_news_title.rst", "w", encoding="utf-8") as f:
        f.write("..\n")
        f.write("  SPDX-License-Identifier: BSD-3-Clause\n")
        f.write("  Copyright (c) Contributors to the OpenEXR Project.\n")
        f.write(f".. |latest-news-title| replace:: **{new_section_title}**")

    content = _git_show("website/news.rst")
    old_news = re.split(r"^=+\s*$", content, maxsplit=1, flags=re.MULTILINE)
    if len(old_news) < 2:
        print(
            "Unexpected format in website/news.rst (missing section divider)",
            file=sys.stderr,
        )
        sys.exit(1)
    old_news[1] = re.sub(
        r"\.\. _LatestNewsStart:\n", "", old_news[1], flags=re.DOTALL
    )
    old_news[1] = re.sub(
        r"\.\. _LatestNewsEnd:\n", "", old_news[1], flags=re.DOTALL
    )

    with open("website/news.rst", "w", encoding="utf-8") as f:
        f.write(old_news[0])
        f.write("=" * len(new_section_title) + "\n\n")
        f.write(".. _LatestNewsStart:\n\n")
        f.write(release_notes + "\n\n")
        f.write(".. _LatestNewsEnd:\n\n")
        f.write(old_section_title + "\n")
        f.write("=" * len(old_section_title))
        f.write(old_news[1])


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: news.py <tag>   e.g. news.py v3.4.8", file=sys.stderr)
        sys.exit(1)
    tag = sys.argv[1]
    release_date, release_notes, release_version = load_release_notes(tag)
    rst_text = markdown_to_rst(release_notes)
    update_news_file(rst_text, release_version, release_date)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
