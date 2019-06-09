# Contributing to OpenEXR

Thank you for your interest in contributing to OpenEXR. This document
explains our contribution process and procedures, so please review it first.

## Getting Connected

There are two primary ways to connect with the OpenEXR project:

* The [openexr-dev](https://lists.aswf.io/g/openexr-dev) mail list:
This is a development focused mail list with a deep history of technical
conversations and decisions that have shaped the project.

* [GitHub Issues](https://github.com/openexr/openexr/issues): GitHub
**issues** are a great place to start a conversation! Issues aren’t
just for bugs, they are also for feature requests and other
suggestions, for the code or documentation or the website.  The only
conversations not appropriate for issues are questions in the form of
“How do I do X”. Please direct these to the openexr-dev mail lists,
and consider contributing what you've learned to our docs if
appropriate.

## Bug Reports and Issue Tracking

OpenEXR use GitHub's issue tracking system for bugs and enhancements:
https://github.com/openexr/openexr/issues

**If you are merely asking a question ("how do I...")**, please do not file
an issue, but instead ask the question on the
[openexr-dev](http://lists.aswf.io/openexr-dev) mailing list.

If you are submitting a bug report, please be sure to note which
version of OpenEXR you are using, on what platform (OS/version, which
compiler you used, and any special build flags or other unusual
environmental issues). Please give a specific an account of

* what you tried
* what happened
* what you expected to happen instead

with enough detail that others can reproduce the problem.

## Reporting Vulnerabilities

If you think you've found a potential vulnerability in OpenEXR, please
report it by emailing security@openexr.com. Only TSC members and ASWF
project management have access to these messages. Include detailed
steps to reproduce the issue, and any other information that could aid
an investigation.

## Git Basics

Working with OpenEXR requires understanding a significant amount of
Git and GitHub based terminology. If you’re unfamiliar with these
tools or their lingo, please look at the [GitHub
Glossary](https://help.github.com/articles/github-glossary/) or browse
[GitHub Help](https://help.github.com/).

To contribute, you need a GitHub account. This is needed in order to
push changes to the upstream repository. After setting up your account
you should then **fork** the OpenEXR repository to your account. This
creates a copy of the repository under your user namespace and serves
as the “home base” for your development branches, from which you will
submit **pull requests** to the upstream repository to be merged.

You will also need Git installed on your local development machine. If
you need setup assistance, please see the official [Git
Documentation](https://git-scm.com/doc).

Once your Git environment is operational, the next step is to locally
**clone** your forked OpenColorIO repository, and add a **remote**
pointing to the upstream OpenColorIO repository. These topics are
covered in [Cloning a repository]
(https://help.github.com/articles/cloning-a-repository/) and
[Configuring a remote for a fork]
(https://help.github.com/articles/configuring-a-remote-for-a-fork/),
but again, if you need assistance feel free to reach out on the
openexr-dev mail list.

## Contributor Licence Agreements

Developers who wish to contribute code to be considered for inclusion
in the OpenEXR distribution must first complete a **Contributor
License Agreement**.

Trivial changes (such as an alteration to the Makefiles, a one-line
bug fix, etc.) may be accepted without a CLA, at the sole discretion of the
project leader, but anything complex requires a CLA. 

* If you are an individual writing the code on your own time and
you're SURE you are the sole owner of any intellectual property you
contribute, use the [Individual Contributor License Agreement]
(http://www.openexr.com/downloads/OpenEXRIndividualContributorLicenseAgreement.docx).

* If you are writing the code as part of your job, or if there is any
possibility that your employers might think they own any intellectual
property you create, then you should use the [Corporate Contributor
Licence Agreement]
(http://www.openexr.com/downloads/OpenEXRCorporateContributorLicenseAgreement.docx).

Download the appropriate CLA from the links above (or find them in the
src/doc directory of the software distribution), print, sign, and
rescan it (or just add a digital signature directly), and email it
back to us (info@openexr.com).

The OpenEXR CLA's are the standard forms used by Linux Foundation
projects.

### Commit Sign-Off

Every commit must be signed off.  That is, every commit log message
must include a “`Signed-off-by`” line (generated, for example, with
“`git commit --signoff`”), indicating that the committer wrote the
code and has the right to release it under the [MPL
2.0](https://www.mozilla.org/MPL/2.0/) license. See
http://developercertificate.org/ for more information on this
requirement.

## Repository Structure and Commit Policy

The OpenEXR repository uses a simple branching and merging strategy.

All development work is done directly on the master branch. The master
branch represents the bleeding-edge of the project and any
contributions should be done on top of it.

After sufficient work is done on the master branch and the OpenEXR
leadership determines that a release is due, we will bump the relevant
internal versioning and tag a commit with the corresponding version
number, e.g. v2.0.1. Each Minor version also has its own “Release
Branch”, e.g. RB-1.1. This marks a branch of code dedicated to that
Major.Minor version, which allows upstream bug fixes to be
cherry-picked to a given version while still allowing the master
branch to continue forward onto higher versions. This basic repository
structure keeps maintenance low, while remaining simple to understand.

To reiterate, the master branch represents the latest development
version, so beware that it may include untested features and is not
generally stable enough for release.  To retrieve a stable version of
the source code, use one of the release branches.

This development workflow is sometimes referred to as
[OneFlow](https://www.endoflineblog.com/oneflow-a-git-branching-model-and-workflow). It
leads to a simple, clean, linear edit history in the repo.

## Development and Pull Requests

Contributions should be submitted as Github pull requests. See
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

4. Create a Github pull request from your topic branch.

5. All pull requests trigger CI builds on [Travis CI](https://travis-ci.org/)
for Linux and Mac OS and [AppVeyor](https://www.appveyor.com/) for Windows.
These builds verify that code compiles and all unit tests succeed. CI build
status is displayed on the GitHub PR page, and changes will only be considered
for merging after all builds have succeeded.

6. Pull requests will be reviewed by project Committers and Contributors,
who may discuss, offer constructive feedback, request changes, or approve
the work.

7. Upon receiving the required number of Committer approvals (as outlined
in [Required Approvals](#required-approvals)), a Committer other than the PR
contributor may squash and merge changes into the master branch.

See also (from the OpenEXR Developer Guide):
* [Getting started](http://opencolorio.org/developers/getting_started.html)
* [Submitting Changes](http://opencolorio.org/developers/submitting_changes.html)

## Code Review and Required Approvals

Modifications of the contents of the OpenEXR repository are made on a
collaborative basis. Anyone with a GitHub account may propose a
modification via pull request and it will be considered by the project
Committers.

Pull requests must meet a minimum number of Committer approvals prior
to being merged. Rather than having a hard rule for all PRs, the
requirement is based on the complexity and risk of the proposed
changes, factoring in the length of time the PR has been open to
discussion. The following guidelines outline the project's established
approval rules for merging:

* Core design decisions, large new features, or anything that might be
perceived as changing the overall direction of the project should be
discussed at length in the mail list before any PR is submitted, in
order to: solicit feedback, try to get as much consensus as possible,
and alert all the stakeholders to be on the lookout for the eventual
PR when it appears.

* Small changes (bug fixes, docs, tests, cleanups) can be approved and
merged by a single Committer.

* Big changes that can alter behavior, add major features, or present
a high degree of risk should be signed off by TWO Committers, ideally
one of whom should be the "owner" for that section of the codebase (if
a specific owner has been designated). If the person submitting the PR
is him/herself the "owner" of that section of the codebase, then only
one additional Committer approval is sufficient. But in either case, a
48 hour minimum is helpful to give everybody a chance to see it,
unless it's a critical emergency fix (which would probably put it in
the previous "small fix" category, rather than a "big feature").

* Escape valve: big changes can nonetheless be merged by a single
Committer if the PR has been open for over two weeks without any
unaddressed objections from other Committers. At some point, we have
to assume that the people who know and care are monitoring the PRs and
that an extended period without objections is really assent.

Approval must be from Committers who are not authors of the change. If
one or more Committers oppose a proposed change, then the change
cannot be accepted unless:

* Discussions and/or additional changes result in no Committers
objecting to the change. Previously-objecting Committers do not
necessarily have to sign-off on the change, but they should not be
opposed to it.

* The change is escalated to the TSC and the TSC votes to approve the
change.  This should only happen if disagreements between Committers
cannot be resolved through discussion.

Committers may opt to elevate significant or controversial
modifications to the TSC by assigning the `tsc-review` label to a pull
request or issue. The TSC should serve as the final arbiter where
required.

## Coding Style

#### Formatting

We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html)
to uniformly format our source code prior to PR submission. Make sure that
clang-format is installed on your local machine, and just run

    make clang-format

and it will automatically reformat your code according to the configuration
file found in the `.clang-format` file at the root directory of the OpenEXR
source code checkout.

One of the TravisCI test matrix entries runs clang-format and fails if any
diffs were generated (that is, if any of your code did not 100% conform to
the `.clang-format` formatting configuration). If it fails, clicking on that
test log will show you the diffs generated, so that you can easily correct
it on your end and update the PR with the formatting fixes.

If you don't have clang-format set up on your machine, and your patch is not
very long, you may find that it's more convenient to just submit it and hope
for the best, and if it doesn't pass the Travis test, look at the diffs in
the log and make the corrections by hand and then submit an update to the
patch (i.e. relying on Travis to run clang-format for you).

Because the basic formatting is automated by clang-format, we won't
enumerate the rules here.

#### File conventions

C++ implementation should be named `*.cpp`. Headers should be named `.h`.
All headers should contain

    #pragma once

All source files should begin with the copyright and license, which
can be found in the [LICENSE](LICENSE.md) file (or cut and pasted from
any other other source file).

For NEW source files, please change the copyright year to the present. DO
NOT edit existing files only to update the copyright year, it just creates
pointless deltas and offers no increased protection.


#### Identifiers

In general, classes and templates should start with upper case and
capitalize new words: `class CustomerList;` In general, local variables
should start with lower case. Macros should be `ALL_CAPS`, if used at all.

If your class is extremely similar to, or modeled after, something in the
standard library, Boost, or something else we interoperate with, it's ok to
use their naming conventions. For example, very general utility classes and
templates (the kind of thing you would normally find in std or boost) should
be lower case with underscores separating words, as they would be if they
were standards.

    template <class T> shared_ptr;
    class scoped_mutex;

Names of data should generally be nouns. Functions/methods are trickier: a
the name of a function that returns a value but has no side effects should
be a noun, but a procedure that performs an action should be a verb.

#### Class structure

Try to avoid public data members, although there are some classes that serve
a role similar to a simple C struct -- a very straightforward collection of
data members. In these, it's fine to make the data members public and have
clients set and get them directly.

Private member data should be named m_foo (alternately, it's ok to use the
common practice of member data foo_, but don't mix the conventions within a
class).

Private member data that needs public accessors should use the convention:

    void foo (const T& newfoo) { m_foo = newfoo; }
    const T& foo () const { return m_foo; }

Avoid multiple inheritance.

Namespaces: yes, use them!

#### Third-party libraries

Prefer C++11 `std` rather than Boost, where both can do the same task.
Feel free to use Boost classes you already see in the code base, but don't
use any Boost you don't see us already using, without first checking with
the project leader.

#### Comments and Doxygen

Comment philosophy: try to be clear, try to help teach the reader what's
going on in your code.

Prefer C++ comments (starting line with `//`) rather than C comments (`/* ... */`).

For public APIs we tend to use Doxygen-style comments (start with `///`).
They looks like this:

    /// Explanation of a class.  Note THREE slashes!
    /// Also, you need at least two lines like this.  If you don't have enough
    /// for two lines, make one line blank like this:
    ///
    class myclass {
        ....
        float foo;  ///< Doxygen comments on same line look like this
    }

#### Miscellaneous

Macros should be used only rarely -- prefer inline functions, templates,
enums, or "const" values wherever possible.

Prefer `std::unique_ptr` over raw new/delete.

#### Bottom Line

When in doubt, look elsewhere in the code base for examples of similar
structures and try to format your code in the same manner, or ask on
the openexr-dev mail list.

For standards on contributing to documentation, see the [Documentation
Guidelines](http://opencolorio.org/developers/documentation_guidelines.html).

## Versioning Policy

OpenEXR uses [semantic versioning](https://semver.org), which labels
each version with three numbers: Major.Minor.Patch, where:

* **MAJOR** indicates incompatible API changes,
* **MINOR** indicates functionality added in a backwards-compatible manner
* **PATCH** indicates backwards-compatible bug fixes 

