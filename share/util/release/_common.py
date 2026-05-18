# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Shared helpers for OpenEXR release utility scripts."""

from __future__ import annotations

import re
import sys
from datetime import datetime
from pathlib import Path
from subprocess import PIPE, run


def extract_section(content: str, version_tag: str) -> tuple[datetime | None, str]:
    """
    Extract the section of release notes starting with a heading like
    ``## Version <tag>``. Include only the text in the section itself,
    omitting subsections (merged PRs, etc.).
    """
    version_header = re.compile(
        rf"^##.*Version {re.escape(version_tag)}\b.*\(([^)]+)\)",
        re.IGNORECASE,
    )
    subsection_header_pattern = re.compile(r"^##\s+")
    lines = content.splitlines()
    capture = False
    section_lines: list[str] = []
    release_date = None

    for line in lines:
        m = version_header.match(line)
        if m:
            release_date = datetime.strptime(m.group(1), "%B %d, %Y")
            capture = True
            continue
        if capture and (
            subsection_header_pattern.match(line)
            or "Merged Pull Requests" in line
            or "Merged pull requests" in line
        ):
            break
        if capture:
            section_lines.append(line)

    return release_date, "\n".join(section_lines).strip()


def apply_release_note_emojis(release_notes: str) -> str:
    """Replace :emoji: shortcodes in release notes with Unicode symbols."""
    subs = (
        (r":bug:", "🐛"),
        (r":rocket:", "🚀"),
        (r":hammer_and_wrench:", "🛠️"),
        (r":wrench:", "🔧"),
        (r":sparkles:", "✨"),
        (r":snake:", "🐍"),
        (r":package:", "📦"),
        (r":warning:", "⚠️"),
        (r":book:", "📖"),
    )
    for pattern, repl in subs:
        release_notes = re.sub(pattern, repl, release_notes, flags=re.DOTALL)
    return release_notes


def parse_release_version(tag: str) -> str:
    """Strip leading ``v`` and trailing ``-rc*`` from a release tag."""
    return tag.lstrip("v").split("-rc")[0]


def get_repo_url() -> str | None:
    try:
        result = run(
            ["git", "config", "--get", "remote.upstream.url"],
            stdout=PIPE,
            stderr=PIPE,
            universal_newlines=True,
            check=False,
        )
        url = (result.stdout or "").strip().rstrip(".git")
        if url:
            return url
        result = run(
            ["git", "config", "--get", "remote.origin.url"],
            stdout=PIPE,
            stderr=PIPE,
            universal_newlines=True,
            check=False,
        )
        return (result.stdout or "").strip().rstrip(".git") or None
    except OSError:
        return None


def load_release_notes(tag: str) -> tuple[datetime, str, str]:
    """
    Read ``CHANGES.md`` and return ``(release_date, release_notes, release_version)``.
    Exits the process if the section is missing.
    """
    release_version = parse_release_version(tag)
    changes_path = Path("CHANGES.md")
    if not changes_path.is_file():
        print("CHANGES.md not found.", file=sys.stderr)
        sys.exit(1)
    content = changes_path.read_text(encoding="utf-8")
    release_date, release_notes = extract_section(content, release_version)
    if release_date is None:
        print("No release found.", file=sys.stderr)
        sys.exit(1)
    release_notes = apply_release_note_emojis(release_notes)
    return release_date, release_notes, release_version


def prev_patch_version(base_tag: str) -> str | None:
    """
    Previous release in the same major.minor line, e.g. ``3.4.5`` -> ``3.4.4``.
    """
    parts = base_tag.split(".")
    if not parts or not parts[-1].isdigit():
        return None
    patch = int(parts[-1])
    if patch <= 0:
        return None
    parts[-1] = str(patch - 1)
    return ".".join(parts)
