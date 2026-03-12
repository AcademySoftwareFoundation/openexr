#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# Helper script that automates parts of the release process:
#
# `release.py notes <tag>` - extract release notes from CHANGES.md for the tagged release, print to stdout
# `release.py news <tag>` - edit the website/news.rst file to add reference to the tagged release
# `release.py draft <tag>` - create a draft release on GitHub
# `release.py candidate <tag>` - format a message about the upcoming release, print to stdout
# `release.py pr-changes <label>` - list merged PRs with label (e.g. v3.4.7), print merge SHAs for
#   cherry-picking, and add a Version section and Merged Pull Requests list to CHANGES.md

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
from subprocess import PIPE, run
from datetime import datetime, timedelta
import markdown

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
    # Replace markdown links [text](url) with reST format `text <url>`_
    markdown_text = re.sub(
        r'\[([\s\S]*?)\]\(([\s\S]*?)\)',
        r'`\1 <\2>`_',
        markdown_text
    )

    # Convert `text to ``text`` (but skip existing ``)
    markdown_text = re.sub(r'(?<!`)`([^`]+)`(?!`)', r'``\1``', markdown_text)

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
        result = run(['git', 'config', '--get', 'remote.origin.url'],
                     stdout=PIPE, stderr=PIPE, universal_newlines=True)
        url = result.stdout.strip()
        return url
    except subprocess.CalledProcessError:
        return None  # Not a git repo or no origin remote

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


