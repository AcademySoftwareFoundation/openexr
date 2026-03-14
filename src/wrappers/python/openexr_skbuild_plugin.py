# Copyright Contributors to the MaterialX Project
# SPDX-License-Identifier: Apache-2.0
# copied from: https://github.com/AcademySoftwareFoundation/MaterialX/blob/main/python/mtx_skbuild_plugin.py
# Modifications Copyright (c) Contributors to the OpenEXR Project.

"""
This is a custom scikit-build-core plugin that will
fetch the OpenEXR version from the CMake project.
"""
import os
import tempfile
import subprocess
from pathlib import Path
from typing import FrozenSet, Dict, Optional, Union, List

from scikit_build_core.file_api.query import stateless_query
from scikit_build_core.file_api.reply import load_reply_dir


def dynamic_metadata(
    fields: FrozenSet[str],
    settings: Optional[Dict[str, object]] = None,
) -> Dict[str, Union[str, Dict[str, Optional[str]]]]:
    print("openexr_skbuild_plugin: Computing OpenEXR version from CMake...")

    if fields != "version":
        msg = f"Only the 'version' field is supported: fields={fields}"
        raise ValueError(msg)

    if settings:
        msg = "No inline configuration is supported"
        raise ValueError(msg)

    if "OPENEXR_RELEASE_CANDIDATE_TAG" in os.environ:

        # e.g. "v3.1.2-rc4"
        #
        # If OPENEXR_RELEASE_CANDIDATE_TAG is set,
        # the build is for a publish to test.pypi.org. Multiple test
        # publishes may happen in the course of preparing for a
        # release, but published packages require unique
        # names/versions, so use the release candidate tag as the
        # version (minus the leading 'v'),

        rct = os.environ["OPENEXR_RELEASE_CANDIDATE_TAG"]
        version = rct[1:]

    else:
        
        current_dir = os.path.dirname(__file__)

        with tempfile.TemporaryDirectory() as tmpdir:
            # We will use CMake's file API to get the version
            # instead of parsing the CMakeLists files.

            # First generate the query folder so that CMake can generate replies.
            reply_dir = stateless_query(Path(tmpdir))

            # Run cmake (configure). CMake will generate a reply automatically.
            try:
                subprocess.run(
                    [
                        "cmake",
                        "-S",
                        current_dir + "../../../..",
                        "-B",
                        tmpdir,
                        "-DOPENEXR_BUILD_LIBS=OFF",
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    check=True,
                    text=True,
                )
            except subprocess.CalledProcessError as exc:
                print(exc.stdout)
                raise RuntimeError(
                    "Failed to configure project to get the version"
                ) from exc

            # Get the generated replies.
            index = load_reply_dir(reply_dir)

            # Get the version from the CMAKE_PROJECT_VERSION variable.
            entries = [
                entry
                for entry in index.reply.cache_v2.entries
                if entry.name == "CMAKE_PROJECT_VERSION"
            ]

            if not entries:
                raise ValueError("Could not find OpenEXR version from CMake project")

            if len(entries) > 1:
                raise ValueError("More than one entry for CMAKE_PROJECT_VERSION found...")

            version = entries[0].value

    print("openexr_skbuild_plugin: Computed version: {0}".format(version))

    return version
