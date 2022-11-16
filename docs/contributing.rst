..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _contributing:
.. _Contributing to OpenEXR:

Contributing to OpenEXR
#######################

Thank you for your interest in contributing to OpenEXR. This document
explains our contribution process and procedures:

For a description of the roles and responsibilities of the various
members of the OpenEXR community, see the :ref:`governance guidelines
<governance>` and the project's :ref:`technical charter <charter>` for
more details. Briefly, contributors are anyone who submits content to
the project, committers review and approve such submissions, and the
Technical Steering Committee provides general project oversight.

Legal Requirements
==================

OpenEXR is a project of the Academy Software Foundation and follows the
open source software best practice policies of the Linux Foundation.

License
-------

OpenEXR is licensed under the :ref:`BSD-3-Clause <license>`
license. Contributions to the library should abide by that standard
license.

Contributor License Agreements
------------------------------

Developers who wish to contribute code to be considered for inclusion
in the OpenEXR distribution must first complete a **Contributor
License Agreement**.

To contribute to OpenEXR, you must sign a CLA through the `EasyCLA
<https://contributor.easycla.lfx.linuxfoundation.org/#/cla/project/2e8710cb-e379-4116-a9ba-964f83618cc5/user/564e571e-12d7-4857-abd4-898939accdd7?redirect=https:%2F%2Fgithub.com%2FAcademySoftwareFoundation%2Fopenexr%2Fpull%2F1154>`_
system, which is integrated with GitHub as a pull request check.

Sign the form through `this link
<https://contributor.easycla.lfx.linuxfoundation.org/#/cla/project/2e8710cb-e379-4116-a9ba-964f83618cc5/user/564e571e-12d7-4857-abd4-898939accdd7?redirect=https:%2F%2Fgithub.com%2FAcademySoftwareFoundation%2Fopenexr%2Fpull%2F1154>`_
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
projects and `recommended by the ASWF TAC
<https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contributor-license-agreement-cla>`_

DCO Commit Sign-Off
-------------------

