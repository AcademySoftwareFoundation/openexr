# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

set(OPENEXR_FUZZERS openexr_exrcheck_fuzzer openexr_exrcorecheck_fuzzer)

foreach(fuzzer IN LISTS OPENEXR_FUZZERS)

  add_executable(${fuzzer} ${fuzzer}.cc)
  target_link_libraries(${fuzzer} PRIVATE OpenEXR::OpenEXR OpenEXR::OpenEXRUtil ${OPENEXR_OSS_FUZZ_LIB_FUZZING_ENGINE})
  install(TARGETS ${fuzzer} DESTINATION ${CMAKE_INSTALL_BINDIR})

endforeach()

