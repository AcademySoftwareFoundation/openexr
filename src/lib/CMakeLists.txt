# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

message(STATUS "Building OpenEXR libraries")

add_subdirectory( Iex )
add_subdirectory( IlmThread )

add_subdirectory( OpenEXRCore )
add_subdirectory( OpenEXR )
add_subdirectory( OpenEXRUtil )
