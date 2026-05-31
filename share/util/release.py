#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# Helper script that automates parts of the release proces:
#
# `release.py notes <tag>` - extract release notes from CHANGES.md for the tagged release, print to stdout
# `release.py news <tag>` - edit the website/news.rst file to add reference to the tagged release
# `release.py draft <tag>` - create a draft release on GitHub
# `release.py candidate <tag>` - format a message about the upcoming release, print to stdout
# `release.py tag <tag>` - create a signed annotated git tag with the release notes as the tag message
# `release.py cherry <label>` - list merged PRs with the given label, print cherry-pick lines (oldest merge first)
# `release.py changes <tag> [pr#> ... ]` - add section to CHANGES.md with given PRs
# `release.py log` - print a `git log` annotated with the labels from each commit's associated PR, if there is one.
#
# The file `CHANGES.md` is assumed to old the release notes. When
# preparing a release, edit this file by hand to add a description of
# the new release, with a heading `## Version ...`.
#
# The file `website/news.rst` is the source for the website's news
# page. The title of the most recent news item goes in
# `website/latest_news_title.rst`, which is also included on the main
# website landing page. The text for the news blurb is bracketed by
# `.. _LatestNewsStart:` and `.. _LatestNewsEnd:`, which allows the
# main website landing page to reproduce the text.
#

import json
import sys
import re
import subprocess
from pathlib import Path
from subprocess import PIPE, run
from datetime import datetime, timedelta
from urllib.error import HTTPError, URLError
from urllib.request import urlopen
import markdown

OSS_FUZZ_ISSUE_JSON_PREFIX = ")]}'"
OSS_FUZZ_ISSUE_API = "https://issues.oss-fuzz.com/action/issues/{issue_id}?format=json"

def extract_section(content, version_tag):
    """Extract the section of release notes starting with a heading
    like "Version <tag>".  Include only the text in the section
    itself, omitting any subsections, which is typically the merged
    PRs, etc..
    """

    # Regular expression to match the version title
    version_header = re.compile(rf'^##.*Version {re.escape(version_tag)}\b.*\(([^)]+)\)', re.IGNORECASE)

    # Regular expression to match subheaders (### or higher)
    subsection_header_pattern = re.compile(r'^##\s+')

    lines = content.splitlines()
    capture = False
    section_lines = []
    release_date = None

    for line in lines:

        # Look for the start of the requested version section
        m = version_header.match(line)
        if m:
            release_date = datetime.strptime(m.group(1), "%B %d, %Y")
            capture = True
            continue

        # Stop capturing when the next subsection (##) starts
        if capture and (subsection_header_pattern.match(line) or
                        "Merged Pull Requests" in line or
                        "Merged pull requests" in line):
            break
        # Capture lines if inside the correct section and before any subsections
        if capture:
            section_lines.append(line)

    return release_date, '\n'.join(section_lines).strip()

def markdown_to_rst(markdown_text):
    """
    Convert simple Markdown text to reStructuredText (reST) manually.
    """

    # Convert `text` to ``text`` (but skip existing ``)
    markdown_text = re.sub(r'(?<!`)`([^`]+)`(?!`)', r'``\1``', markdown_text)

    # Replace markdown links [text](url) with reST format `text <url>`_
    markdown_text = re.sub(
        r'\[([\s\S]*?)\]\(([\s\S]*?)\)',
        r'`\1 <\2>`_',
        markdown_text
    )

    # Convert the special symbols
    markdown_text = re.sub(r':bug:', "🐛", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':rocket:', "🚀", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':hammer_and_wrench:', "🛠️", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':wrench:', "🔧", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':sparkles:', "✨", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':snake:', "🐍", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':package:', "📦", markdown_text, flags=re.DOTALL)
    markdown_text = re.sub(r':warning:', "⚠️", markdown_text, flags=re.DOTALL)

    # Split into lines for further processing
    lines = markdown_text.splitlines()
    rst_lines = []
    for line in lines:
        # Replace bullet points (- becomes *)
        if line.startswith("- "):
            line = line.replace("- ", "* ", 1)

        # Handle other elements (bold, italics, etc.)
        line = re.sub(r'\*\*(.*?)\*\*', r'**\1**', line)  # Bold
        line = re.sub(r'\*(.*?)\*', r'*\1*', line)  # Italics

        rst_lines.append(line)

    return '\n'.join(rst_lines).strip()

