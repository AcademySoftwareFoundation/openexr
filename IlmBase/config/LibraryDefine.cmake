# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# NB: This function has a number if IlmBase specific names / variables
# in it, so be careful copying...
function(ILMBASE_DEFINE_LIBRARY libname)
  set(options)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR)
  set(multiValueArgs SOURCES HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(ILMBASE_CURLIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # only do the object library mechanism in a few cases:
  # - xcode doesn't handle "empty" targets (i.e. add_library with
  #   an object lib only)
  # - if we're not compiling both, don't add the extra layer to prevent
  #   extra compiles since we aren't doing that anyway
  if(ILMBASE_BUILD_BOTH_STATIC_SHARED AND NOT (APPLE OR WIN32))
    set(use_objlib TRUE)
  else()
    set(use_objlib)
  endif()
  if (MSVC)
    set(_ilmbase_extra_flags "/EHsc")
  endif()
  if(use_objlib)
    set(objlib ${libname}_Object)
    add_library(${objlib} OBJECT
      ${ILMBASE_CURLIB_HEADERS}
      ${ILMBASE_CURLIB_SOURCES})
  else()
    set(objlib ${libname})
    add_library(${objlib}
      ${ILMBASE_CURLIB_HEADERS}
      ${ILMBASE_CURLIB_SOURCES})
  endif()

  target_compile_features(${objlib} PUBLIC cxx_std_${OPENEXR_CXX_STANDARD})
  if(ILMBASE_CURLIB_PRIV_EXPORT AND BUILD_SHARED_LIBS)
    target_compile_definitions(${objlib} PRIVATE ${ILMBASE_CURLIB_PRIV_EXPORT})
    if(WIN32 AND NOT ILMBASE_BUILD_BOTH_STATIC_SHARED)
      target_compile_definitions(${objlib} PUBLIC OPENEXR_DLL)
    endif()
  endif()
  if(ILMBASE_CURLIB_CURDIR)
    target_include_directories(${objlib} INTERFACE $<BUILD_INTERFACE:${ILMBASE_CURLIB_CURDIR}>)
  endif()
  if(ILMBASE_CURLIB_CURBINDIR)
    target_include_directories(${objlib} PRIVATE $<BUILD_INTERFACE:${ILMBASE_CURLIB_CURBINDIR}>)
  endif()
  target_link_libraries(${objlib} PUBLIC ${PROJECT_NAME}::Config ${ILMBASE_CURLIB_DEPENDENCIES})
  if(ILMBASE_CURLIB_PRIVATE_DEPS)
    target_link_libraries(${objlib} PRIVATE ${ILMBASE_CURLIB_PRIVATE_DEPS})
  endif()
  set_target_properties(${objlib} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )
  if (_ilmbase_extra_flags)
    target_compile_options(${objlib} PUBLIC ${_ilmbase_extra_flags})
  endif()
  set_property(TARGET ${objlib} PROPERTY PUBLIC_HEADER ${ILMBASE_CURLIB_HEADERS})

  if(use_objlib)
    install(TARGETS ${objlib}
      EXPORT ${PROJECT_NAME}
      PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ILMBASE_OUTPUT_SUBDIR}
    )
  endif()

  # let the default behaviour BUILD_SHARED_LIBS control the
  # disposition of the default library...
  if(use_objlib)
    add_library(${libname} $<TARGET_OBJECTS:${objlib}>)
    target_link_libraries(${libname} PUBLIC ${objlib})
  endif()
  if(BUILD_SHARED_LIBS)
    set_target_properties(${libname} PROPERTIES
      SOVERSION ${ILMBASE_SOVERSION}
      VERSION ${ILMBASE_LIB_VERSION}
    )
  endif()
  set_target_properties(${libname} PROPERTIES
      OUTPUT_NAME "${libname}${ILMBASE_LIB_SUFFIX}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  add_library(${PROJECT_NAME}::${libname} ALIAS ${libname})

  install(TARGETS ${libname}
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ILMBASE_OUTPUT_SUBDIR}
  )
  if(BUILD_SHARED_LIBS AND (NOT "${ILMBASE_LIB_SUFFIX}" STREQUAL "") AND NOT WIN32)
    set(verlibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${ILMBASE_LIB_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(baselibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${CMAKE_SHARED_LIBRARY_SUFFIX})
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E chdir \"\$ENV\{DESTDIR\}${CMAKE_INSTALL_FULL_LIBDIR}\" ${CMAKE_COMMAND} -E create_symlink ${verlibname} ${baselibname})")
    install(CODE "message(\"-- Creating symlink in ${CMAKE_INSTALL_FULL_LIBDIR} ${baselibname} -> ${verlibname}\")")
    set(verlibname)
    set(baselibname)
  endif()

  if(ILMBASE_BUILD_BOTH_STATIC_SHARED)
    if(use_objlib)
      add_library(${libname}_static STATIC $<TARGET_OBJECTS:${objlib}>)
      target_link_libraries(${libname}_static INTERFACE ${objlib})
    else()
      # have to build multiple times... but have different flags anyway (i.e. no dll)
      target_compile_definitions(${libname} PRIVATE OPENEXR_DLL)
      set(curlib ${libname}_static)
      add_library(${curlib} STATIC ${ILMBASE_CURLIB_SOURCES})
      target_compile_features(${curlib} PUBLIC cxx_std_${OPENEXR_CXX_STANDARD})
      if(ILMBASE_CURLIB_CURDIR)
        target_include_directories(${curlib} INTERFACE $<BUILD_INTERFACE:${ILMBASE_CURLIB_CURDIR}>)
      endif()
      if(ILMBASE_CURLIB_CURBINDIR)
        target_include_directories(${curlib} PRIVATE $<BUILD_INTERFACE:${ILMBASE_CURLIB_CURBINDIR}>)
      endif()
      target_link_libraries(${curlib} PUBLIC ${PROJECT_NAME}::Config ${ILMBASE_CURLIB_DEPENDENCIES})
      if(ILMBASE_CURLIB_PRIVATE_DEPS)
        target_link_libraries(${curlib} PRIVATE ${ILMBASE_CURLIB_PRIVATE_DEPS})
      endif()
      set(curlib)
    endif()

    set_target_properties(${libname}_static PROPERTIES
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      SOVERSION ${ILMBASE_SOVERSION}
      VERSION ${ILMBASE_LIB_VERSION}
      OUTPUT_NAME "${libname}${ILMBASE_LIB_SUFFIX}${ILMBASE_STATIC_LIB_SUFFIX}"
    )
    add_library(${PROJECT_NAME}::${libname}_static ALIAS ${libname}_static)

    install(TARGETS ${libname}_static
      EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()
endfunction()
