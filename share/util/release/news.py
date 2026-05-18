#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Update website/news.rst and latest_news_title.rst for a release."""

from __future__ import annotations

import re
import sys
from datetime import datetime
from subprocess import PIPE, run

from _common import load_release_notes


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


def update_news_file(
    release_notes: str, tag: str, release_date: datetime
) -> None:
    date_str = release_date.strftime("%B %e, %Y")
    new_section_title = f"{date_str} - OpenEXR {tag} Released"

    result = run(
        ["git", "show", "HEAD:website/latest_news_title.rst"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    line = result.stdout.split("\n")[-1]
    old_section_title = line.split("replace:: ")[1].rstrip().lstrip("**").rstrip("**")

    with open("website/latest_news_title.rst", "w", encoding="utf-8") as f:
        f.write("..\n")
        f.write("  SPDX-License-Identifier: BSD-3-Clause\n")
        f.write("  Copyright (c) Contributors to the OpenEXR Project.\n")
        f.write(f".. |latest-news-title| replace:: **{new_section_title}**")

    result = run(
        ["git", "show", "HEAD:website/news.rst"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    content = result.stdout
    if content is None:
        print(f"No news.rst at tag {tag}")

    old_news = re.split(r"^=+\s*$", content, maxsplit=1, flags=re.MULTILINE)
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