Every commit must be signed off.  That is, every commit log message
must include a ``Signed-off-by`` line (generated, for example, with
``git commit --signoff``, indicating that the committer wrote the code
and has the right to release it under the [BSD-3-Clause](LICENSE.md)
license. See TAC `sign-off guidelines
<https://github.com/AcademySoftwareFoundation/tac/blob/main/process/contributing.md#contribution-sign-off>`_
for more information on this requirement.

Development Workflow
====================

Git Basics
----------

Working with OpenEXR requires understanding a significant amount of
Git and GitHub based terminology. If you’re unfamiliar with these
tools or their lingo, please look at the `GitHub Glossary
<https://help.github.com/articles/github-glossary/>`_ or browse
`GitHub Help <https://help.github.com/>`_.

To contribute, you need a GitHub account. This is needed in order to
push changes to the upstream repository, via a pull request.

You will also need Git installed on your local development machine. If
you need setup assistance, please see the official `Git Documentation
<https://git-scm.com/doc>`_.

Repository Structure and Commit Policy
--------------------------------------

The OpenEXR repository uses a simple branching and merging strategy.

All development work is done directly on the ``main`` branch. The ``main``
branch represents the bleeding-edge of the project and most
contributions should be done on top of it.

After sufficient work is done on the ``main`` branch and the OpenEXR
leadership determines that a release is due, we will bump the relevant
internal versioning and tag a commit with the corresponding version
number, e.g. v2.0.1. Each Minor version also has its own “Release
Branch”, e.g. RB-1.1. This marks a branch of code dedicated to that
Major.Minor version, which allows upstream bug fixes to be
cherry-picked to a given version while still allowing the ``main``
branch to continue forward onto higher versions. This basic repository
structure keeps maintenance low, while remaining simple to understand.

To reiterate, the ``main`` branch represents the latest development
version, so beware that it may include untested features and is not
generally stable enough for release.  To retrieve a stable version of
the source code, use one of the release branches.

The Git Workflow
----------------

This development workflow is sometimes referred to as `OneFlow
<https://www.endoflineblog.com/oneflow-a-git-branching-model-and-workflow>`_. It
leads to a simple, clean, linear edit history in the repo.

The OpenEXR GitHub repo allows rebase merging and disallows merge
commits and squash merging. This ensures that the repo edit history
remains linear, avoiding the "bubbles" characteristic of the `GitFlow
<https://www.endoflineblog.com/gitflow-considered-harmful>`_ workflow.

Use the Fork, Luke
------------------

In a typical workflow, you should **fork** the OpenEXR repository to
your account. This creates a copy of the repository under your user
namespace and serves as the “home base” for your development branches,
from which you will submit **pull requests** to the upstream
repository to be merged.

Once your Git environment is operational, the next step is to locally
**clone** your forked OpenEXR repository, and add a **remote**
pointing to the upstream OpenEXR repository. These topics are covered
in the GitHub documentation `Cloning a repository
<https://help.github.com/articles/cloning-a-repository/>`_ and
`Configuring a remote for a fork
<https://help.github.com/articles/configuring-a-remote-for-a-fork/>`_,
but again, if you need assistance feel free to reach out on the
openexr-dev@lists.aswf.io mail list.

Pull Requests
-------------

Contributions should be submitted as Github pull requests. See
`Creating a pull request
<https://help.github.com/articles/creating-a-pull-request/>`_ if
you're unfamiliar with this concept.

The development cycle for a code change should follow this protocol:

1. Create a topic branch in your local repository, following the naming format
   ``<your-feature>`` or ``<your-fix>``.

2. Make changes, compile, and test thoroughly. Code style should match
   existing style and conventions, and changes should be focused on
   the topic the pull request will be addressing. Make unrelated
   changes in a separate topic branch with a separate pull request.

3. Push commits to your fork.

4. Create a Github pull request from your topic branch.

5. Pull requests will be reviewed by project committers and
   contributors, who may discuss, offer constructive feedback, request
   changes, or approve the work.

6. Upon receiving the required number of committer approvals (as
   outlined in :ref:`Code Review and Required Approvals <approval>`, a committer
   other than the PR contributor may merge changes into the ``main``
   branch.

.. _approval:

Code Review and Required Approvals
----------------------------------

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
  discussed at length in the mail list or TSC meetings before any PR
  is submitted, in order to solicit feedback, try to get as much
  consensus as possible, and alert all the stakeholders to be on the
  lookout for the eventual PR when it appears.

* Trivial changes that don't affect functionality (typos, docs, tests)
  can be approved by the committer without review, after waiting at
  least 48 hours.

* Big changes that can alter behavior, add major features, or present
  a high degree of risk should be signed off by TWO committers,
  ideally one of whom should be the "owner" for that section of the
  codebase (if a specific owner has been designated). If the person
  submitting the PR is him/herself the "owner" of that section of the
  codebase, then only one additional committer approval is
  sufficient. But in either case, a 48 hour minimum is helpful to give
  everybody a chance to see it, unless it's a critical emergency fix
  (which would probably put it in the previous "small fix" category,
  rather than a "big feature").

* Escape valve: big changes can nonetheless be merged by a single
  committer if the PR has been open for over two weeks without any
  unaddressed objections from other committers. At some point, we have
  to assume that the people who know and care are monitoring the PRs
  and that an extended period without objections is really assent.

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
modifications to the TSC by assigning the ``TSC`` label to a pull
request or issue. The TSC should serve as the final arbiter where
required.

Test Policy
-----------

All functionality in the library must be covered by an automated
test. Each library has a companion ``Test`` project, e.g. ``OpenEXRTest``,
``OpenEXRCoreTest``, ``OpenEXRUtilTest``, etc.  This test suite is collectively
expected to validate the behavior of very part of the library.

* All new functionality should be accompanied by a test that validates
  its behavior.

* Any change to existing functionality should have tests added if they
  don't already exist.

The test should should be run, via ``make check``, before submitting a
pull request.

In addition, the ``OpenEXRFuzzTest`` project validates the library by
feeding it corrupted input data. This test is time-consuming (possible
over 24 hours), so it will only be run occasionally, but it must
succeed before a release is made.

Coding Style
============

The coding style of the library source code is enforced via Clang
format, with the configuration defined in ``.clang-format``.

Formatting
----------

When modifying existing code, follow the surrounding formatting
conventions so that new or modified code blends in with the current
code.

* Indent with spaces, never tabs. Each indent level should be 4 spaces.

* Function return types go on a separate line:

.. code-block::

        const float &	
        Header::pixelAspectRatio () const
        {
            ...
        }

* Use a space between function names and the following parentheses
  (although you can eliminate the space for functions with no
  arguments):

.. code-block::

        void
        Header::insert (const string& name, const Attribute& attribute)
        {
            insert (name.c_str(), attribute);
        }

* Place curly braces on their own lines:

.. code-block::

        void
        RgbaOutputFile::ToYca::padTmpBuf ()
        {
            for (int i = 0; i < N2; ++i)
            {
                _tmpBuf[i] = _tmpBuf[N2];
                _tmpBuf[_width + N2 + i] = _tmpBuf[_width + N2 - 2];
            }
        }

Naming Conventions
------------------

* In general, classes and template type names should start with upper
  case and capitalize new words: ``class CustomerList;``

* In general, local variables should use camelCase. Macros and
  constants should use ``ALL_CAPS``.

* Member fields in a class should start with an underscore. No other
  variables should begin with underscore.

File Conventions
----------------

C++ implementation should be named ``*.cpp``. Headers should be named ``.h``.

All headers should contain:

.. code-block::

    #pragma once

Type Conventions
----------------

Because OpenEXR must deal properly with large images, whose width
and/or height approach the maximum allowable in 32-bit signed
integers, take special care that integer arithmetic doesn't overflow,
and make it as clear as possible exactly what the code is doing,
especially in the edge cases.

To clarify the intention, prefer to cast between types using
``static_cast<>()`` rather than the basic C-style ``()`` notation:

.. code-block::

    // good:
    size_t x = static_cast <size_t> (y);

    // bad:
    x = (size_t) y;
    x = size_t (y);

Prefer to use ``std::numeric_limits<>`` instead of preprocessor
define's such as ``INT_MAX``:

.. code-block::

    // good:
    if (x > std::numeric_limits<int>::max())
        std::cout << "That's too freakin' high.\n";

    // bad:
    if (x > INT_MAX)

Copyright Notices
-----------------

All new source files should begin with a copyright and license stating:

.. code-block::

    //
    // SPDX-License-Identifier: BSD-3-Clause
    // Copyright (c) Contributors to the OpenEXR Project. 
    //
    
Third-party libraries
---------------------

Prefer C++11 ``std`` over boost where possible.  Use boost classes you
already see in the code base, but check with the project leadership
before adding new boost usage.

Comments and Doxygen
--------------------

Comment philosophy: try to be clear, try to help teach the reader
what's going on in your code.

Prefer C++ comments (starting line with ``//``) rather than C comments
(``/* ... */``).

For public APIs, use Doxygen-style comments (start with ``///``), such as:

.. code-block::

    /// Explanation of a class.  Note THREE slashes!
    /// Also, you need at least two lines like this.  If you don't have enough
    /// for two lines, make one line blank like this:
    ///
    class myclass {
        ....
        float foo;  ///< Doxygen comments on same line look like this
    }

Versioning Policy
=================

OpenEXR uses `semantic versioning <https://semver.org>`_, which labels
each version with three numbers: Major.Minor.Patch, where:

* **MAJOR** indicates incompatible API changes
* **MINOR** indicates functionality added in a backwards-compatible manner
* **PATCH** indicates backwards-compatible bug fixes 

ASWF Docker
===========

OpenEXR's CI infrastructure utilizes ASWF-managed docker containers for 
Linux builds. Each image implements a specific `VFX Reference Platform 
<https://vfxplatform.com/>`__ calendar year and all dependencies for a specific 
ASWF project.

For example, the `aswf/ci-openexr:2020 
<https://hub.docker.com/layers/aswf/ci-openexr/2022.2/images/sha256-92f2b69814b97dc4ba1ac205546e46ad23bcd9cb3075950b7e7f3f650112b072?context=explore>`__
container provides a build environment with all upstream OpenEXR 
dependencies and adheres to VFX Reference Platform CY2020. VFX Reference 
Platform calendar years starting with 2018 up to the current draft year 
specification are supported.

These Docker images are available in the `aswf DockerHub repository 
<https://hub.docker.com/u/aswf>`__ for public use. See the table in the 
`aswf-docker source GitHub repository 
<https://github.com/AcademySoftwareFoundation/aswf-docker>`__ for a summary of
all available images.

Creating a Release
==================

To create a new release from the ``main`` branch:

1. Update the release notes in ``CHANGES.md``.

   Write a high-level summary of the features and
   improvements. Include the summary in ``CHANGES.md`` and also in the
   Release comments.

   Include the log of all PR's included beyond the previous release. 

2. Create a new release on the GitHub Releases page.

3. Tag the release with name beginning with ``v``', e.g. ``v2.3.0``.
    
