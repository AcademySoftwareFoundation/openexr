#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""Add or update a release section in CHANGES.md with merged PR entries."""

from __future__ import annotations

import json
import re
import sys
from datetime import datetime, timedelta
from subprocess import PIPE, run

from _common import (
    changes_anchor_date_slug,
    format_month_day_year,
    prev_patch_version,
    require_repo_url,
)
from _security import (
    collect_security_refs_for_prs,
    gh_security_advisories_cve_titles,
    oss_fuzz_issue_titles,
)

MERGED_PR_HEADING_RE = re.compile(
    r"^###\s+Merged Pull Requests\s*:?\s*$", re.IGNORECASE
)
MERGED_WORKFLOW_HEADING_RE = re.compile(
    r"^###\s+Merged Workflow Pull Requests\s*:?\s*$", re.IGNORECASE
)
SECURITY_HEADING_RE = re.compile(r"^###\s+Security\s*:?\s*$", re.IGNORECASE)
PR_BULLET_RE = re.compile(r"^\*\s*\[(\d+)\]\(")


def gh_pr_view(pr_number: str) -> dict:
    result = run(
        [
            "gh",
            "pr",
            "view",
            pr_number,
            "--json",
            "title,author",
        ],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "gh pr view failed\n")
        sys.exit(1)
    return json.loads(result.stdout)


def strip_security_from_heading(heading_lines: list[str]) -> list[str]:
    out: list[str] = []
    i = 0
    n = len(heading_lines)
    while i < n:
        if SECURITY_HEADING_RE.match(heading_lines[i].strip()):
            if out and not out[-1].strip():
                out.pop()
            i += 1
            while i < n and not heading_lines[i].startswith("### "):
                i += 1
            continue
        out.append(heading_lines[i])
        i += 1
    return out


def parse_pr_blocks_dict(lines, i, stop_at_workflow_heading):
    out = {}
    n = len(lines)
    while i < n:
        line = lines[i]
        if stop_at_workflow_heading and MERGED_WORKFLOW_HEADING_RE.match(
            line.strip()
        ):
            break
        mo = PR_BULLET_RE.match(line.strip())
        if mo:
            pr = mo.group(1)
            bullet = line
            if i + 1 < n:
                nxt = lines[i + 1]
                if stop_at_workflow_heading and MERGED_WORKFLOW_HEADING_RE.match(
                    nxt.strip()
                ):
                    out[pr] = bullet
                    i += 1
                    continue
                if PR_BULLET_RE.match(nxt.strip()):
                    out[pr] = bullet
                    i += 1
                    continue
                out[pr] = "\n".join([bullet, nxt])
                i += 2
                continue
            out[pr] = bullet
            i += 1
            continue
        i += 1
    return out, i


def parse_section(lines):
    heading = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        if MERGED_PR_HEADING_RE.match(line.strip()):
            break
        heading.append(line)
        i += 1

    merged_prs = {}
    if i < n and MERGED_PR_HEADING_RE.match(lines[i].strip()):
        i += 1
        merged_prs, i = parse_pr_blocks_dict(lines, i, stop_at_workflow_heading=True)

    merged_workflow_prs = {}
    if i < n and MERGED_WORKFLOW_HEADING_RE.match(lines[i].strip()):
        i += 1
        merged_workflow_prs, i = parse_pr_blocks_dict(
            lines, i, stop_at_workflow_heading=False
        )

    return heading, merged_prs, merged_workflow_prs


