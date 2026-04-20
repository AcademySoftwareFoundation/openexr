#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# Helper script that automates parts of the release proces:
#
# `release.py notes <tag>` - extract release notes from CHANGES.md for the tagged release, print to stdout
# `release.py news <tag>` - edit the website/news.rst file to add reference to the tagged release
# `release.py draft <tag>` - create a draft release on GitHub
# `release.py candidate <tag>` - format a message about the upcoming release, print to stdout
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
                        "Merged Pull Requests" in line):
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

    print(f"share/util/release.py changes {label} {changes_prs}")

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
            str(pr_number),
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


MERGED_PR_HEADING_RE = re.compile(
    r"^###\s+Merged Pull Requests\s*:?\s*$", re.IGNORECASE
)
MERGED_WORKFLOW_HEADING_RE = re.compile(
    r"^###\s+Merged Workflow Pull Requests\s*:?\s*$", re.IGNORECASE
)
PR_BULLET_RE = re.compile(r"^\*\s*\[(\d+)\]\(")


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
            pr = int(mo.group(1))
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
        merged_prs, i = _parse_pr_blocks_dict(lines, i, stop_at_workflow_heading=True)

    merged_workflow_prs = {}
    if i < n and MERGED_WORKFLOW_HEADING_RE.match(lines[i].strip()):
        i += 1
        merged_workflow_prs, i = _parse_pr_blocks_dict(lines, i, stop_at_workflow_heading=False)

    return heading, merged_prs, merged_workflow_prs

def pr_is_workflow_only(pr_number: int) -> bool:
    """Return True if every file changed by the PR is under .github/workflows/."""
    result = run(
        ["gh", "pr", "view", str(pr_number), "--json", "files",
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

    release_date = datetime.now() + timedelta(days=3)
    date_str = release_date.strftime("%B %e, %Y")

    if section_index is not None:
        # Parse the existing section into the heading (section heading
        # and existing notes), the dict of merged PRs, and the dict of merge workflow PRs
        section_heading, merged_prs, merged_workflow_prs = parse_section(
            lines[section_index:footer_index]
        )
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
        f.write("\n### Merged Pull Requests\n\n")
        for pr, value in sorted(merged_prs.items(), reverse=True):
            f.write(value + "\n")
        f.write("\n### Merged Workflow Pull Requests\n\n")
        for pr, value in sorted(merged_workflow_prs.items(), reverse=True):
            f.write(value + "\n")
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

    Return the release-related labels for the identified PR, i.e.
    labels of the form 'v<major>.<minor>.<patch>'
    """

    LOG_PR_SUFFIX_RE = re.compile(r"\(#(\d+)\)\s*$")
    m = LOG_PR_SUFFIX_RE.search(line)
    if not m:
        return ""
    pr_number = m.group(1)
    result = run(
        ["gh", "pr", "view", pr_number, "--json", "labels"],
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True,
        check=False,
    )
    if result.returncode != 0:
        return ""
    try:
        data = json.loads(result.stdout or "{}")
    except json.JSONDecodeError:
        return ""
    out = []
    for lab in data.get("labels") or []:
        name = lab.get("name") or ""
        if name.startswith("v"):
            out.append(name)
    return " ".join(out)

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
        l = pr_labels(line)
        print(f"{l:7} {line}")

def main():

    if len(sys.argv) > 1:
        action = sys.argv[1]
        if action == "log":
            cmd_log()
            return

    if len(sys.argv) < 2:
        print(
            "Usage: python release.py "
            "<notes|news|draft|candidate|cherry|changes> ...\n"
            "  release.py changes <tag> <pr-number>   e.g. release.py changes v3.4.7 1234"
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
            "<notes|news|draft|candidate|cherry|changes> <tag-or-label> [date]"
        )
        sys.exit(1)

    tag = sys.argv[2]

    # Strip leading 'v' and trailing '-rc<candidate>' if necessary
    base_tag = tag.lstrip('v').split('-rc')[0]
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
        print(f"OpenEXR {version} is staged for release at tag <a href=https://github.com/AcademySoftwareFoundation/openexr/releases/tag/{tag}>{tag}</a> and will be released officially {date_string} barring any issues. <br><br> {html_notes}")

if __name__ == "__main__":
    main()
