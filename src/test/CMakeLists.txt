# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# We require this to get object library link library support and
# combined python 2 + 3 support

if (OPENEXR_TEST_LIBRARIES)
  add_subdirectory(IexTest)
  add_subdirectory(OpenEXRCoreTest)
  add_subdirectory(OpenEXRTest)
  add_subdirectory(OpenEXRUtilTest)
  add_subdirectory(OpenEXRFuzzTest)
endif()

if (OPENEXR_BUILD_TOOLS AND OPENEXR_TEST_TOOLS)
  add_subdirectory(bin)
endif()

