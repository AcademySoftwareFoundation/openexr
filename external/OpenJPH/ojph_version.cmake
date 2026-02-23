################################################################################################
# Generating ojph library version number
################################################################################################

############################################################
# Parse version file
# credit: https://stackoverflow.com/a/47084079

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/core/openjph/ojph_version.h" VERFILE)
if (NOT VERFILE)
    message(FATAL_ERROR "Failed to parse ojph_version.h!")
endif()

string(REGEX MATCH "OPENJPH_VERSION_MAJOR ([0-9]*)" _ ${VERFILE})
set(OPENJPH_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "OPENJPH_VERSION_MINOR ([0-9]*)" _ ${VERFILE})
set(OPENJPH_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "OPENJPH_VERSION_PATCH ([0-9]*)" _ ${VERFILE})
set(OPENJPH_VERSION_PATCH ${CMAKE_MATCH_1})

set(OPENJPH_VERSION "${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}.${OPENJPH_VERSION_PATCH}")
############################################################

message(STATUS "OpenJPH library version: ${OPENJPH_VERSION}")

if (OPENJPH_VERSION)
else()
  message(FATAL_ERROR "OPENJPH_VERSION is not set")
endif()
