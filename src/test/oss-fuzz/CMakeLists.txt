# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

#
# Build the oss-fuzz fuzzers
#
# This is configured only if OPENEXR_OSS_FUZZ_LIB_FUZZING_ENGINE
# is set, which is expected to the oss-fuzz fuzzer link archive.
# The "oss_fuzz" target builds the fuzzers.
#
# This is generally invoked only by the oss-fuzz project and executed here:
#
# https://github.com/google/oss-fuzz/blob/master/projects/openexr/build.sh
#

set(OPENEXR_FUZZERS openexr_exrcheck_fuzzer openexr_exrcorecheck_fuzzer)
add_custom_target(oss_fuzz ALL
  DEPENDS openexr_exrcheck_fuzzer openexr_exrcorecheck_fuzzer
)

foreach(fuzzer IN LISTS OPENEXR_FUZZERS)

  add_executable(${fuzzer} ${fuzzer}.cc)
  target_link_libraries(${fuzzer} PRIVATE OpenEXR::OpenEXR OpenEXR::OpenEXRUtil ${OPENEXR_OSS_FUZZ_LIB_FUZZING_ENGINE})
  install(TARGETS ${fuzzer}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT oss_fuzz
  )

endforeach()

