# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# NB: This function has a number of specific names / variables
# in it, so be careful copying...
function(OPENEXR_DEFINE_LIBRARY libname)
  set(options)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR)
  set(multiValueArgs SOURCES HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(OPENEXR_CURLIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(objlib ${libname}_Object)
  add_library(${objlib} OBJECT ${OPENEXR_CURLIB_SOURCES})
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
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )
  set_property(TARGET ${objlib} PROPERTY PUBLIC_HEADER ${OPENEXR_CURLIB_HEADERS})

  install(TARGETS ${objlib}
    EXPORT ${PROJECT_NAME}
    PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}
  )

  # let the default behaviour BUILD_SHARED_LIBS control the
  # disposition of the default library...
  add_library(${libname} $<TARGET_OBJECTS:${objlib}>)
  target_link_libraries(${libname} PUBLIC ${objlib})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${libname} PROPERTIES
      SOVERSION ${OPENEXR_SOVERSION}
      VERSION ${OPENEXR_LIB_VERSION}
      OUTPUT_NAME "${libname}${OPENEXR_LIB_SUFFIX}"
    )
  endif()
  add_library(${PROJECT_NAME}::${libname} ALIAS ${libname})

  install(TARGETS ${libname}
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  if(OPENEXR_BUILD_BOTH_STATIC_SHARED)
    add_library(${libname}_static STATIC $<TARGET_OBJECTS:${objlib}>)
    target_link_libraries(${libname}_static INTERFACE ${objlib})
    set_target_properties(${libname}_static PROPERTIES
      SOVERSION ${OPENEXR_SOVERSION}
      VERSION ${OPENEXR_LIB_VERSION}
      OUTPUT_NAME "${libname}${OPENEXR_LIB_SUFFIX}${OPENEXR_STATIC_LIB_SUFFIX}"
    )
    add_library(${PROJECT_NAME}::${libname}_static ALIAS ${libname}_static)

    install(TARGETS ${libname}_static
      EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
  endif()
endfunction()