def create_draft_release(tag, release_notes):
    """
    Create a draft release using the GitHub CLI.
    """

    release_tag = tag.split("-rc")[0]
    result = run(['gh', 'release', 'create', tag, '--draft', '--title', release_tag, '-F', '-'],
                 input=release_notes,
                 text=True,
                 check=True
                 )

def get_repo_url():
    try:
        # Return the upstream if this is in a fork
        result = run(['git', 'config', '--get', 'remote.upstream.url'],
                     stdout=PIPE, stderr=PIPE, universal_newlines=True)
        url = result.stdout.strip().rstrip(".git")
        if url:
            return url
        
        result = run(['git', 'config', '--get', 'remote.origin.url'],
                     stdout=PIPE, stderr=PIPE, universal_newlines=True)
        url = result.stdout.strip().rstrip(".git")
        return url
    except subprocess.CalledProcessError:
        return None  # Not a git repo or no origin remote


def cmd_cherry(label):
    """
    List merged PRs with the given GitHub label, ordered by merge time
    (oldest first). Print one line per merge commit suitable for cherry-picking.
    """
    result = run(
        [
            "gh",
            "pr",
            "list",
            "--label",
            label,
            "--state",
            "merged",
            "--limit",
            "500",
            "--json",
            "mergedAt,mergeCommit,title,number",
        ],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "gh pr list failed\n")
        sys.exit(1)

    try:
        prs = json.loads(result.stdout or "[]")
    except json.JSONDecodeError as e:
        print(f"Invalid JSON from gh: {e}", file=sys.stderr)
        sys.exit(1)

    merged = []
    for pr in prs:
        id = pr.get("number") or "?"
        mc = pr.get("mergeCommit") or {}
        oid = mc.get("oid")
        merged_at = pr.get("mergedAt")
        title = pr.get("title") or ""
        if not oid or not merged_at:
            continue
        merged.append((merged_at, oid, title, id))

    merged.sort(key=lambda x: x[0])

    changes_prs = ""
    
    for _merged_at, oid, title, id in merged:
        abbrev = oid[:7]
        title_one_line = " ".join(title.split())
        print(f"git cherry-pick {abbrev} # {title_one_line} (#{id})")
        changes_prs += f" {id}" 

    print(f"git cherry-pick {' '.join(o[:7] for a, o, t, i in merged)}")
    print(f"release.py changes {label} {' '.join(str(id) for a, o, t, id in merged)}")

def prev_patch_version(base_tag):
    """
    Previous release in the same major.minor line, e.g. 3.4.5 -> 3.4.4.
    Returns None if the patch segment is missing, non-numeric, or already 0.
    """
    parts = base_tag.split(".")
    if not parts or not parts[-1].isdigit():
        return None
    patch = int(parts[-1])
    if patch <= 0:
        return None
    parts[-1] = str(patch - 1)
    return ".".join(parts)


def gh_pr_view(pr_number):
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