def pr_is_workflow_only(pr_number: str) -> bool:
    result = run(
        ["gh", "pr", "view", pr_number, "--json", "files", "--jq", "[.files[].path]"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "gh pr view failed\n")
        sys.exit(1)
    paths = json.loads(result.stdout or "[]")
    return bool(paths) and all(p.startswith(".github/workflows/") for p in paths)


def main() -> None:
    if len(sys.argv) < 3:
        print(
            "Usage: changes.py <tag> <pr-number> ...\n"
            "Example: changes.py v3.4.7 1234 1235",
            file=sys.stderr,
        )
        sys.exit(1)

    tag = sys.argv[1]
    prs = sys.argv[2:]

    with open("CHANGES.md", encoding="utf-8") as f:
        lines = f.read().splitlines()

    base_tag = tag.lstrip("v").split("-rc")[0]
    prev_tag = prev_patch_version(base_tag)

    section_re = re.compile(
        rf"^##\s+Version\s+{re.escape(base_tag)}\b", re.IGNORECASE
    )
    prev_re = (
        re.compile(rf"^##\s+Version\s+{re.escape(prev_tag)}\b", re.IGNORECASE)
        if prev_tag
        else None
    )

    section_index = None
    footer_index = None
    for i, line in enumerate(lines):
        if section_index is None and section_re.match(line):
            section_index = i
        if prev_re is not None and footer_index is None and prev_re.match(line):
            footer_index = i

    header_index = section_index if section_index is not None else footer_index
    if header_index is None:
        print(
            "Could not locate insertion point (no matching version section and no "
            "previous patch release heading in CHANGES.md).",
            file=sys.stderr,
        )
        sys.exit(1)

    release_date = datetime.now() + timedelta(days=2)
    date_str = format_month_day_year(release_date)

    if section_index is not None:
        section_heading, merged_prs, merged_workflow_prs = parse_section(
            lines[section_index:footer_index]
        )
        section_heading = strip_security_from_heading(section_heading)
    else:
        section_heading = [f"## Version {base_tag} ({date_str})\n"]
        merged_prs = {}
        merged_workflow_prs = {}

    toc, prev_toc = None, None
    toc_re = re.compile(rf"^\*\s+\[Version\s+{re.escape(base_tag)}\]", re.IGNORECASE)
    prev_toc_re = (
        re.compile(rf"^\*\s+\[Version\s+{re.escape(prev_tag)}\]", re.IGNORECASE)
        if prev_tag
        else None
    )
    for i, line in enumerate(lines[:footer_index]):
        if toc is None and toc_re.match(line):
            toc = i
        elif prev_toc is None and prev_toc_re and prev_toc_re.match(line):
            prev_toc = i
            break

    url = require_repo_url()
    for pr_number in prs:
        info = gh_pr_view(pr_number)
        title = info.get("title") or ""
        author = (info.get("author") or {}).get("login") or ""
        is_workflow = "dependabot" in author or pr_is_workflow_only(pr_number)
        title_one_line = " ".join(title.split())
        pr_block = f"* [{pr_number}]({url}/pull/{pr_number})\n{title_one_line}"
        if is_workflow:
            merged_workflow_prs[pr_number] = pr_block
        else:
            merged_prs[pr_number] = pr_block

    all_prs_for_security = sorted(
        set(merged_prs) | set(merged_workflow_prs),
        key=int,
        reverse=True,
    )
    cves, oss_fuzz_issues = collect_security_refs_for_prs(all_prs_for_security)

    advisory_titles = gh_security_advisories_cve_titles()
    oss_fuzz_titles = oss_fuzz_issue_titles(list(dict.fromkeys(oss_fuzz_issues)))

    with open("CHANGES.md", "w", encoding="utf-8", newline="\n") as f:
        if toc is None:
            f.write("\n".join(lines[:prev_toc]) + "\n")
            base_tag_nonum = base_tag.replace(".", "")
            date_str_lower = changes_anchor_date_slug(release_date)
            f.write(
                f"* [Version {base_tag}](#version-{base_tag_nonum}-{date_str_lower}) "
                f"{date_str}\n"
            )
            f.write("\n".join(lines[prev_toc:header_index]) + "\n")
        else:
            f.write("\n".join(lines[:header_index]) + "\n")
        f.write("\n".join(section_heading))
        if cves or oss_fuzz_issues:
            f.write("\n\n### Security\n")
            f.write(
                "\nThis release addresses the following security "
                "vulnerabilities:\n\n"
            )
            for cve in sorted(set(cves), reverse=True):
                title = advisory_titles.get(cve, "")
                suffix = f"\n  {title}" if title else ""
                f.write(f"* [{cve}](https://www.cve.org/CVERecord?id={cve}){suffix}\n")
            for issue_id in sorted(set(oss_fuzz_issues), reverse=True):
                url_issue = f"https://issues.oss-fuzz.com/issues/{issue_id}"
                f.write(f"* OSS-Fuzz [{issue_id}]({url_issue})\n")
                short = oss_fuzz_titles.get(issue_id, "")
                if short:
                    f.write(f"  {short}\n")
        f.write("\n### Merged Pull Requests\n\n")
        for pr, value in sorted(merged_prs.items(), key=lambda kv: int(kv[0]), reverse=True):
            f.write("  " + value + "\n")
        f.write("\n### Merged Workflow Pull Requests\n\n")
        for pr, value in sorted(merged_workflow_prs.items(), key=lambda kv: int(kv[0]), reverse=True):
            f.write("  " + value + "\n")
        f.write("\n" + "\n".join(lines[footer_index:]))


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
