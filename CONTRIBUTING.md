<!-- SPDX-License-Identifier: BSD-3-Clause -->
<!-- Copyright (c) Contributors to the OpenEXR Project -->

# Contributing to OpenEXR

Thank you for your interest in contributing to OpenEXR. This document
explains our contribution process and procedures:

* [Getting Information](#Getting-Information)
* [Legal Requirements](#Legal-Requirements)
* [Development Workflow](#Development-Workflow)
* [Coding Style](#Coding-Style)
* [Versioning Policy](#Versioning-Policy)
* [Contributing to the Website](#Contributing-to-the-Website)
* [Creating a Patch Release](#Creating-a-Patch-Release)
* [Creating a Major/Minor Release](#Creating-a-Major/Minor-Release)

For a description of the roles and responsibilities of the various
members of the OpenEXR community, see [GOVERNANCE](GOVERNANCE.md), and
for further details, see the OpenEXR project's [Technical
Charter](ASWF/charter/OpenEXR-Technical-Charter.md). Briefly,
a "contributor" is anyone who submits content to the project, a
"committer" reviews and approves such submissions, and the "Technical
Steering Committee" provides general project oversight and governance.

## Getting Information

There are two primary ways to connect with the OpenEXR project:

* The [openexr-dev](https://lists.aswf.io/g/openexr-dev) mail list:
  This is a development focused mail list with a deep history of
  technical conversations and decisions that have shaped the project.

* [GitHub Issues](https://github.com/AcademySoftwareFoundation/openexr/issues): GitHub
  Issues are used both to track bugs and to discuss feature requests.

### How to Ask for Help

If you have trouble installing, building, or using the library, but
there's not yet reason to suspect you've encountered a genuine bug,
start by posting a question to the
[openexr-dev](http://lists.aswf.io/g/openexr-dev) mailing list. This is
the place for question such has "How do I...".

### How to Report a Bug

OpenEXR use GitHub's issue tracking system for bugs and enhancements:
https://github.com/AcademySoftwareFoundation/openexr/issues

If you are submitting a bug report, please be sure to note which
version of OpenEXR you are using, on what platform (OS/version, which
compiler you used, and any special build flags or other unusual
environmental issues). Please give a specific account of

* what you tried
* what happened
* what you expected to happen instead

with enough detail that others can reproduce the problem.

### How to Request a Change

Open a GitHub issue: https://github.com/AcademySoftwareFoundation/openexr/issues.

Describe the situation and the objective in as much detail as
possible. Feature requests will almost certainly spawn a discussion
among the project community.

### How to Report a Security Vulnerability

If you think you've found a potential vulnerability in OpenEXR, please
refer to [SECURITY.md](SECURITY.md) to responsibly disclose it.

### How to Contribute a Bug Fix or Change

To contribute code to the project, first read over the [GOVERNANCE](GOVERNANCE.md) page to understand the roles involved. You'll need:

* A good knowledge of git.

* A fork of the GitHub repo.

* An understanding of the project's development workflow.

* Legal authorization, that is, you need to have signed a Contributor
  License Agreement. See below for details.

## Legal Requirements

OpenEXR is a project of the Academy Software Foundation and follows the
open source software best practice policies of the Linux Foundation.

### License

OpenEXR is licensed under the [BSD-3-Clause](LICENSE.md)
license. Contributions to the library should abide by that standard
license.

### Contributor License Agreements

Developers who wish to contribute code to be considered for inclusion
in the OpenEXR distribution must first complete a **Contributor
License Agreement**.

To contribute to OpenEXR, you must sign a CLA through the
[EasyCLA](https://docs.linuxfoundation.org/lfx/easycla)
system, which is integrated with GitHub as a pull request check.

If a contributor opens a pull request without having a CLA on file, the 
contributor will be guided through the process to have the appropriate 
CLA signed. Look in the PR comments for the "linux-foundation-easycla" 
check that would fail, and a red "NOT COVERED" button will appear in the PR
comments; click the link in the comment to sign the CLA. For organizations, 
you can alternatively go to [this
link](https://organization.lfx.linuxfoundation.org/foundation/a09410000182dD2AAI/project/a092M00001If9ujQAB/cla)
prior to submitting a pull request, which will guide you through the 
process to have a CLA signed on behalf of the organization.

* If you are an individual writing the code on your own time and
  you're **sure** you are the sole owner of any intellectual property you
  contribute, you can sign the CLA as an **Individual Contributor**. If you
  are unsure, please contact your employer for clarity.

* If you are writing the code as part of your job, or if your employer
  retains ownership to intellectual property you create, no matter how
  small, then your company's legal affairs representatives should sign
  a **Corporate Contributor Licence Agreement**. If your company already
  has a signed CCLA on file, ask your local CLA manager to add you
  (via your GitHub account name/email address) to your company's
  "approved" list.

The downloadable PDF's on the EasyCLA page are provided for reference
only. To execute the signature, sign the form online through the
relevant links.

The OpenEXR CLAs are the standard forms used by the Linux Foundation
projects and [recommended by the ASWF
TAC](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contributor-license-agreement-cla). 
Note that if you have signed a CLA for a different ASWF or LF project, that 
CLA doesn't apply here and you need to sign for this project specifically.

### Commit Sign-Off

Every commit must be signed off.  That is, every commit log message
must include a “`Signed-off-by`” line (generated, for example, with
“`git commit --signoff`”), indicating that the committer wrote the
code and has the right to release it under the [BSD-3-Clause](LICENSE.md)
license. See https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contribution-sign-off for more information on this requirement.

## Development Workflow

### Git Basics

Working with OpenEXR requires understanding a significant amount of
Git and GitHub based terminology. If you’re unfamiliar with these
tools or their lingo, please look at the [GitHub
Glossary](https://help.github.com/articles/github-glossary/) or browse
[GitHub Help](https://help.github.com/).

To contribute, you need a GitHub account. This is needed in order to
push changes to the upstream repository, via a pull request.

You will also need Git installed on your local development machine. If
you need setup assistance, please see the official [Git
Documentation](https://git-scm.com/doc).

### Repository Structure and Commit Policy

The OpenEXR repository uses a simple branching and merging strategy.

All development work is done directly on the ``main`` branch. The ``main``
branch represents the bleeding-edge of the project and most
contributions should be done on top of it.

After sufficient work is done on the ``main`` branch and the OpenEXR
leadership determines that a release is due, we will bump the relevant
internal versioning and tag a commit with the corresponding version
number, e.g. v2.0.1. Each minor version also has its own “Release
Branch”, e.g. RB-1.1. This marks a branch of code dedicated to that
``major.minor version``, which allows upstream bug fixes to be
cherry-picked to a given version while still allowing the ``main``
branch to continue forward onto higher versions. This basic repository
structure keeps maintenance low, while remaining simple to understand.

To reiterate, the ``main`` branch represents the latest development
version, so beware that it may include untested features and is not
generally stable enough for release.  To retrieve a stable version of
the source code, use one of the release branches.

### The Git Workflow

This development workflow is sometimes referred to as
[OneFlow](https://www.endoflineblog.com/oneflow-a-git-branching-model-and-workflow). It
leads to a simple, clean, linear edit history in the repo.

The OpenEXR GitHub repo allows rebase merging and disallows merge
commits and squash merging. This ensures that the repo edit history
remains linear, avoiding the "bubbles" characteristic of the
[GitFlow](https://www.endoflineblog.com/gitflow-considered-harmful)
workflow.

### Use the Fork, Luke

In a typical workflow, you should **fork** the OpenEXR repository to
your account. This creates a copy of the repository under your user
namespace and serves as the “home base” for your development branches,
from which you will submit **pull requests** to the upstream
repository to be merged.

Once your Git environment is operational, the next step is to locally
**clone** your forked OpenEXR repository, and add a **remote**
pointing to the upstream OpenEXR repository. These topics are
covered in the GitHub documentation [Cloning a
repository](https://help.github.com/articles/cloning-a-repository/)
and [Configuring a remote for a
fork](https://help.github.com/articles/configuring-a-remote-for-a-fork/),
but again, if you need assistance feel free to reach out on the
openexr-dev@lists.aswf.io mail list.

### Pull Requests

Contributions should be submitted as GitHub pull requests. See
[Creating a pull request](https://help.github.com/articles/creating-a-pull-request/)
if you're unfamiliar with this concept.

The development cycle for a code change should follow this protocol:

1. Create a topic branch in your local repository, following the naming format
"feature/<your-feature>" or "bugfix/<your-fix>".

2. Make changes, compile, and test thoroughly. Code style should match existing
style and conventions, and changes should be focused on the topic the pull
request will be addressing. Make unrelated changes in a separate topic branch
with a separate pull request.

3. Push commits to your fork.

4. Create a GitHub pull request from your topic branch.

5. Pull requests will be reviewed by project committers and contributors,
who may discuss, offer constructive feedback, request changes, or approve
the work.

6. Upon receiving the required number of committer approvals (as
outlined in [Required Approvals](#required-approvals)), a committer
other than the PR contributor may merge changes into the ``main``
branch.

### Code Review and Required Approvals

Modifications of the contents of the OpenEXR repository are made on a
collaborative basis. Anyone with a GitHub account may propose a
modification via pull request and it will be considered by the project
committers.

Pull requests must meet a minimum number of committer approvals prior
to being merged. Rather than having a hard rule for all PRs, the
requirement is based on the complexity and risk of the proposed
changes, factoring in the length of time the PR has been open to
discussion. The following guidelines outline the project's established
approval rules for merging:

* Core design decisions, large new features, or anything that might be
perceived as changing the overall direction of the project should be
discussed at length in the mail list or TSC meetings before any PR is
submitted, in order to solicit feedback, try to get as much consensus
as possible, and alert all the stakeholders to be on the lookout for
the eventual PR when it appears.

* Trivial changes that don't affect functionality (typos, tests, website)
can be approved by the committer without review, after waiting at
least 48 hours.

* Big changes that can alter behavior, add major features, or present
a high degree of risk should be signed off by TWO committers, ideally
one of whom should be the "owner" for that section of the codebase (if
a specific owner has been designated). If the person submitting the PR
is him/herself the "owner" of that section of the codebase, then only
one additional committer approval is sufficient. But in either case, a
48 hour minimum is helpful to give everybody a chance to see it,
unless it's a critical emergency fix (which would probably put it in
the previous "small fix" category, rather than a "big feature").

* Escape valve: big changes can nonetheless be merged by a single
committer if the PR has been open for over two weeks without any
unaddressed objections from other committers. At some point, we have
to assume that the people who know and care are monitoring the PRs and
that an extended period without objections is really assent.

Approval must be from committers who are not authors of the change. If
one or more committers oppose a proposed change, then the change
cannot be accepted unless:

* Discussions and/or additional changes result in no committers
objecting to the change. Previously-objecting committers do not
necessarily have to sign-off on the change, but they should not be
opposed to it.

* The change is escalated to the TSC and the TSC votes to approve the
change.  This should only happen if disagreements between committers
cannot be resolved through discussion.

Committers may opt to elevate significant or controversial
modifications to the TSC by assigning the `tsc-review` label to a pull
request or issue. The TSC should serve as the final arbiter where
required.

### Test Policy

All functionality in the library must be covered by an automated
test. Each library has a companion ``Test`` project, e.g. ``OpenEXRTest``,
``OpenEXRCoreTest``, ``OpenEXRUtilTest``, etc.  This test suite is collectively
expected to validate the behavior of very part of the library.

* All new functionality should be accompanied by a test that validates
  its behavior.

* Any change to existing functionality should have tests added if they
  don't already exist.

The test should should be run, via:

    make test

before submitting a pull request.

In addition, the ``OpenEXRFuzzTest`` project validates the library by
feeding it corrupted input data. This test is time-consuming (possible
over 24 hours), so it will only be run occasionally, but it must
succeed before a release is made.

## Coding Style

#### Formatting

The coding style of the library source code is enforced via Clang format, with the configuration defined in [.clang-format](.clang-format).

When modifying existing code, follow the surrounding formatting
conventions so that new or modified code blends in with the current
code.

* Indent with spaces, never tabs. Each indent level should be 4 spaces.

* Function return types go on a separate line:

        const float &
        Header::pixelAspectRatio () const
        {
            ...
        }

* Use a space between function names and the following parentheses
  (although you can eliminate the space for functions with no
  arguments):

        void
        Header::insert (const string& name, const Attribute& attribute)
        {
            insert (name.c_str(), attribute);
        }

* Place curly braces on their own lines:

        void
        RgbaOutputFile::ToYca::padTmpBuf ()
        {
            for (int i = 0; i < N2; ++i)
            {
                _tmpBuf[i] = _tmpBuf[N2];
                _tmpBuf[_width + N2 + i] = _tmpBuf[_width + N2 - 2];
            }
        }

#### Naming Conventions

* In general, classes and template type names should start with upper
  case and capitalize new words: `class CustomerList;`

* In general, local variables should use camelCase. Macros and
  constants should use `ALL_CAPS`.

* Member fields in a class should start with an underscore. No other
  variables should begin with underscore.

#### File conventions

C++ implementation should be named `*.cpp`. Headers should be named `.h`.

All headers should contain:

    #pragma once

#### Type Conventions

Because OpenEXR must deal properly with large images, whose width
and/or height approach the maximum allowable in 32-bit signed
integers, take special care that integer arithmetic doesn't overflow,
and make it as clear as possible exactly what the code is doing,
especially in the edge cases.

To clarify the intention, prefer to cast between types using
``static_cast<>()`` rather than the basic C-style ``()`` notation:

    // good:
    size_t x = static_cast <size_t> (y);

    // bad:
    x = (size_t) y;
    x = size_t (y);

Prefer to use ``std::numeric_limits<>`` instead of preprocessor
define's such as ``INT_MAX``:

    // good:
    if (x > std::numeric_limits<int>::max())
        std::cout << "That's too freakin' high.\n";

    // bad:
    if (x > INT_MAX)

#### Copyright Notices

All new source files should begin with a copyright and license stating:

    //
    // SPDX-License-Identifier: BSD-3-Clause
    // Copyright (c) Contributors to the OpenEXR Project. 
    //
    
#### Third-party libraries

Prefer C++11 `std` over boost where possible.  Use boost classes you
already see in the code base, but check with the project leadership
before adding new boost usage.

#### Comments and Doxygen

Comment philosophy: try to be clear, try to help teach the reader
what's going on in your code.

Prefer C++ comments (starting line with `//`) rather than C comments
(`/* ... */`).

For public APIs, use Doxygen-style comments (start with `///`), such as:

    /// Explanation of a class.  Note THREE slashes!
    /// Also, you need at least two lines like this.  If you don't have enough
    /// for two lines, make one line blank like this:
    ///
    class myclass {
        ....
        float foo;  ///< Doxygen comments on same line look like this
    }

## Versioning Policy

OpenEXR uses [semantic versioning](https://semver.org), which labels
each version with three numbers: ``major.minor.patch``, where:

* ``major`` - indicates incompatible API changes
* ``minor`` - indicates functionality added in a backwards-compatible manner
* ``patch`` - indicates backwards-compatible bug fixes

## Contributing to the Website

The https://openexr.com website is generated via
[Sphinx](https://www.sphinx-doc.org) with the
[Breathe](https://breathe.readthedocs.io) extension, using the
[sphinx-press-theme](https://pypi.org/project/sphinx-press-theme), and
is hosted by
[readthedocs](https://readthedocs.org/projects/openexr). The website
source is in [restructured
text](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html)
in the ``website`` directory.

To build the website locally from the source ``.rst`` files, set the
CMake option ``BUILD_WEBSITE=ON``. This adds the ``website`` CMake
target. Generation is off by default.

Building the website requires that ``sphinx``, ``breathe``, and
``doxygen`` are installed. It further requires the [sphinx-press-theme]
(https://pypi.org/project/sphinx-press-theme). Complete dependencies
are described in the [requirements.txt](website/requirements.txtg)
file.

On Debian/Ubuntu Linux:

.. code-block::

    % apt-get install doxygen python3-sphinx
    % pip3 install breathe
    % pip3 install sphinx_press_theme
   
    % mkdir _build
    % cd _build
    % cmake .. -DBUILD_WEBSITE=ON
    % cmake --build . --target website 

### Testing a Website Build

When you have configured cmake with ``BUILD_WEBSITE=ON`` and done a
build, you should find a file ``website/sphinx/index.html`` in the
build directory which is the website source. Load this file in a
browser to preview the resulting website, that is, load
``file://<build-directory>/website/sphinx/index.html`` into your web
browser.

Once you submit a PR, a check labeled ``docs/readthedocs.org:openexr``
will validate the build. Click on the ``Details`` link to
preview. Also, a link to this preview will be added automatically to
the PR description.

### Test Images

To contribute a new test image, commit it to the
[openexr-images](https://github.com/AcademySoftwareFoundation/openexr-images)
repo, along with an associated ``.jpg`` file for display on the
website.

The [website/scripts/test_images.py](website/scripts/test_images.py)
utility processes images from
[openexr-images](https://github.com/AcademySoftwareFoundation/openexr-images)
to produce `.rst` files for input to Sphinx. It runs ``exrheader`` on
the ``.exr`` to generate the image description. It also processes
``README`` files in the image repo for additional website content,
useful for describing a collection of images. Once the new image is in
the ``openexr-images`` repo, run ``website/scripts/test_images.py``,
then commit the new/modified ``.rst`` files to git and submit a PR.

## Creating a Patch Release

These instructions are for project administrators who have "push"
access on the GitHub repo.

Making a patch release involves merging changes from the main branch
into the release branch, since all development takes place on the main
branch. The process involves these steps:

1. Cherry-pick commits from main to the RB- release branch
2. Bump the version number
3. Add release notes to CHANGES.md
4. Create a "release candidate" tag and announce it publicly
5. Draft the GitHub release
6. Create a signed release tag
7. Publish the release
8. Add an entry to the website news page

Helper scripts in ``share/util/release/`` automate many of the steps
in the process.

A patch release *must* be ABI-compatible with preceding minor releases
and should be validated with an ABI-checker tool such as
[``abipkgdiff``](https://manpages.ubuntu.com/manpages/lunar/en/man1/abipkgdiff.1.html).

### Labeling PRs for Release

Commits for the patch release are identified via labels on PRs. Create
a label for the release, beginning with a `v`, as in
``v3.4.11``. Assign this label to all PRs to be included in the
release.

If a PR should be merged into more than one release branch, i.e. a fix
goes into v3.4 and v3.3, simply add multiple labels, one per release.

It's good practice to label _every_ PR to identify what
release it goes into, even if it's the next minor/major release. That
helps make sure PRs don't get overlooked.

The `share/util/release/log.py` script prints all PRs in the order
they were merged, annotated by their release labels.

### Security Issues

When a PR addresses a security vulnerability, mention it in the
description or comments:

    Addresses CVE-2026-39886

The release scripts will detect this and add an appropriate entry to the
release notes.

When a PR addresses an OSS-Fuzz issue, mention it by url in the
description or comments:

    Addresses https://issues.oss-fuzz.com/issues/456158449

The release scripts will detect this and add an appropriate entry to the
release notes.

### GPG

The signed git tags associated with the release require a [GPG
key](https://docs.github.com/en/authentication/managing-commit-signature-verification/generating-a-new-gpg-key)
that is
[registered](https://docs.github.com/en/authentication/managing-commit-signature-verification/telling-git-about-your-signing-key)
with your GitHub account and git config.

## Release Workflow

Once all PRs have appropriate release labels, follow these steps, all
while on the appropriate `RB-` branch:

1. Cherry-pick commits from the main branch to the release branch.

   This prints the SHAs for the commits associated with the
   labeled PRs. The output will look something like:
      
        % python share/util/release/cherry.py v3.4.8
        git cherry-pick 2e32c6b 3df0122 6155271
        changes.py v3.4.8 2311 2294 2315
          
   Copy/paste the `git cherry-pick` command to the shell.

   If there are conflicts, resolve them and continue.

   If the conflicts are too messy to resolve, consider
   cherry-picking the commits one at a time. The `cherry.py`
   script accepts a `--single` option that prints the commits one
   `git cherry-pick` command per line:

        % python share/util/release/cherry.py --single v3.4.8
        git cherry-pick 2e32c6b # Bump actions/cache from 5.0.3 to 5.0.4 (#2311)
        git cherry-pick 3df0122 # Pin pypa/cibuildwheel actions to release sha (#2294)
        git cherry-pick 6155271 # Force macos cibuildwheel to use Xcode clang (#2315)
        changes.py v3.4.8 2311 2294 2315

2. Bump the OpenEXR version.

   Edit the number in `src/lib/OpenEXRCore/openexr_version.h`. Commit
   and push the change.

3. Create an entry in the release notes file `CHANGES.md`.

   The `changes.py` script takes a list of PRs
   and adds entries in the `CHANGES.md` file:
        
        % python share/util/release/changes.py v3.4.8 2311 2294 2315

   This adds the `Merged Pull Requests` and the
   `Merged Workflow Requests` sections. It also adds a `Security` section
   that lists the CVEs and OSS-Fuzz issues addressed. 
   
   Edit the section by hand to add a summary.

4. Create a `-rc` "release candidate" tag.

   Obviously, build locally and confirm nothing broke in the
   cherry-picking commits and resolving conflicts.
   
   Push the release branch and confirm the CI succeeds before creating
   the release candidate tag.

   Create the tag with a `-rc` suffix, e.g. `v3.4.12-rc`.
   
   The `tag.py` script create a signed tag using the release notes as
   the tag message:

        % python share/util/release/tag.py v3.4.12-rc

   Adding the release notes as the tag message is good practice.

   Push the tags. This triggers the `python-wheels-publish-test`
   CI workflow. Confirm it succeeds.

   Run the `candidate.py` script to format a
   message about the release candidate, formatted in HTML. Pipe the
   output to a `.html` file:
   
        % python share/util/release/candidate.py v3.4.12-rc > v3.4.12.html

   Load the file in a browers to render the formatting, then
   copy/paste the contents into an email to `openexr-dev@lists.aswf.io`
   and the `#openexr` Slack channel.

   If any problems arise after tagging the candidate, fix them in
   subsequent commits and create an
   additional tag with a number appended:
   
        % python share/util/release/tag.py v3.4.12-rc2

5. Draft the GitHub release

        % python share/util/release/draft.py v3.4.12
       
   Verify the notes look correct on the
   [Releases](https://github.com/AcademySoftwareFoundation/openexr/releases)
   page and edit as appropriate. Save as a "draft". DO NOT PUBLISH THE
   RELEASE YET.

6. Create a signed release tag

   Assuming you have your PGP key set up, create the release tag at
   the same commit as the release candidate:
   
        % python share/util/release/tag.py v3.4.12

7. Publish the release

   Set the GitHub release to correspond to the release tag (the `Tag:
   Select tag` option), then click the "Publish release" button on the
   GitHub release draft.

   Monitor the GitHub actions on the
   `https://github.com/AcademySoftwareFoundation/openexr` repo to
   ensure everything builds properly. 
   
   Update the `release` branch, which should always point to the
   most recent patch of the most recent minor release, i.e. the most
   preferred release.

   From a clone of the main repo:

        % git checkout release
        % git merge RB-3.1
        % git push

   Publishing the release triggers a post on the `#openexr` Slack
   channel, but you must manually send an email to
   `openexr-dev@lists.aswf.io` officially announcing the release.

8. Add an entry to the website news page.

        % python share/util/release/news.py v3.4.12

   This pulls the release notes from `CHANGES.md` and inserts a news
   item in `website/news.rst` and `website/latest_news_title.rst`.

   Commit this change, and then cherry-pick the commit from the
   release branch containing the notes and submit the pair as a
   PR. This pulls the release notes for the patch into the main branch.

## Creating a Major/Minor Release

A major/minor release is created from the main branch, assuming there
are no changes on ``main`` that should *not* go into the release. We
don't generally allow experimental changes onto ``main``. Anything
accepted onto ``main`` should be intended for the next minor/major
(not _patch_)release.

The overall workflow is similar to a patch release, as described
above, but it's simpler because there is no cherry-picking of
commits. The major/minor release is simply a snapshot of ``main``.

To create a new release from the ``main`` branch, start by creating
the new "release branch", e.g. `RB-3.5` from the tip of `main`. Then
follow the same instructions above as for a patch release.

Assuming all PRs that have _not_ been merged into a patch release are
labeled with the new minor release, e.g. `v3.5`, the helper scripts in
`share/util/release` will automate the various steps in generating the
release notes, drafting the release, and helping with the various
announcements. 

In addition to the above:
    
1. Increment the ``OPENEXR_LIB_SOVERSION`` setting in [CMakeLists.txt](CMakeLists.txt).

   The SO version increases whenever, and only when, the ABI changes
   in non-backwards-compatible ways. Consistent with the semantic
   versioning policy, this usually happens at major and minor
   releases, but never on a patch release.

   Commit this change to the `RB-` branch.


2. Confirm that the ``OPENEXR_VERSION_MAJOR``,
   ``OPENEXR_VERSION_MINOR``, and ``OPENEXR_VERSION_PATCH`` value in
   [src/lib/OpenEXRCore/openexr_version.h](src/lib/OpenEXRCore/openexr_version.h)
   are correct on the main branch. The OpenEXR project policy is that
   the values on the main branch, which is the bleeding edge of
   development, correspond to the next minor release, with the patch
   set to 0.

3. Update the ``IMATH_TAG`` setting in
   [cmake/OpenEXRSetup.cmake](cmake/OpenEXRSetup.cmake) to correspond
   to the proper Imath release.

