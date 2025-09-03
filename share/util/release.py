#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# Helper script that automates parts of the release proces:
#
# `release.py notes <tag>` - extract release notes from CHANGES.md for the tagged release, print to stdout
# `release.py news <tag>` - edit the website/news.rst file to add reference to the tagged release
# `release.py draft <tag>` - create a draft release on GitHub
# `release.py candidate <tag>` - format a message about the upcoming release, print to stdout

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

import sys
import re
from subprocess import PIPE, run
from datetime import datetime
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
    # Replace markdown links [text](url) with reST format `text <url>`_
    markdown_text = re.sub(r'\[(.*?)\]\((.*?)\)', r'`\1 <\2>`_', markdown_text)

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
    old_section_title = result.stdout.split("replace:: ")[1].rstrip().lstrip('**').rstrip('**')

    # Write the news section title to the latest_news_title.rst file,
    # so it can be include on the website front page
    with open('website/latest_news_title.rst', 'w') as f:
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

def main():
    if len(sys.argv) < 3:
        print("Usage: python release.py <notes|news|draft|candidate> <tag> [date]")
        sys.exit(1)

    action = sys.argv[1]
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
