#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Create a draft GitHub release for a tagged release."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from subprocess import PIPE, run

from _common import (
    changes_anchor_date_slug,
    load_release_notes,
    require_repo_url,
)

PR_BULLET_RE = re.compile(r"^\*\s*\[(\d+)\]\(")
VERSION_HEADER_RE_TMPL = r"^##\s+.*Version\s+{}\b"
NEXT_VERSION_HEADER_RE = re.compile(r"^##\s+")


def extract_merged_pr_numbers(content: str, version_tag: str) -> list[str]:
    """
    Return the PR numbers listed under the "Merged Pull Requests" and
    "Merged Workflow Pull Requests" subsections for the given version.
    """
    version_header_re = re.compile(
        VERSION_HEADER_RE_TMPL.format(re.escape(version_tag)), re.IGNORECASE
    )
    lines = content.splitlines()
    capture = False
    prs: list[str] = []
    for line in lines:
        if version_header_re.match(line):
            capture = True
            continue
        if capture and NEXT_VERSION_HEADER_RE.match(line):
            break
        if capture:
            mo = PR_BULLET_RE.match(line.strip())
            if mo:
                prs.append(mo.group(1))
    return prs


def gh_pr_author(pr_number: str) -> dict:
    result = run(
        ["gh", "pr", "view", pr_number, "--json", "author"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        return {}
    return json.loads(result.stdout).get("author") or {}


def collect_contributors(pr_numbers: list[str]) -> dict[str, str]:
    """Map GitHub login -> display name for non-bot authors of the given PRs."""
    contributors: dict[str, str] = {}
    for pr_number in pr_numbers:
        author = gh_pr_author(pr_number)
        login = author.get("login")
        if not login or author.get("is_bot"):
            continue
        contributors[login] = author.get("name") or login
    return contributors


def build_contributors_section(contributors: dict[str, str]) -> str:
    """Render a "### Contributors" section with linked avatar images, sorted alphabetically."""
    logins = sorted(contributors, key=lambda login: contributors[login].casefold())
    avatars = [
        f'[![{contributors[login]}](https://github.com/{login}.png?size=50 '
        f'"{contributors[login]}")](https://github.com/{login})'
        for login in logins
    ]
    return "### Contributors\n\n" + " ".join(avatars) + "\n"


def build_changes_link(tag: str, release_version: str, release_date) -> str:
    """Return a "See CHANGES.md for more details" line linking to the release's section."""
    url = require_repo_url()
    base_tag_nonum = release_version.replace(".", "")
    date_slug = changes_anchor_date_slug(release_date)
    anchor = f"version-{base_tag_nonum}-{date_slug}"
    changes_url = f"{url}/blob/{tag}/CHANGES.md#{anchor}"
    return f"See [CHANGES.md]({changes_url}) for more details."


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
    release_date, release_notes, release_version = load_release_notes(tag)

    changes_link = build_changes_link(tag, release_version, release_date)
    release_notes = release_notes.rstrip() + "\n\n" + changes_link + "\n"

    changes_path = Path("CHANGES.md")
    if changes_path.is_file():
        content = changes_path.read_text(encoding="utf-8")
        pr_numbers = extract_merged_pr_numbers(content, release_version)
        contributors = collect_contributors(pr_numbers)
        if contributors:
            release_notes = (
                release_notes.rstrip() + "\n\n" + build_contributors_section(contributors)
            )

    create_draft_release(tag, release_notes)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)

