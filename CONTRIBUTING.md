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
[openexr-dev](http://lists.aswf.io/openexr-dev) mailing list. This is
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

* Legal authorization, that is, you need to have signed a contributor
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
[EasyCLA](https://contributor.easycla.lfx.linuxfoundation.org/#/cla/project/2e8710cb-e379-4116-a9ba-964f83618cc5/user/564e571e-12d7-4857-abd4-898939accdd7?redirect=https:%2F%2Fgithub.com%2FAcademySoftwareFoundation%2Fopenexr%2Fpull%2F1154)
system, which is integrated with GitHub as a pull request check.

Sign the form through [this
link](https://contributor.easycla.lfx.linuxfoundation.org/#/cla/project/2e8710cb-e379-4116-a9ba-964f83618cc5/user/564e571e-12d7-4857-abd4-898939accdd7?redirect=https:%2F%2Fgithub.com%2FAcademySoftwareFoundation%2Fopenexr%2Fpull%2F1154)
prior to submitting a pull request. If you submit a pull request
before the form is signed, the "linux-foundation-easycla" check will
fail and a red "NOT COVERED" button will appear in the PR
comments. Click that link to sign the form.

* If you are an individual writing the code on your own time and
  you're **sure** you are the sole owner of any intellectual property you
  contribute, you can sign the CLA as an **Individual Contributor**.

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

The OpenEXR CLAs are the standard forms used by Linux Foundation
projects and [recommended by the ASWF
TAC](https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contributor-license-agreement-cla).

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

Making a patch release typically involves merging changes from the
main branch into the release branch, since all development generally
takes place on the main branch. The usual workflow is to group these
changes together and merge them all just prior to a release, rather
than merging one-by-one as the changes go into main, although merging
along the way is acceptable as well. For OpenEXR and Imath, a patch
release typically involves under a dozen commits, so it's not a huge
amount of work to organize them all at once.

A patch release *must* be ABI-compatible with preceding minor releases
and should be validated with an ABI-checker tool such as
[``abipkgdiff``](https://manpages.ubuntu.com/manpages/lunar/en/man1/abipkgdiff.1.html).

These instructions are for project administrators who have "push"
access on the GitHub repo.

The preferred workflow is:

1. Make a PR that merges appropriate changes from the main branch to
   the release branch:

   a. In a forked clone, create a branch to hold the patch commits,
      e.g. ``v3.1.9-fixes``.

   b. Cherry-pick the appropriate commits from ``main``, resolving any
      merge conflicts.

   c. Increment ``OPENEXR_VERSION_PATCH`` in
      [src/lib/OpenEXRCore/openexr_version.h](src/lib/OpenEXRCore/openexr_version.h)

   d. Update the ``IMATH_TAG`` setting in
      [cmake/OpenEXRSetup.cmake](cmake/OpenEXRSetup.cmake) to
      correspond to the proper Imath release.

   e. Add release notes to [CHANGES.md](CHANGES.md):

      - Generate a list of links to merged pull requests.

        Use ``git log`` to identify the merged commits, and for each
        commit, and add a link in the notes to the corresponding PR
        that merged it to ``main``. Citing PR's in the release notes
        is preferable to citing raw commits because the PR's often
        have helpful information and discussion missing from the
        commit descriptions, and the commit history is readily
        accessible via ``git log`` anyway.

        The typical OpenEXR project workflow uses "squash and merge"
        to merge PR's into ``main``, so the changes involved in each
        PR end up on ``main`` as a single commit. This is preferable
        because a raw PR often includes numerous commits that address
        comments and feedback or fix typos or mistakes, intermediate
        steps not helpful to the preserved history of the main
        development thread. Note that GitHub's "squash and merge"
        helpfully appends the PR number to the commit subject line.

        Note that when this PR is merged to the release branch, it
        should go in via "rebase and merge" that the release branch
        retains the granular changes, described below.

      - Generate a list of OSS-Fuzz issues addressed.

        These are security concerns, so they deserve special
        attention. Provide a link in the notes to the issue at
        https://bugs.chromium.org/p/oss-fuzz, including the issue id
        number and description.

      - If there are any public CVE's, mention them explicitly with a
        link to the CVE registry item.

      - Provide an executive summary of the patch changes, in a few
        sentences as well as bullet points if appropriate.

      - Choose a proposed release date at least several days in
        advance.

   f. If there are any public CVE's, reference them in
      [SECURITY.md](SECURITY.md).

   g. Submit the PR for others to review. The PR should go *to the
      release branch, not ``main``*, obviously.

   h. After others have had a chance to sanity-check the changes,
      merge the PR *with "rebase and merge"*.  Unlike with the usual
      PR's merged to main, it is essential to retain the individual
      commits on the release branch. That way, the release branch
      commit history retains the details of the changes.

   i. If further fixes come in that need to go into the release, push
      them to the PR branch. It's not absolutely essential that all
      changes to the release branch go in via a PR. The PR is simply a
      convenient forum for publicly discussing and reviewing the
      composition of the release.

2. Tag the release with a ``-rc`` "release candidate" tag,
   e.g. ``v3.1.9-rc``.

3. Validate ABI compatibility. Build at the release candidate tag and
   run
   [abipkgdiff](https://manpages.ubuntu.com/manpages/lunar/en/man1/abipkgdiff.1.html)
   against a build of the previous patch release to confirm that no
   ABI changes have leaked in. Additions to the ABI are acceptable for
   a patch release, but there should be no symbol changes and no
   symbols removed. If there are, back up and fix them before proceeding.

4. Send mail to ``openexr-dev@lists.aswf.io`` announcing the staging
   of the release with link to the release candidate tag. Include the
   release notes from [CHANGES.md](CHANGES.md) for review.

5. Draft the release on the GitHub
   [Releases](https://github.com/AcademySoftwareFoundation/openexr/releases)
   page.  Include the summary from the notes in
   [CHANGES.md](CHANGES.md), but don't include the list of PR's.

   Create the release from the latest ``--rc`` tag, and give it a name
   that begins with ``v``, i.e. ``v3.1.9``.

   Save the release as a "draft".

6. Wait at least 48 hours, to give the community time to discover and
   report any obvious problems. Avoid the temptation to rush changes
   into a release and publish it immediately, as that is uncomfortably
   error prone.

   If additional fixes need to go in before release:

   a. Merge commits to the release branch. Push them directly, no need
      for a pull request.

   b. Update the release notes in a separate commit.

   c. Re-tag with a incremented "release candidate" number,
      e.g. ``v3.1.9-rc2``.  

   d. Send an email update to ``openexr-dev@lists.aswf.io`` notifying
      the community of the addition and the new tag.

7. Create a signed release tag

   a. Make sure you have a [GPG
      key](https://docs.github.com/en/authentication/managing-commit-signature-verification/generating-a-new-gpg-key)
      and it is
      [registered](https://docs.github.com/en/authentication/managing-commit-signature-verification/telling-git-about-your-signing-key)
      with your GitHub account and git config.

   b. Create a signed tag with the release name via `git tag -s v3.1.9`.

   c. Push the tag via `git push --tags`

8. Publish the release

   a. Click the "Publish release" button on the GitHub release draft

   b. Send an email to ``openexr-dev@lists.aswf.io`` officially
      announcing the release.

9. Update the ``release`` branch, which should always point to the
   most recent patch of the most recent minor release, i.e. the most
   preferred release.

   From a clone of the main repo:

       % git checkout release
       % git merge RB-3.1
       % git push
         
10. Submit a PR that adds the release notes to [CHANGES.md](CHANGES.md)
    on the main branch. Cherry-pick the release notes commit from
    the release branch.

    - If any changes have gone into [SECURITY.md](SECURITY), cherry-pick
      the associated commit as well.

    - Also include in this PR edits to [``website/news.rst``](website/news.rst)
      that add an announcement of the release.

11. After review/merge of the updates to ``website/news.rst``, build the
    website at https://readthedocs.org/projects/openexr.

12. If the release has resolved any OSS-Fuzz issues, update the
    associated pages at https://bugs.chromium.org/p/oss-fuzz with a
    reference to the release.

13. If the release has resolved any public CVE's, request an update
    from the registry service providing the release and a link to the
    release notes.

## Creating a Major/Minor Release

A major/minor release is created from the main branch, assuming there
are no changes on ``main`` that should *not* go into the release. We
don't generally allow experimental changes onto ``main``. Anything
accepted onto ``main`` should be intended for the next release.

The overall workflow is similar to a patch release, as described
above, but it's simpler because there is no cherry-picking and merging
of commits. The major/minor release is simply a snapshot of ``main``.

To create a new release from the ``main`` branch:

1. Confirm that the ``OPENEXR_VERSION_MAJOR``,
   ``OPENEXR_VERSION_MINOR``, and ``OPENEXR_VERSION_PATCH`` value in
   [src/lib/OpenEXRCore/openexr_version.h](src/lib/OpenEXRCore/openexr_version.h)
   are correct. The OpenEXR project policy is that the values on the
   main branch, which is the bleeding edge of development, correspond
   to the next minor release, with the patch set to 0.

2. Update the release notes in [CHANGES.md](CHANGES.md):

   - Write a high-level summary of the features and improvements.

   - Include the log of all PR's that have *not* been merged into the
     previous minor release.

   - Mention any OSS-Fuzz issues. Provide a link in the notes to the issue at
     https://bugs.chromium.org/p/oss-fuzz, including the issue id
     number and description.

   - If there are any public CVE's, mention them explicitly with a
     link to the CVE registry item.

   - Submit this change as a separate PR.

3. Add a mention of the release to [``website/news.rst``](website/news.rst)

   - Submit this change as a separate PR.

4. Increment the ``OPENEXR_LIB_SOVERSION`` setting in [CMakeLists.txt](CMakeLists.txt).

   - The SO version increases whenever, and only when, the ABI changes
     in non-backwards-compatible ways. Consistent with the semantic
     versioning policy, this usually happens at major and minor
     releases, but never on a patch release.

   - Submit this change as a separate PR for review.

5. Once the above PR's are merged, create the release branch with the
   ``RB`` prefix, e.g. ``RB-3.2``.

6. Update the ``IMATH_TAG`` setting in
   [cmake/OpenEXRSetup.cmake](cmake/OpenEXRSetup.cmake) to correspond
   to the proper Imath release.

7. Tag the release with a ``-rc`` "release candidate" tag,
   e.g. ``v3.2.0-rc``.

8. Send mail to ``openexr-dev@lists.aswf.io`` announcing the staging
   of the release with link to the release candidate tag. Include the
   release notes from [CHANGES.md](CHANGES.md) for review.

9. If additional fixes need to go in before release:

   a. Merge commits to the release branch. Push them directly, no need
      for a pull request.

   b. Update the release notes in a separate commit.

   c. Re-tag with a incremented "release candidate" number,
      e.g. ``v3.2.0-rc2``.  

   d. Send a email update to ``openexr-dev@lists.aswf.io`` notifying
      the community of the addition.

10. Draft the release on the GitHub
    [Releases](https://github.com/AcademySoftwareFoundation/openexr/releases)
    page.  Include the summary from the notes in
    [CHANGES.md](CHANGES.md), but don't include the list of PR's.

    - Create the release from the latest ``--rc`` tag, and give it a name
      that begins with ``v`` and ends in ``0``, e.g. ``v3.2.0``.

    - Save the release as a "draft".

11. Wait at least 48 hours after the email announcement.

12. Publish the release

    a. Click the "Publish release" button on the GitHub release draft

    b. Send an email to ``openexr-dev@lists.aswf.io`` officially
       announcing the release.

13. Update the ``release`` branch, which should always point to the
    most recent release.

    From a clone of the main repo:

        % git checkout release
        % git merge RB-3.1
        % git push
         
14. Increment ``OPENEXR_VERSION_MINOR`` in
    [src/lib/OpenEXRCore/openexr_version.h](src/lib/OpenEXRCore/openexr_version.h) on the main branch

    - Submit a PR for this. This leaves the release version on the
      main branch pointing to the next minor release, as described in
      Step #1.

15. Build the website at https://readthedocs.org/projects/openexr.

16. If the release has resolved any OSS-Fuzz issues, update the
    associated pages at https://bugs.chromium.org/p/oss-fuzz with a
    reference to the release.

17. If the release has resolved any public CVE's, request an update
    from the registry service providing the release and a link to the
    release notes.
