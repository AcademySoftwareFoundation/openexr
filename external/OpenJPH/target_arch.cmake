# This is to detect the target architecture.
# The detection relies on the compiler's "#error" preprocessor directive to emit the architecture.

# This is inspired by https://github.com/axr/solar-cmake/blob/master/TargetArch.cmake
# which is inspired by 
# https://qt.gitorious.org/qt/qtbase/blobs/master/src/corelib/global/qprocessordetection.h

set(archdetect_c_code "
#if defined(__arm__) || defined(__TARGET_ARCH_ARM)  \
  || defined(__aarch64__) || defined(_M_ARM64)
  #error cmake_ARCH OJPH_ARCH_ARM
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
  #error cmake_ARCH OJPH_ARCH_I386
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
  #error cmake_ARCH OJPH_ARCH_X86_64
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
  #error cmake_ARCH OJPH_ARCH_IA64
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) \\
  || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \\
  || defined(_M_MPPC) || defined(_M_PPC)
  #if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
    #error cmake_ARCH OJPH_ARCH_PPC64
  #else
    #error cmake_ARCH OJPH_ARCH_PPC
  #endif
#endif

#error cmake_ARCH OJPH_ARCH_UNKNOWN
")

function(target_architecture output_var)

  file(WRITE "${CMAKE_BINARY_DIR}/arch.c" "${archdetect_c_code}")

  enable_language(C)

  # Detect the architecture in a rather creative way...
  # This compiles a small C program which is a series of ifdefs that selects a
  # particular #error preprocessor directive whose message string contains the
  # target architecture. The program will always fail to compile (both because
  # file is not a valid C program, and obviously because of the presence of the
  # #error preprocessor directives... but by exploiting the preprocessor in this
  # way, we can detect the correct target architecture even when cross-compiling,
  # since the program itself never needs to be run (only the compiler/preprocessor)
  try_run(
      run_result_unused
      compile_result_unused
      "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/arch.c"
      COMPILE_OUTPUT_VARIABLE ARCH
  )

  # Parse the architecture name from the compiler output
  string(REGEX MATCH "cmake_ARCH ([a-zA-Z0-9_]+)" ARCH "${ARCH}")

  # Get rid of the value marker leaving just the architecture name
  string(REPLACE "cmake_ARCH " "" ARCH "${ARCH}")

  # If we are compiling with an unknown architecture this variable should
  # already be set to "unknown" but in the case that it's empty (i.e. due
  # to a typo in the code), then set it to unknown
  if (NOT ARCH)
      set(ARCH OJPH_ARCH_UNKNOWN)
  endif()

  set(${output_var} "${ARCH}" PARENT_SCOPE)

endfunction()