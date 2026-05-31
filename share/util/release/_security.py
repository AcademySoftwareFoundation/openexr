# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""CVE and OSS-Fuzz reference scanning for release notes (used by changes and log)."""

from __future__ import annotations

import json
import re
import sys
from subprocess import PIPE, run
from urllib.error import HTTPError, URLError
from urllib.request import urlopen

OSS_FUZZ_ISSUE_JSON_PREFIX = ")]}'"
OSS_FUZZ_ISSUE_API = "https://issues.oss-fuzz.com/action/issues/{issue_id}?format=json"

PR_LINE_ADDRESS_RE = re.compile(r"(?i)\bAddress(?:es|ed|ing)?\b")
CVE_ID_RE = re.compile(r"CVE-\d{4}-\d+", re.IGNORECASE)
OSS_FUZZ_ISSUE_RE = re.compile(
    r"https://issues\.oss-fuzz\.com/issues/(\d+)", re.IGNORECASE
)


def _collect_addressed_security_refs(
    blocks: list[str],
) -> tuple[list[str], list[str]]:
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
    parts = full_title.split(":", 2)
    if len(parts) >= 3:
        return parts[2].strip()
    return full_title.strip()


def _oss_fuzz_full_title_from_fetch(data, issue_id: str) -> str | None:
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
    if not pr_number:
        return [], []
    return _collect_addressed_security_refs(_gh_pr_text_blocks(pr_number))


def gh_security_advisories_cve_titles() -> dict[str, str]:
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


def collect_security_refs_for_prs(
    pr_numbers: list[str],
) -> tuple[list[str], list[str]]:
    cves: list[str] = []
    oss_fuzz_issues: list[str] = []
    for pr_number in pr_numbers:
        pr_cves, pr_oss_fuzz = pr_addressed_cves(pr_number)
        cves.extend(pr_cves)
        oss_fuzz_issues.extend(pr_oss_fuzz)
    return cves, oss_fuzz_issues