def get_merged_prs_by_label(label):
    """
    Return merged PRs that have the given GitHub label (e.g. 'v3.4.7').

    Args:
        label: GitHub label string; only PRs with this label and state 'merged' are returned.

    Returns:
        List of dicts, each with "number" (int), "title" (str), "merge_sha" (str or None).
        Sorted by PR number in decreasing order.
    """
    result = run(
        [
            "gh",
            "pr",
            "list",
            "--label", label,
            "--state", "merged",
            "--json", "number,title,mergeCommit",
            "--limit", "100",
        ],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        sys.exit(1)
    prs = json.loads(result.stdout)
    out = []
    for pr in prs:
        merge_sha = None
        if pr.get("mergeCommit"):
            merge_sha = pr["mergeCommit"].get("oid")
        out.append({
            "number": pr["number"],
            "title": pr["title"].strip(),
            "merge_sha": merge_sha,
        })
    out.sort(key=lambda x: x["number"], reverse=True)
    return out


def get_repo_name_with_owner():
    """Return 'owner/repo' for the current repo using gh."""
    result = run(
        ["gh", "repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return "AcademySoftwareFoundation/openexr"
    return result.stdout.strip()


def version_section_anchor(version, date_str):
    """
    Build the markdown anchor for a version section heading.
    Matches the format used in the CHANGES.md TOC (e.g. #version-346-march-1-2026).

    Args:
        version: Version string (e.g. '3.4.7' or '3.0.1-beta').
        date_str: Date string as in the heading (e.g. 'March 1, 2026').

    Returns:
        Anchor string without the leading '#', for use in [text](#anchor).
    """
    version_part = version.replace(".", "")
    date_part = date_str.lower().replace(",", "").replace(" ", "-")
    return f"version-{version_part}-{date_part}"


def update_toc(lines, version, date_str):
    """
    Add or update the TOC entry at the top of CHANGES.md for this version.
    TOC lines look like: * [Version 3.4.6](#version-346-march-1-2026) March 1, 2026.
    If an entry for this version exists, update its date and anchor. Otherwise
    insert a new entry at the top of the TOC list (newest first).

    Args:
        lines: Full list of lines from CHANGES.md.
        version: Version string (e.g. '3.4.7').
        date_str: Date string for the entry (e.g. 'March 1, 2026').

    Returns:
        New list of lines with the TOC updated.
    """
    anchor = version_section_anchor(version, date_str)
    new_entry = f"* [Version {version}](#{anchor}) {date_str}"
    toc_entry_re = re.compile(r"^\*\s+\[Version ([^\]]+)\]\(#[^)]+\)\s+.+$")
    content_start = None
    toc_first = None
    for i, line in enumerate(lines):
        if line.startswith("## Version "):
            content_start = i
            break
        if toc_first is None and toc_entry_re.match(line):
            toc_first = i
    if content_start is None or toc_first is None:
        return lines
    for i in range(toc_first, content_start):
        m = toc_entry_re.match(lines[i])
        if m and m.group(1) == version:
            lines = lines[:i] + [new_entry] + lines[i + 1:]
            return lines
    lines = lines[:toc_first] + [new_entry] + lines[toc_first:]
    return lines


def find_existing_version_section(lines, version):
    """
    If CHANGES.md already has a section for this version, return its extent,
    the index of the ### Merged Pull Requests line (if any), and the list of
    PRs already in the Merged Pull Requests list.

    Args:
        lines: List of lines from CHANGES.md (e.g. content.splitlines()).
        version: Version string without leading 'v' (e.g. '3.4.7'). Must match
            the heading after "## Version ".

    Returns:
        If the section exists: (header_line_index, merged_pr_index, section_end_index, existing_pr_list).
        merged_pr_index is the line index of "### Merged Pull Requests" or None.
        existing_pr_list is a list of dicts with keys "number" (int) and "title" (str),
        one per PR in the section, in file order. Section extent is [header_idx, section_end_idx).
        If the section does not exist: (None, None, None, []).
    """
    version_heading = re.compile(r"^##\s+Version\s+" + re.escape(version) + r"\s*(\s|\(|$)", re.IGNORECASE)
    merged_pr_heading = re.compile(r"^###\s+Merged\s+Pull\s+Requests", re.IGNORECASE)
    pr_line_pattern = re.compile(r"^\*\s+\[(\d+)\]")
    header_idx = None
    merged_pr_idx = None
    section_end_idx = None
    existing_pr_list = []

    i = 0
    while i < len(lines):
        line = lines[i]
        if header_idx is None:
            if version_heading.match(line):
                header_idx = i
            i += 1
            continue
        if line.startswith("## "):
            section_end_idx = i
            break
        if merged_pr_heading.match(line):
            merged_pr_idx = i
        m = pr_line_pattern.match(line)
        if m:
            num = int(m.group(1))
            title = ""
            if i + 1 < len(lines) and lines[i + 1].strip() and not lines[i + 1].startswith("* "):
                title = lines[i + 1].strip()
                i += 1
            existing_pr_list.append({"number": num, "title": title})
        i += 1
    if header_idx is not None and section_end_idx is None:
        section_end_idx = len(lines)
    if header_idx is None:
        return (None, None, None, [])
    return (header_idx, merged_pr_idx, section_end_idx, existing_pr_list)


def update_existing_section(
    lines, header_idx, merged_pr_idx, section_end_idx, body_lines, existing_pr_list, prs, repo_slug, release_date
):
    """
    Update an existing Version section: set the header date to release_date,
    retain body_lines (text between header and ### Merged Pull Requests), and
    replace the Merged Pull Requests list with the merge of existing PRs and
    prs (from the API), sorted by PR number in decreasing order.

    Args:
        lines: Full list of lines from CHANGES.md.
        header_idx: Line index of the "## Version X (date)" heading.
        merged_pr_idx: Line index of "### Merged Pull Requests" or None.
        section_end_idx: Line index where this section ends (next ## or end of file).
        body_lines: Lines to retain between the header and ### Merged Pull Requests
            (typically lines[header_idx+1 : merged_pr_idx] or []).
        existing_pr_list: List of dicts with "number" and "title" for PRs already
            in the section (from find_existing_version_section).
        prs: List of dicts with "number", "title", "merge_sha" from the API (e.g.
            get_merged_prs_by_label). Used as the source of truth for titles when
            a PR appears in both.
        repo_slug: GitHub 'owner/repo' string for building PR URLs.
        release_date: datetime to write in the version heading.

    Returns:
        New list of lines for CHANGES.md.
    """
    version_header_pattern = re.compile(r"^(\s*##\s+Version\s+[^\s(]+)(?:\s*\([^)]*\))?(\s*)$")
    header_line = lines[header_idx]
    m = version_header_pattern.match(header_line)
    date_str = release_date.strftime("%B %d, %Y")
    if m:
        new_header = m.group(1) + f" ({date_str})" + m.group(2)
    else:
        new_header = header_line
    existing_by_number = {p["number"]: p for p in existing_pr_list}
    api_by_number = {p["number"]: p for p in prs}
    all_numbers = sorted(set(existing_by_number) | set(api_by_number), reverse=True)
    base_url = f"https://github.com/{repo_slug}/pulls"
    merged_list_lines = [
        "### Merged Pull Requests:",
        "",
    ]
    for num in all_numbers:
        pr = api_by_number.get(num) or existing_by_number.get(num)
        merged_list_lines.append(f"* [{pr['number']}]({base_url}/{pr['number']})")
        merged_list_lines.append(pr["title"])
        merged_list_lines.append("")
    new_lines = (
        lines[:header_idx]
        + [new_header]
        + body_lines
        + merged_list_lines
        + lines[section_end_idx:]
    )
    return new_lines


def add_release_section_to_changes(label, prs, repo_slug, release_date):
    """
    Add or update a Version section and Merged Pull Requests list in CHANGES.md.
    If the section already exists, retain any text between the section header
    and the ### Merged Pull Requests subsection, merge in any new PRs (not
    already listed), re-sort the full list by PR number descending, and set
    the section date to release_date. Otherwise insert a new section before
    the first ## Version line.

    Args:
        label: GitHub label string (e.g. 'v3.4.7'). The version heading uses
            label with leading 'v' stripped.
        prs: List of dicts with keys "number" (int), "title" (str), "merge_sha"
            (str or None), e.g. from get_merged_prs_by_label(label). Merged PRs
            are listed in decreasing order by number.
        repo_slug: GitHub 'owner/repo' string used to build PR URLs.
        release_date: datetime used in the "## Version X (date)" heading.

    Returns:
        Section header plus any body text up to (but not including) the
        ### Merged Pull Requests subsection. For a new section, just the
        header line.
    """
    version = label.lstrip("v")
    date_str = release_date.strftime("%B %d, %Y")
    with open("CHANGES.md", "r", encoding="utf-8") as f:
        content = f.read()
    lines = content.splitlines()

    header_idx, merged_pr_idx, section_end_idx, existing_pr_list = find_existing_version_section(
        lines, version
    )
    if header_idx is not None:
        version_header_pattern = re.compile(r"^(\s*##\s+Version\s+[^\s(]+)(?:\s*\([^)]*\))?(\s*)$")
        header_line = lines[header_idx]
        m = version_header_pattern.match(header_line)
        if m:
            new_header = m.group(1) + f" ({date_str})" + m.group(2)
        else:
            new_header = header_line
        if merged_pr_idx is not None:
            body_lines = lines[header_idx + 1 : merged_pr_idx]
        else:
            body_lines = []
        section_header_and_body = new_header + "\n" + "\n".join(body_lines)
        if body_lines:
            section_header_and_body += "\n"
        new_lines = update_existing_section(
            lines,
            header_idx,
            merged_pr_idx,
            section_end_idx,
            body_lines,
            existing_pr_list,
            prs,
            repo_slug,
            release_date,
        )
        new_lines = update_toc(new_lines, version, date_str)
        with open("CHANGES.md", "w", encoding="utf-8") as f:
            f.write("\n".join(new_lines) + "\n")
        return section_header_and_body

    insert_at = None
    for i, line in enumerate(lines):
        if re.match(r"^## Version ", line):
            insert_at = i
            break
    if insert_at is None:
        print("Could not find '## Version' in CHANGES.md", file=sys.stderr)
        sys.exit(1)
    section = [
        "",
        f"## Version {version} ({date_str})",
        "",
        "",
        "### Merged Pull Requests:",
        "",
    ]
    base_url = f"https://github.com/{repo_slug}/pulls"
    for pr in sorted(prs, key=lambda x: x["number"], reverse=True):
        section.append(f"* [{pr['number']}]({base_url}/{pr['number']})")
        section.append(pr["title"])
        section.append("")
    section.append("")
    new_lines = lines[:insert_at] + section + lines[insert_at:]
    new_lines = update_toc(new_lines, version, date_str)
    with open("CHANGES.md", "w", encoding="utf-8") as f:
        f.write("\n".join(new_lines) + "\n")
    return f"## Version {version} ({date_str})"


def main():
    if len(sys.argv) < 3:
        print("Usage: python release.py <notes|news|draft|candidate|pr-changes> <tag-or-label> [date]")
        sys.exit(1)

    action = sys.argv[1]
    tag = sys.argv[2]
    # Strip leading 'v' and trailing '-rc<candidate>' if necessary
    base_tag = tag.lstrip('v').split('-rc')[0]

    if action == "pr-changes":
        prs = get_merged_prs_by_label(tag)
        if not prs:
            print(f"No merged PRs found with label {tag}", file=sys.stderr)
            sys.exit(1)
        for pr in prs:
            if pr["merge_sha"]:
                print(f"git cherry-pick {pr['merge_sha'][:7]} # {pr['number']} {pr['title']}")
            else:
                print(f"(PR #{pr['number']}: no merge commit)", file=sys.stderr)
        release_date = datetime.now() + timedelta(days=3)
        repo_slug = get_repo_name_with_owner()
        section_text = add_release_section_to_changes(tag, prs, repo_slug, release_date)
        print(section_text)
        sys.exit(0)

    if len(sys.argv) < 3:
        print("Usage: python release.py <notes|news|draft|candidate> <tag> [date]")
        sys.exit(1)

    result = run(['git', 'tag', '--list', tag], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    if result.stdout == "":
        tag += "-rc"
        result = run(['git', 'tag', '--list', tag], stdout=PIPE, stderr=PIPE, universal_newlines=True)
        if result.stdout != "":
            print(f"Using {tag} instead...")
        else:
            print(f"No such tag: {tag}")
            sys.exit(1)

    # Get the content of the CHANGES.md at the specified git tag
    try:
        result = run(['git', 'show', f"{tag}:CHANGES.md"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
        content = result.stdout
        if content == None or content == "":
            print(f"No news.txt at tag {tag}")
            sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

    # Extract the release notes
    release_date, release_notes = extract_section(content, base_tag)
    if release_date == None:
        print("No release found.")
        return

    if action == "notes":
        print(release_notes)

    elif action == "draft":
        create_draft_release(tag, release_notes)

    elif action == "news":
        rst_text = markdown_to_rst(release_notes)
        update_news_file(rst_text, base_tag, release_date)

    elif action == "candidate":
        version = base_tag
        html_notes = markdown_to_html(release_notes)
        date_string = release_date.strftime("%A, %B %e")
        url = get_repo_url()
        project = url.split('/')[-1]
        if project == "openexr":
            project = "OpenEXR"
        print(f"{project} {version} is staged for release at tag <a href={url}/releases/tag/{tag}>{tag}</a> and will be released officially {date_string} barring any issues. <br><br> {html_notes}")

if __name__ == "__main__":
    main()
