
function(PYILMBASE_ADD_LIBRARY_PRIV libname)
  set(options)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR OUTROOT)
  set(multiValueArgs SOURCE HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(PYILMBASE_CURLIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  set(objlib ${libname}_Object)
  add_library(${objlib} OBJECT ${PYILMBASE_CURLIB_SOURCE})
  target_compile_features(${objlib} PUBLIC cxx_std_${PYILMBASE_CXX_STANDARD})
  if(PYILMBASE_CURLIB_PRIV_EXPORT AND BUILD_SHARED_LIBS)
    target_compile_definitions(${objlib} PRIVATE ${PYILMBASE_CURLIB_PRIV_EXPORT})
    if(WIN32)
      target_compile_definitions(${objlib} PUBLIC OPENEXR_DLL)
    endif()
  endif()
  if(PYILMBASE_CURLIB_CURDIR)
    target_include_directories(${objlib} INTERFACE $<BUILD_INTERFACE:${PYILMBASE_CURLIB_CURDIR}>)
  endif()
  if(PYILMBASE_CURLIB_CURBINDIR)
    target_include_directories(${objlib} PRIVATE $<BUILD_INTERFACE:${PYILMBASE_CURLIB_CURBINDIR}>)
  endif()
  target_link_libraries(${objlib} PUBLIC ${PYILMBASE_CURLIB_DEPENDENCIES})
  if(PYILMBASE_CURLIB_PRIVATE_DEPS)
    target_link_libraries(${objlib} PRIVATE ${PYILMBASE_CURLIB_PRIVATE_DEPS})
  endif()
  set_target_properties(${objlib} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )
  set_property(TARGET ${objlib} PROPERTY PUBLIC_HEADER ${PYILMBASE_CURLIB_HEADERS})
  
  install(TARGETS ${objlib}
      EXPORT ${PROJECT_NAME}
      PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PYILMBASE_OUTPUT_SUBDIR}
  )

  # let the default behaviour BUILD_SHARED_LIBS control the
  # disposition of the default library...
  add_library(${libname} $<TARGET_OBJECTS:${objlib}>)
  target_link_libraries(${libname} PUBLIC ${objlib})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${libname} PROPERTIES
    SOVERSION ${PYILMBASE_SOVERSION}
    VERSION ${PYILMBASE_LIB_VERSION}
    )
  endif()
  set_target_properties(${libname} PROPERTIES
    OUTPUT_NAME "${PYILMBASE_OUTPUT_OUTROOT}${libname}${PYILMBASE_LIB_SUFFIX}"
  )
  add_library(${PROJECT_NAME}::${libname} ALIAS ${libname})

  install(TARGETS ${libname}
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endfunction()

# NB: This function has a number if specific names / variables
# not to mention behavior, so be careful copying...
function(PYILMBASE_DEFINE_MODULE modname)
  set(options)
  set(oneValueArgs PRIV_EXPORT CURDIR CURBINDIR LIBNAME)
  set(multiValueArgs LIBSOURCE MODSOURCE HEADERS DEPENDENCIES PRIVATE_DEPS)
  cmake_parse_arguments(PYILMBASE_CURMOD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(PYILMBASE_BUILD_SUPPORT_LIBRARIES)
    set(libarglist SOURCE ${PYILMBASE_CURMOD_LIBSOURCE})
    if(PYILMBASE_CURMOD_HEADERS)
      list(APPEND libarglist HEADERS ${PYILMBASE_CURMOD_HEADERS})
    endif()
    if(PYILMBASE_CURMOD_CURDIR)
      list(APPEND libarglist CURDIR ${PYILMBASE_CURMOD_CURDIR})
    endif()
    if(PYILMBASE_CURMOD_CURBINDIR)
      list(APPEND libarglist CURDIR ${PYILMBASE_CURMOD_CURBINDIR})
    endif()
    if(PYILMBASE_CURMOD_DEPENDENCIES)
      list(APPEND libarglist DEPENDENCIES ${PYILMBASE_CURMOD_DEPENDENCIES})
    endif()
    # NB: make this one last so we can cheat and add the python and boost
    # libs as private deps at the end regardless of whether it was provided
    list(APPEND libarglist PRIVATE_DEPS ${PYILMBASE_CURMOD_PRIVATE_DEPS})
    if(TARGET Python2::Python AND TARGET Boost::${PYILMBASE_BOOST_PY2_COMPONENT})
      set(libname "${PYILMBASE_CURMOD_LIBNAME}${PYILMBASE_LIB_PYTHONVER_ROOT}${Python2_VERSION_MAJOR}_${Python2_VERSION_MINOR}")
      pyilmbase_add_library_priv(${libname}
        OUTROOT "Python${Python2_VERSION_MAJOR}_${Python2_VERSION_MINOR}/"
        ${libarglist} Python2::Python Boost::${PYILMBASE_BOOST_PY2_COMPONENT}
      )
      Python2_add_library(${modname}_python2 MODULE ${PYILMBASE_CURMOD_MODSOURCE})
      target_link_libraries(${modname}_python2
        PRIVATE ${libname})
      set_target_properties(${modname}_python2 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python${Python2_VERSION_MAJOR}_${Python2_VERSION_MINOR}/"
        LIBRARY_OUTPUT_NAME "${modname}"
      )

      #### TODO: Define installation rules
    endif()
    if(TARGET Python3::Python AND TARGET Boost::${PYILMBASE_BOOST_PY3_COMPONENT})
      set(libname "${PYILMBASE_CURMOD_LIBNAME}${PYILMBASE_LIB_PYTHONVER_ROOT}${Python3_VERSION_MAJOR}_${Python3_VERSION_MINOR}")
      pyilmbase_add_library_priv(${libname}
        OUTROOT "Python${Python3_VERSION_MAJOR}_${Python3_VERSION_MINOR}/"
        ${libarglist} Python3::Python Boost::${PYILMBASE_BOOST_PY3_COMPONENT}
      )
      Python3_add_library(${modname}_python3 MODULE ${PYILMBASE_CURMOD_MODSOURCE})
      target_link_libraries(${modname}_python3
        PRIVATE ${libname})

      set_target_properties(${modname}_python3 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python${Python3_VERSION_MAJOR}_${Python3_VERSION_MINOR}/"
        LIBRARY_OUTPUT_NAME "${modname}"
      )
      #### TODO: Define installation rules
    endif()
  else()
    if(TARGET Python2::Python AND TARGET Boost::${PYILMBASE_BOOST_PY2_COMPONENT})
      Python2_add_library(${modname}_python2 MODULE
        ${PYILMBASE_CURMOD_LIBSOURCE}
        ${PYILMBASE_CURMOD_MODSOURCE})
      # add library will already depend on python...
      target_link_libraries(${modname}_python2
        PRIVATE
          Boost::${PYILMBASE_BOOST_PY2_COMPONENT}
          ${PYILMBASE_CURMOD_DEPENDENCIES}
          ${PYILMBASE_CURMOD_PRIVATE_DEPS})
      set_target_properties(${modname}_python2 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python${Python2_VERSION_MAJOR}_${Python2_VERSION_MINOR}/"
        LIBRARY_OUTPUT_NAME "${modname}"
      )

      #### TODO: Define installation rules
    endif()

    if(TARGET Python3::Python AND TARGET Boost::${PYILMBASE_BOOST_PY3_COMPONENT})
      Python3_add_library(${modname}_python3 MODULE 
        ${PYILMBASE_CURMOD_LIBSOURCE}
        ${PYILMBASE_CURMOD_MODSOURCE})
      # add library will already depend on python...
      target_link_libraries(${modname}_python3
        PRIVATE
          Boost::${PYILMBASE_BOOST_PY3_COMPONENT}
          ${PYILMBASE_CURMOD_DEPENDENCIES}
          ${PYILMBASE_CURMOD_PRIVATE_DEPS})
      set_target_properties(${modname}_python3 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python${Python3_VERSION_MAJOR}_${Python3_VERSION_MINOR}/"
        LIBRARY_OUTPUT_NAME "${modname}"
      )

      #### TODO: Define installation rules
    endif()
  endif()
endfunction()