def gh_security_advisories_cve_titles() -> dict[str, str]:
    """
    List all repository security advisories for the current repo via ``gh api``
    (``GET .../security-advisories``, paginated). Build a dict mapping each
    assigned ``CVE-YYYY-NNNN`` id (uppercase) to the advisory ``summary``
    (GitHub's short title). Entries without ``cve_id`` are omitted.

    Requires ``gh`` authentication with permission to read repository security
    advisories. Returns ``{}`` if ``gh`` fails, the repo cannot be resolved, or
    the response is not a JSON array. If multiple advisories share the same
    ``cve_id``, the first wins.
    """
    r = run(
        ["gh", "repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if r.returncode != 0:
        return {}
    nwo = (r.stdout or "").strip()
    if "/" not in nwo:
        return {}

    r2 = run(
        [
            "gh",
            "api",
            f"repos/{nwo}/security-advisories",
            "--paginate",
        ],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if r2.returncode != 0:
        return {}

    try:
        items = json.loads(r2.stdout or "[]")
    except json.JSONDecodeError:
        return {}

    if not isinstance(items, list):
        return {}

    out: dict[str, str] = {}
    for item in items:
        if not isinstance(item, dict):
            continue
        cve = item.get("cve_id")
        if not cve or not isinstance(cve, str):
            continue
        key = cve.strip().upper()
        summary = item.get("summary")
        title = summary.strip() if isinstance(summary, str) else ""
        if key not in out:
            out[key] = title
    return out


# Line must contain address / addresses / addressed / addressing (word-boundary);
# collect every CVE-YYYY-nnnn and OSS-Fuzz issue URL on that line.
PR_LINE_ADDRESS_RE = re.compile(r"(?i)\bAddress(?:es|ed|ing)?\b")
CVE_ID_RE = re.compile(r"CVE-\d{4}-\d+", re.IGNORECASE)
OSS_FUZZ_ISSUE_RE = re.compile(
    r"https://issues\.oss-fuzz\.com/issues/(\d+)", re.IGNORECASE
)


def _collect_addressed_security_refs(blocks: list[str]) -> tuple[list[str], list[str]]:
    """
    On each line containing ``Address`` / ``Addresses`` / etc., collect CVE ids
    and OSS-Fuzz issue ids (from ``https://issues.oss-fuzz.com/issues/<id>`` URLs).
    Returns ``(cves, oss_fuzz_issue_ids)`` in first-seen order within each list.
    """
    cves: list[str] = []
    cves_seen: set[str] = set()
    oss_fuzz: list[str] = []
    oss_seen: set[str] = set()
    for block in blocks:
        for line in block.splitlines():
            if not PR_LINE_ADDRESS_RE.search(line):
                continue
            for m in CVE_ID_RE.finditer(line):
                cve = m.group(0).upper()
                if cve not in cves_seen:
                    cves_seen.add(cve)
                    cves.append(cve)
            for m in OSS_FUZZ_ISSUE_RE.finditer(line):
                issue_id = m.group(1)
                if issue_id not in oss_seen:
                    oss_seen.add(issue_id)
                    oss_fuzz.append(issue_id)
    return cves, oss_fuzz


def _oss_fuzz_short_title(full_title: str) -> str:
    """
    Strip the ``project:fuzzer:`` prefix from an OSS-Fuzz issue title, leaving
    the crash description (matches manual CHANGES.md style).
    """
    parts = full_title.split(":", 2)
    if len(parts) >= 3:
        return parts[2].strip()
    return full_title.strip()


def _oss_fuzz_full_title_from_fetch(data, issue_id: str) -> str | None:
    """Extract the full issue title from an IssueFetchResponse JSON payload."""
    if isinstance(data, dict):
        return None
    target = int(issue_id)

    def walk(obj):
        if isinstance(obj, list):
            if (
                len(obj) >= 3
                and obj[0] is None
                and obj[1] == target
                and isinstance(obj[2], list)
                and len(obj[2]) > 5
                and isinstance(obj[2][5], str)
            ):
                return obj[2][5]
            for item in obj:
                found = walk(item)
                if found:
                    return found
        return None

    return walk(data)


def _fetch_oss_fuzz_issue_json(issue_id: str):
    """
    Fetch issue metadata from issues.oss-fuzz.com. Public issues return JSON;
    private or restricted issues may return HTTP 403 or a JSON error object.
    """
    url = OSS_FUZZ_ISSUE_API.format(issue_id=issue_id)
    try:
        with urlopen(url, timeout=30) as resp:
            raw = resp.read().decode()
    except (HTTPError, URLError, TimeoutError, OSError) as exc:
        print(
            f"Warning: could not fetch OSS-Fuzz issue {issue_id}: {exc}",
            file=sys.stderr,
        )
        return None
    if raw.startswith(OSS_FUZZ_ISSUE_JSON_PREFIX):
        raw = raw[len(OSS_FUZZ_ISSUE_JSON_PREFIX) :]
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        print(
            f"Warning: invalid JSON fetching OSS-Fuzz issue {issue_id}",
            file=sys.stderr,
        )
        return None


def oss_fuzz_issue_titles(issue_ids: list[str]) -> dict[str, str]:
    """
    Map OSS-Fuzz issue id to a short title (crash description only) for use in
    release notes. Issues that cannot be fetched are omitted from the dict.
    """
    titles: dict[str, str] = {}
    for issue_id in issue_ids:
        data = _fetch_oss_fuzz_issue_json(issue_id)
        if data is None:
            continue
        if isinstance(data, dict) and data.get("message"):
            print(
                f"Warning: OSS-Fuzz issue {issue_id}: {data['message']}",
                file=sys.stderr,
            )
            continue
        full = _oss_fuzz_full_title_from_fetch(data, issue_id)
        if full:
            titles[issue_id] = _oss_fuzz_short_title(full)
    return titles


def _gh_pr_text_blocks(pr_number: str) -> list[str]:
    result = run(
        [
            "gh",
            "pr",
            "view",
            pr_number,
            "--json",
            "body,comments,reviews",
        ],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "gh pr view failed\n")
        sys.exit(1)
    data = json.loads(result.stdout or "{}")
    blocks: list[str] = []
    body = data.get("body")
    if body:
        blocks.append(body)
    for c in data.get("comments") or []:
        if isinstance(c, dict):
            b = c.get("body")
            if b:
                blocks.append(b)
    for r in data.get("reviews") or []:
        if isinstance(r, dict):
            b = r.get("body")
            if b:
                blocks.append(b)
    return blocks


def pr_addressed_cves(pr_number: str) -> tuple[list[str], list[str]]:
    """
    Scan the GitHub PR description, issue comments, and review bodies.
    On each line that contains ``Address``, ``Addresses``, ``address``, etc.
    (case-insensitive, word-boundary match), collect:

    * every ``CVE-<year>-<number>`` substring
    * every OSS-Fuzz issue referenced as
      ``https://issues.oss-fuzz.com/issues/<id>``

    Return ``(cves, oss_fuzz_issue_ids)``, each list in first-seen order.
    Empty lists if nothing matches.

    This approximates prose such as "Addresses CVE-2024-12345" or
    "Addresses https://issues.oss-fuzz.com/issues/512314697".
    """

    if not pr_number:
        return [], []

    return _collect_addressed_security_refs(_gh_pr_text_blocks(pr_number))


MERGED_PR_HEADING_RE = re.compile(
    r"^###\s+Merged Pull Requests\s*:?\s*$", re.IGNORECASE
)
MERGED_WORKFLOW_HEADING_RE = re.compile(
    r"^###\s+Merged Workflow Pull Requests\s*:?\s*$", re.IGNORECASE
)
SECURITY_HEADING_RE = re.compile(r"^###\s+Security\s*:?\s*$", re.IGNORECASE)
PR_BULLET_RE = re.compile(r"^\*\s*\[(\d+)\]\(")


def strip_security_from_heading(heading_lines: list[str]) -> list[str]:
    """
    Remove an existing ``### Security`` subsection from release section heading
    lines so ``cmd_changes`` can regenerate it from current PR references.

    Preserves a single blank line that separated descriptive text from
    ``### Security``; that blank is reinserted when the section is rewritten.
    """
    out: list[str] = []
    i = 0
    n = len(heading_lines)
    while i < n:
        if SECURITY_HEADING_RE.match(heading_lines[i].strip()):
            # Drop the blank line between descriptive text and ### Security.
            if out and not out[-1].strip():
                out.pop()
            i += 1
            while i < n and not heading_lines[i].startswith("### "):
                i += 1
            continue
        out.append(heading_lines[i])
        i += 1
    return out


def collect_security_refs_for_prs(pr_numbers: list[str]) -> tuple[list[str], list[str]]:
    """
    Scan the given PR numbers for CVE and OSS-Fuzz references. Returns
    ``(cves, oss_fuzz_issue_ids)`` in first-seen order (deduped per list).
    """
    cves: list[str] = []
    oss_fuzz_issues: list[str] = []
    for pr_number in pr_numbers:
        pr_cves, pr_oss_fuzz = pr_addressed_cves(pr_number)
        cves.extend(pr_cves)
        oss_fuzz_issues.extend(pr_oss_fuzz)
    return cves, oss_fuzz_issues


def parse_pr_blocks_dict(lines, i, stop_at_workflow_heading):
    """
    From index *i*, read ``* [pr](url)`` entries. Each value is the bullet line and
    the following line joined with ``\\n`` when that following line is not another
    bullet or (if applicable) the workflow subsection heading.

    If *stop_at_workflow_heading* is true, stop before ``### Merged Workflow Pull Requests``.
    Returns ``(dict pr -> str, next_index)``.
    """
    out = {}
    n = len(lines)
    while i < n:
        line = lines[i]
        if stop_at_workflow_heading and MERGED_WORKFLOW_HEADING_RE.match(line.strip()):
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
    """
    Parse a version slice of CHANGES.md (list of line strings, no trailing newlines).

    Returns ``heading`` (lines before ``### Merged Pull Requests``), and two dicts
    mapping PR number -> two-line bullet+title string for each subsection.
    """
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
        merged_workflow_prs, i = parse_pr_blocks_dict(lines, i, stop_at_workflow_heading=False)

    return heading, merged_prs, merged_workflow_prs

def pr_is_workflow_only(pr_number: str) -> bool:
    """Return True if every file changed by the PR is under .github/workflows/."""
    result = run(
        ["gh", "pr", "view", pr_number, "--json", "files",
         "--jq", "[.files[].path]"],
        stdout=PIPE, stderr=PIPE, universal_newlines=True, check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stderr or "gh pr view failed\n")
        sys.exit(1)
    paths = json.loads(result.stdout or "[]")
    return bool(paths) and all(p.startswith(".github/workflows/") for p in paths)

def cmd_changes(tag, prs):
    """
    Add a section to CHANGES.md for the given release if one doesn't already exist, and add the given PR 
    to the list if it's not already there.
    """

    with open("CHANGES.md", encoding="utf-8") as f:
        lines = f.read().splitlines()

    base_tag = tag.lstrip("v").split("-rc")[0] # i.e. "3.4.7"
    prev_tag = prev_patch_version(base_tag) # i.e.e "3.4.6"

    section_re = re.compile(rf"^##\s+Version\s+{re.escape(base_tag)}\b", re.IGNORECASE)
    prev_re = re.compile(rf"^##\s+Version\s+{re.escape(prev_tag)}\b", re.IGNORECASE)

    #
    # Terminology:
    #  "header" = everything from the beginning of the file up to the new section
    #  "footer" = everything afer the new section
    #
    
    # Find the line index of the existing section if there is one, and
    # to the previous release section, i.e. the release before, which
    # comes after it in the file.
    
    section_index = None
    footer_index = None # line of the following section, i.e. the remainder of the file.
    for i, line in enumerate(lines):
        if section_index is None and section_re.match(line):
            section_index = i
        if prev_re is not None and footer_index is None and prev_re.match(line):
            footer_index = i

    # header_index is the last line of content before the new section
    header_index = ( 
        section_index if section_index is not None else footer_index
    )
    if header_index is None:
        print(
            "Could not locate insertion point (no matching version section and no "
            "previous patch release heading in CHANGES.md).",
            file=sys.stderr,
        )
        sys.exit(1)

    release_date = datetime.now() + timedelta(days=2)
    date_str = release_date.strftime("%B %e, %Y")

    if section_index is not None:
        # Parse the existing section into the heading (section heading
        # and existing notes), the dict of merged PRs, and the dict of merge workflow PRs
        section_heading, merged_prs, merged_workflow_prs = parse_section(
            lines[section_index:footer_index]
        )
        section_heading = strip_security_from_heading(section_heading)
    else:
        # The section does not exist, so create the stub of a new one
        section_heading = [f"## Version {base_tag} ({date_str})\n"]
        merged_prs = {}
        merged_workflow_prs = {}

    # Find the index of the table of contents entry, if there is one.
    toc, prev_toc = None, None
    toc_re = re.compile(rf"^\*\s+\[Version\s+{re.escape(base_tag)}\]", re.IGNORECASE)
    prev_toc_re = re.compile(rf"^\*\s+\[Version\s+{re.escape(prev_tag)}\]", re.IGNORECASE)
    for i, line in enumerate(lines[:footer_index]):
        if toc is None and toc_re.match(line):
            toc = i
        elif prev_toc is None and prev_toc_re.match(line):
            prev_toc = i
            break

    # Format the list entry for each new PR:
    # * [number](url)
    # title
    for pr_number in prs:
        info = gh_pr_view(pr_number)
        title = info.get("title") or ""
        author = (info.get("author") or {}).get("login") or ""
        is_workflow = "dependabot" in author or pr_is_workflow_only(pr_number)
        url = get_repo_url()
        title_one_line = " ".join(title.split())
        pr_block = f"* [{pr_number}]({url}/pull/{pr_number})\n{title_one_line}"
        # Add it to the appropriate section
        if is_workflow:
            merged_workflow_prs[pr_number] = pr_block
        else:
            merged_prs[pr_number] = pr_block

    # Scan every PR listed in the section (not only those on the command line)
    # so Security stays complete when ``changes`` is re-run incrementally.
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
            # No table of contents entry for this release, so add one.
            f.write("\n".join(lines[:prev_toc]) + "\n")  # everything up to the previous release entry
            # Write new entry
            base_tag_nonum = base_tag.replace(".","")
            date_str_lower = date_str.lower().replace(",","").replace(" ","-")
            f.write(f"* [Version {base_tag}](#version-{base_tag_nonum}-{date_str_lower}) {date_str}\n")
            # Write the rest of the table of contents and everything up to the new release section
            f.write("\n".join(lines[prev_toc:header_index]) + "\n")  
        else:
            # Table of contents already has an entry, so just write the header
            f.write("\n".join(lines[:header_index]) + "\n")
        # Write the release section
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
                url = f"https://issues.oss-fuzz.com/issues/{issue_id}"
                f.write(f"* OSS-Fuzz [{issue_id}]({url})\n")
                short = oss_fuzz_titles.get(issue_id, "")
                if short:
                    f.write(f"  {short}\n")
        f.write("\n### Merged Pull Requests\n\n")
        for pr, value in sorted(merged_prs.items(), reverse=True):
            f.write("  " + value + "\n")
        f.write("\n### Merged Workflow Pull Requests\n\n")
        for pr, value in sorted(merged_workflow_prs.items(), reverse=True):
            f.write("  " + value + "\n")
        # Write the rest of the file
        f.write("\n" + "\n".join(lines[footer_index:]))

def update_news_file(release_notes, tag, release_date):
    """
    Update the website/news.rst file with the release notes.
    """
    date_str = release_date.strftime("%B %e, %Y")
    new_section_title = f"{date_str} - OpenEXR {tag} Released"

    # Extract the section header for the most recent item from website/latest_news_title.rst.
    # This needs to be added as an explicit section header.
    result = run(['git', 'show', f"HEAD:website/latest_news_title.rst"],
                 stdout=PIPE, stderr=PIPE, universal_newlines=True)
    line = result.stdout.split('\n')[-1]
    old_section_title = line.split("replace:: ")[1].rstrip().lstrip('**').rstrip('**')

    # Write the news section title to the latest_news_title.rst file,
    # so it can be include on the website front page
    with open('website/latest_news_title.rst', 'w') as f:
        f.write("..\n")
        f.write("  SPDX-License-Identifier: BSD-3-Clause\n")
        f.write("  Copyright (c) Contributors to the OpenEXR Project.\n")
        f.write(f".. |latest-news-title| replace:: **{new_section_title}**")

    result = run(['git', 'show', f"HEAD:website/news.rst"],
                 stdout=PIPE, stderr=PIPE, universal_newlines=True)
    content = result.stdout
    if content == None:
        print("No news.txt at tag {tag}")

    # Split the news text at the first header line, the one containing only ='s. Discard the ='s.
    # old_news[0] is everything up to the first ====...'
    # old_news[1] is everything after the frist ====...'
    old_news = re.split(r'^=+\s*$', content, maxsplit=1, flags=re.MULTILINE)

    # Remove the _LatestNewsStart/_LatestNewsEnd from the second half
    old_news[1] = re.sub(r'\.\. _LatestNewsStart:\n', '', old_news[1], flags=re.DOTALL)
    old_news[1] = re.sub(r'\.\. _LatestNewsEnd:\n', '', old_news[1], flags=re.DOTALL)

    # Write the new website/news.rst file, with the new section
    # inserted between the halves of the existing file, with new
    # section headers and _LastsNews markers.

    with open('website/news.rst', 'w') as f:
        f.write(old_news[0])
        f.write('='*len(new_section_title)+'\n\n')
        f.write('.. _LatestNewsStart:\n\n')
        f.write(release_notes+'\n\n')
        f.write('.. _LatestNewsEnd:\n\n')
        f.write(old_section_title+'\n')
        f.write('='*len(old_section_title))
        f.write(old_news[1])

def markdown_to_html(markdown_text):
    """
    Convert markdown content to HTML.
    """

    result = run(
        ['pandoc', '-f', 'markdown', '-t', 'html'],  # pandoc command to convert markdown to html
        input=markdown_text,                         # pass markdown_text as stdin
        text=True,                                   # treat input and output as strings (not bytes)
        capture_output=True                          # capture the output
    )

    html = f"<style> body {{ line-height: 1.0; margin: 0; padding: 0; }} p, h1, h2, h3, h4, h5, h6 {{ margin: 0; padding: 0; }} </style> {result.stdout}"

    return html


def convert_to_html(release_notes):
    """
    Convert release notes to HTML format.
    """
    notes = release_notes.replace('\n', '<br>')
    html_content = f"<h2>Release Notes</h2><p>{notes}</p>"
    return html_content

def pr_labels(line):
    """line is from `git log --pretty:format='%h %s'`, so for
    squash/merge commits, it ends in (#<pr>).

    Return the PR number and its release-related labels, i.e.
    labels of the form 'v<major>.<minor>.<patch>'
    """

    LOG_PR_SUFFIX_RE = re.compile(r"\(#(\d+)\)\s*$")
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

def cmd_log():
    """
    Run ``git log`` and print commits annotated with the release labels for the associated PRs
    """

    # Output looks like:
    # v3.4.7  3978976a Bump pypa/cibuildwheel from 3.3 to 3.4 (#2287)
    #         b19cfec2 Bump github/codeql-action from 4.32.4 to 4.32.5 (#2286)
    # v3.4.7  50ea61bd Fix build failure with glibc 2.43 due to C11 threads.h conflicts (#2262)

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
        pr, l = pr_labels(line)
        pr_cves, pr_oss_fuzz = pr_addressed_cves(pr)
        extras = " ".join(pr_cves + [f"oss-fuzz:{i}" for i in pr_oss_fuzz])
        print(f"{l:7} {line} {extras}")

def cmd_tag(tag, release_date, release_notes) -> None:
    """
    Create a signed annotated tag at the current ``HEAD`` using the
    given release date and notes
    """

    tag_message = f"{tag} - {release_date}\n{release_notes}\n"
    
    cmd = ["git", "tag", "-s", tag, "-F", "-"]
    run(cmd, input=tag_message, text=True, check=True)


def main():

    if len(sys.argv) > 1:
        action = sys.argv[1]
        if action == "log":
            cmd_log()
            return

    if len(sys.argv) < 2:
        print(
            "Usage: python release.py "
            "<notes|news|draft|candidate|tag|cherry|changes> ...\n"
            "  release.py changes <tag> <pr-number>   e.g. release.py changes v3.4.7 1234\n"
            "  release.py tag <tag> [--force]       e.g. release.py tag v3.4.7"
        )
        sys.exit(1)

    action = sys.argv[1]

    if action == "cherry":
        if len(sys.argv) < 3:
            print(
                "Usage: python release.py cherry <label>",
                file=sys.stderr,
            )
            sys.exit(1)
        cmd_cherry(sys.argv[2])
        return

    if action == "changes":
        if len(sys.argv) < 4:
            print(
                "Usage: python release.py changes <tag> <pr-number>\n"
                "Example: python release.py changes v3.4.7 1234",
                file=sys.stderr,
            )
            sys.exit(1)
        tag = sys.argv[2]
        prs = sys.argv[3:]
        cmd_changes(tag, prs)
        return

    if len(sys.argv) < 3:
        print(
            "Usage: python release.py "
            "<notes|news|draft|candidate|tag|cherry|changes> <tag-or-label> [date]"
        )
        sys.exit(1)

    tag = sys.argv[2]

    # Strip leading 'v' and trailing '-rc<candidate>' if necessary
    release_version = tag.lstrip('v').split('-rc')[0]

    # Get the content of the CHANGES.md at the specified git tag
    with open("CHANGES.md", "r") as f:
        content = f.read()

    # Extract the release notes
    release_date, release_notes = extract_section(content, release_version)
    if release_date == None:
        print("No release found.")
        return

    # Convert the special symbols
    release_notes = re.sub(r':bug:', "🐛", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':rocket:', "🚀", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':hammer_and_wrench:', "🛠️", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':wrench:', "🔧", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':sparkles:', "✨", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':snake:', "🐍", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':package:', "📦", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':warning:', "⚠️", release_notes, flags=re.DOTALL)
    release_notes = re.sub(r':book:', "📖", release_notes, flags=re.DOTALL)


    if action == "notes":
        print(release_notes)

    elif action == "draft":
        create_draft_release(tag, release_notes)

    elif action == "news":
        rst_text = markdown_to_rst(release_notes)
        update_news_file(rst_text, release_version, release_date)

    elif action == "candidate":
        html_notes = markdown_to_html(release_notes)
        date_string = release_date.strftime("%A, %B %e")
        url = get_repo_url()
        project = url.split('/')[-1]
        if project == "openexr":
            project = "OpenEXR"
        print(f"{project} {release_version} is staged for release at tag <a href={url}/releases/tag/{tag}>{tag}</a> and will be released officially {date_string} barring any issues. <br><br> {html_notes}")

    elif action == "tag":
        cmd_tag(tag, release_date, release_notes)
        
if __name__ == "__main__":
    main()
