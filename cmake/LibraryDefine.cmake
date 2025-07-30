# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# NB: This function has a number of Imath-specific names/variables
# in it, so be careful copying...
function(OPENEXR_DEFINE_LIBRARY libname)
  set(options)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR)
  set(multiValueArgs SOURCES HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(OPENEXR_CURLIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (MSVC)
    set(_openexr_extra_flags "$<$<COMPILE_LANGUAGE:CXX>:/EHsc>" "$<$<COMPILE_LANGUAGE:CXX>:/MP>")
  endif()
  set(objlib ${libname})
  add_library(${objlib}
    ${OPENEXR_CURLIB_HEADERS}
    ${OPENEXR_CURLIB_SOURCES})

  # Use ${OPENEXR_CXX_STANDARD} to determine the standard we use to compile
  # OpenEXR itself. The headers will use string_view and such, so ensure
  # the user is at least 17, but could be higher
  target_compile_features(${objlib}
                          PRIVATE cxx_std_${OPENEXR_CXX_STANDARD}
                          INTERFACE cxx_std_17 )

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
  target_link_libraries(${objlib} PUBLIC ${PROJECT_NAME}::Config ${OPENEXR_CURLIB_DEPENDENCIES} ${CMAKE_DL_LIBS} ${EXR_OPENJPH_LIB})
  if(OPENEXR_CURLIB_PRIVATE_DEPS)
    target_link_libraries(${objlib} PRIVATE ${OPENEXR_CURLIB_PRIVATE_DEPS})
  endif()
  set_target_properties(${objlib} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )
  if (NOT OPENEXR_USE_DEFAULT_VISIBILITY)
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

  if(BUILD_SHARED_LIBS)
    set_target_properties(${libname} PROPERTIES
      SOVERSION ${OPENEXR_LIB_SOVERSION}
      VERSION ${OPENEXR_LIB_VERSION}
    )
  endif()
  # Set OUTPUT_NAME to avoid suffix for frameworks
  set_target_properties(${libname} PROPERTIES
      OUTPUT_NAME "${libname}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  if(OPENEXR_FRAMEWORK)
    # Mark resource files for inclusion in the framework bundle
    set_source_files_properties(${OPENEXR_RESOURCES} PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
    )
    set_target_properties(${libname} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION "${OPENEXR_VERSION_FULL}"
      PRODUCT_BUNDLE_IDENTIFIER "github.com/AcademySoftwareFoundation/openexr/${libname}"
      XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
      XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
      XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
      MACOSX_FRAMEWORK_IDENTIFIER "github.com/AcademySoftwareFoundation/openexr/${libname}"
      MACOSX_FRAMEWORK_BUNDLE_VERSION "${OPENEXR_VERSION_FULL}"
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${OPENEXR_VERSION_API}"
      MACOSX_RPATH TRUE
    )
    configure_framework(${libname} "${OPENEXR_RESOURCES}")
  endif()
  add_library(${PROJECT_NAME}::${libname} ALIAS ${libname})

  if(OPENEXR_INSTALL)
    install(TARGETS ${libname}
      EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}
    )
    if(OPENEXR_FRAMEWORK)
      install(FILES ${OPENEXR_RESOURCES}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/${libname}.framework/Resources"
      )
    endif()
  endif()
  if(BUILD_SHARED_LIBS AND (NOT "${OPENEXR_LIB_SUFFIX}" STREQUAL "") AND NOT WIN32 AND NOT OPENEXR_FRAMEWORK)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
    set(verlibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${OPENEXR_LIB_SUFFIX}${CMAKE_${uppercase_CMAKE_BUILD_TYPE}_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(baselibname ${CMAKE_SHARED_LIBRARY_PREFIX}${libname}${CMAKE_${uppercase_CMAKE_BUILD_TYPE}_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX})
    file(CREATE_LINK ${verlibname} ${CMAKE_CURRENT_BINARY_DIR}/${baselibname} SYMBOLIC)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${baselibname} DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
    install(CODE "message(STATUS \"Creating symlink ${CMAKE_INSTALL_FULL_LIBDIR}/${baselibname} -> ${verlibname}\")")
    set(verlibname)
    set(baselibname)
  endif()
endfunction()

function(configure_framework libname resources)
  if(OPENEXR_FRAMEWORK)
    set(RES_DEST_DIR "$<TARGET_BUNDLE_CONTENT_DIR:${libname}>/Resources")
    message(STATUS "Configuring framework for ${libname}, copying resources to ${RES_DEST_DIR}")
    message(STATUS "Resources to copy: ${resources}")
    add_custom_command(TARGET ${libname} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${RES_DEST_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${resources} "${RES_DEST_DIR}/"
      COMMAND ${CMAKE_COMMAND} -E echo "Copied resources: ${resources} to ${RES_DEST_DIR}"
      COMMENT "Copying resource files to ${libname}.framework/Resources in build directory"
      VERBATIM
    )
  endif()
endfunction()