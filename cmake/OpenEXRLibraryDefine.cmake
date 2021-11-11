# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# NB: This function has a number of specific names / variables
# in it, so be careful copying...
function(OPENEXR_DEFINE_LIBRARY libname)
  set(options)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR)
  set(multiValueArgs SOURCES HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(OPENEXR_CURLIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(objlib ${libname})
  add_library(${objlib}
    ${OPENEXR_CURLIB_HEADERS}
    ${OPENEXR_CURLIB_SOURCES})

  # we use atomics internally for the core library, so need C11, but
  # is only a private requirement
  target_compile_features(${objlib} PRIVATE c_std_11)
  target_compile_features(${objlib} PUBLIC cxx_std_${OPENEXR_CXX_STANDARD})
  if(OPENEXR_CURLIB_PRIV_EXPORT AND BUILD_SHARED_LIBS)
    target_compile_definitions(${objlib} PRIVATE ${OPENEXR_CURLIB_PRIV_EXPORT})
    if(WIN32)
      target_compile_definitions(${objlib} PUBLIC OPENEXR_DLL)
    endif()
  endif()
  if(OPENEXR_CURLIB_CURDIR)
    target_include_directories(${objlib} INTERFACE $<BUILD_INTERFACE:${OPENEXR_CURLIB_CURDIR}>)
  endif()
  if(OPENEXR_CURLIB_CURBINDIR)
    target_include_directories(${objlib} PRIVATE $<BUILD_INTERFACE:${OPENEXR_CURLIB_CURBINDIR}>)
  endif()
  target_link_libraries(${objlib} PUBLIC ${PROJECT_NAME}::Config ${OPENEXR_CURLIB_DEPENDENCIES})
  if(OPENEXR_CURLIB_PRIVATE_DEPS)
    target_link_libraries(${objlib} PRIVATE ${OPENEXR_CURLIB_PRIVATE_DEPS})
  endif()
  set_target_properties(${objlib} PROPERTIES
    C_STANDARD_REQUIRED ON
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )
  set_property(TARGET ${objlib} PROPERTY PUBLIC_HEADER ${OPENEXR_CURLIB_HEADERS})

  if(BUILD_SHARED_LIBS)
    set_target_properties(${libname} PROPERTIES
      SOVERSION ${OPENEXR_LIB_SOVERSION}
      VERSION ${OPENEXR_LIB_VERSION}
    )
  endif()
  set_target_properties(${libname} PROPERTIES
      OUTPUT_NAME "${libname}${OPENEXR_LIB_SUFFIX}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  add_library(${PROJECT_NAME}::${libname} ALIAS ${libname})

  if(OPENEXR_INSTALL)
    install(TARGETS ${libname}
      EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}
    )
  endif()
  if(BUILD_SHARED_LIBS AND (NOT "${OPENEXR_LIB_SUFFIX}" STREQUAL "") AND NOT WIN32)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
    set(verlibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${OPENEXR_LIB_SUFFIX}${CMAKE_${uppercase_CMAKE_BUILD_TYPE}_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(baselibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${CMAKE_${uppercase_CMAKE_BUILD_TYPE}_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E chdir \"\$ENV\{DESTDIR\}${CMAKE_INSTALL_FULL_LIBDIR}\" ${CMAKE_COMMAND} -E create_symlink ${verlibname} ${baselibname})")
    install(CODE "message(STATUS \"Creating symlink ${CMAKE_INSTALL_FULL_DIR}/${baselibname} -> ${verlibname}\")")
    set(verlibname)
    set(baselibname)
  endif()

endfunction()
