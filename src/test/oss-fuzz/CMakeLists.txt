# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

#
# Build the oss-fuzz fuzzers
#
# This is configured only if OPENEXR_BUILD_OSS_FUZZ is set _and_ the
# environment variable LIB_FUZZING_ENGINE is set, which is expected to
# the oss-fuzz fuzzer link archive.  The "oss_fuzz" target builds the
# fuzzers. The OUT environment variable specifies the installation
# directory (set by oss-fuzz).
#
# This is generally invoked only by the oss-fuzz project and executed here:
#
# https://github.com/google/oss-fuzz/blob/master/projects/openexr/build.sh
#

message(STATUS "Configuring oss_fuzz with $ENV{LIB_FUZZING_ENGINE}")
message(STATUS "SRC=$ENV{SRC}")
message(STATUS "WORK=$ENV{WORK}")
message(STATUS "OUT=$ENV{OUT}")
message(status "CXX=$ENV{CXX}")
message(status "CXX_FLAGS=$ENV{CXX_FLAGS}")
message(status "CC=$ENV{CC}")
message(status "CC_FLAGS=$ENV{CC_FLAGS}")

set(OPENEXR_FUZZERS openexr_exrcheck_fuzzer openexr_exrcorecheck_fuzzer)
add_custom_target(oss_fuzz ALL
  DEPENDS openexr_exrcheck_fuzzer openexr_exrcorecheck_fuzzer
)

foreach(fuzzer IN LISTS OPENEXR_FUZZERS)

  add_executable(${fuzzer} ${fuzzer}.cc)
  target_link_libraries(${fuzzer}
    PRIVATE OpenEXR::OpenEXR OpenEXR::OpenEXRUtil $ENV{LIB_FUZZING_ENGINE})
  install(TARGETS ${fuzzer}
    DESTINATION $ENV{OUT}
    COMPONENT oss_fuzz
  )

endforeach()

