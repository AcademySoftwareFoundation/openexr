# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

##########################
# Binaries / Utilities
##########################
if(OPENEXR_BUILD_TOOLS)
  add_subdirectory( exr2aces )
  add_subdirectory( exrheader )
  add_subdirectory( exrinfo )
  add_subdirectory( exrmaketiled )
  add_subdirectory( exrstdattr )
  add_subdirectory( exrmakepreview )
  add_subdirectory( exrenvmap )
  add_subdirectory( exrmultiview )
  add_subdirectory( exrmultipart )
  add_subdirectory( exrcheck )
endif()
