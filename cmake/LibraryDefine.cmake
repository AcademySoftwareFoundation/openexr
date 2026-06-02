# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# NB: This function has a number of Imath-specific names/variables
# in it, so be careful copying...
function(OPENEXR_DEFINE_LIBRARY libname)
  set(options EMBEDDED)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR)
  set(multiValueArgs SOURCES HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(OPENEXR_CURLIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (MSVC)
    set(_openexr_extra_flags "$<$<COMPILE_LANGUAGE:CXX>:/EHsc>" "$<$<COMPILE_LANGUAGE:CXX>:/MP>")
  endif()
  set(objlib ${libname})
  if(OPENEXR_CURLIB_EMBEDDED)
    set(libopts STATIC)
  else()
    set(libopts)
  endif()
  add_library(${objlib} ${libopts}
    ${OPENEXR_CURLIB_HEADERS}
    ${OPENEXR_CURLIB_SOURCES})

  # Use ${OPENEXR_CXX_STANDARD} to determine the standard we use to compile
  # OpenEXR itself. The headers will use string_view and such, so ensure
  # the user is at least 17, but could be higher
  target_compile_features(${objlib}
                          PRIVATE cxx_std_${OPENEXR_CXX_STANDARD}
                          INTERFACE cxx_std_17 )

  if(OPENEXR_CURLIB_EMBEDDED)
    set(libopts)
  elseif(OPENEXR_CURLIB_PRIV_EXPORT AND BUILD_SHARED_LIBS)
    target_compile_definitions(${objlib} PRIVATE ${OPENEXR_CURLIB_PRIV_EXPORT})
    if(WIN32)
      target_compile_definitions(${objlib} PUBLIC OPENEXR_DLL)
    endif()
  endif()

  if(OPENEXR_CURLIB_CURDIR)
    target_include_directories(${objlib} INTERFACE $<BUILD_INTERFACE:${OPENEXR_CURLIB_CURDIR}>)

    # When an application builds against OpenEXR via
    # add_subdirector(OpenEXR), if should still recognize include
    # statements with the OpenEXR subdirectory, ie. #include
    # <OpenEXR/ImfHeader.h>, altough in that configuration, there is
    # no OpenEXR subdirectory, since it is the install step that
    # places headers there; for an in-tree build, the headers are in
    # each library.  To allow such an application to use the #include
    # <OpenEXR/...> construct, create a directory in the build root
    # with symbolic links to the headers in the library directories.
    #
    # We detect this condition via the OPENEXR_IS_SUBPROJECT setting,
    # which is set in OpenEXRSetup.cmake when CMAKE_PROJECT_NAME is
    # not "OpenEXR".
    
    if(OPENEXR_IS_SUBPROJECT AND OPENEXR_CURLIB_HEADERS)
      foreach(_hdr IN LISTS OPENEXR_CURLIB_HEADERS)
        if(IS_ABSOLUTE "${_hdr}")
          set(_hdr_src "${_hdr}")
        else()
          set(_hdr_src "${OPENEXR_CURLIB_CURDIR}/${_hdr}")
        endif()
        get_filename_component(_hdr_name "${_hdr}" NAME)
        set(_hdr_dst "${OPENEXR_BUILD_INTERFACE_UNIFIED}/${OPENEXR_OUTPUT_SUBDIR}/${_hdr_name}")
        if(EXISTS "${_hdr_src}")
          file(REMOVE "${_hdr_dst}")
          if(CMAKE_HOST_UNIX)
            file(CREATE_LINK "${_hdr_src}" "${_hdr_dst}" SYMBOLIC)
          else()
            execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_hdr_src}" "${_hdr_dst}")
          endif()
        else()
          message(WARNING "OpenEXR build-interface include: missing public header ${_hdr_src}")
        endif()
      endforeach()
    endif()
    if(OPENEXR_IS_SUBPROJECT)
      target_include_directories(${objlib} INTERFACE $<BUILD_INTERFACE:${OPENEXR_BUILD_INTERFACE_UNIFIED}>)
    endif()
  endif()
  if(OPENEXR_CURLIB_CURBINDIR)
    target_include_directories(${objlib} PRIVATE $<BUILD_INTERFACE:${OPENEXR_CURLIB_CURBINDIR}>)
  endif()
  target_link_libraries(${objlib} PUBLIC ${PROJECT_NAME}::Config ${OPENEXR_CURLIB_DEPENDENCIES} ${CMAKE_DL_LIBS})
  if(OPENEXR_CURLIB_PRIVATE_DEPS)
    target_link_libraries(${objlib} PRIVATE ${OPENEXR_CURLIB_PRIVATE_DEPS})
  endif()
  set_target_properties(${objlib} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )
  if (OPENEXR_CURLIB_EMBEDDED OR NOT OPENEXR_USE_DEFAULT_VISIBILITY)
    set_target_properties(${objlib} PROPERTIES
      C_VISIBILITY_PRESET hidden
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
      )
  else()
      target_compile_definitions(${objlib} PUBLIC OPENEXR_USE_DEFAULT_VISIBILITY)
  endif()
  if (_openexr_extra_flags)
    target_compile_options(${objlib} PRIVATE ${_openexr_extra_flags})
  endif()
  set_property(TARGET ${objlib} PROPERTY PUBLIC_HEADER ${OPENEXR_CURLIB_HEADERS})

  if(BUILD_SHARED_LIBS AND NOT OPENEXR_CURLIB_EMBEDDED)
    set_target_properties(${libname} PROPERTIES
      SOVERSION ${OPENEXR_LIB_SOVERSION}
      VERSION ${OPENEXR_LIB_VERSION}
    )
  endif()
  set_target_properties(${libname} PROPERTIES
      OUTPUT_NAME "${libname}${OPENEXR_LIB_SUFFIX}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

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
  if(OPENEXR_CURLIB_EMBEDDED)
    set(libopts)
  elseif(BUILD_SHARED_LIBS AND (NOT "${OPENEXR_LIB_SUFFIX}" STREQUAL "") AND NOT WIN32)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
    set(verlibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${OPENEXR_LIB_SUFFIX}${CMAKE_${uppercase_CMAKE_BUILD_TYPE}_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(baselibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${CMAKE_${uppercase_CMAKE_BUILD_TYPE}_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    file(CREATE_LINK ${verlibname} ${CMAKE_CURRENT_BINARY_DIR}/${baselibname} SYMBOLIC)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${baselibname} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(CODE "message(STATUS \"Creating symlink ${CMAKE_INSTALL_LIBDIR}/${baselibname} -> ${verlibname}\")")
    set(verlibname)
    set(baselibname)
  endif()

  add_library(${PROJECT_NAME}::${libname} ALIAS ${libname})
endfunction()
