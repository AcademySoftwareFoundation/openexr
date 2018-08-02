
# adapted from FindOpenEXR.cmake in Pixar's USD distro.
# 
# The original license is as follows:
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

find_path(OPENEXR_INCLUDE_DIR
    OpenEXR/ImfHeader.h

HINTS
    "${OPENEXR_LOCATION}"
    "${ILMBASE_LOCATION}"
    "$ENV{OPENEXR_LOCATION}"
    "$ENV{OPENEXR_ROOT}"
    "$ENV{ILMBASE_LOCATION}"
    "$ENV{ILMBASE_ROOT}"

PATH_SUFFIXES
    include/

NO_DEFAULT_PATH
NO_SYSTEM_ENVIRONMENT_PATH

DOC
    "OpenEXR headers path"
)

if(OPENEXR_INCLUDE_DIR)
  set(openexr_config_file "${OPENEXR_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
  if(EXISTS ${openexr_config_file})
      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_STRING.*$")
      string(REGEX MATCHALL "[0-9.]+" OPENEXR_VERSION ${TMP})

      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MAJOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MAJOR_VERSION ${TMP})

      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MINOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MINOR_VERSION ${TMP})
  endif()
else()
    message(WARNING, " OpenEXR headers not found")
endif()

foreach(OPENEXR_LIB
    IlmImf
    IlmImfUtil)

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}
            ${OPENEXR_LIB}
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        DOC
            "OPENEXR's ${OPENEXR_LIB} library path"
    )
    #mark_as_advanced(OPENEXR_${OPENEXR_LIB}_LIBRARY)

    if(OPENEXR_${OPENEXR_LIB}_LIBRARY)
        list(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
    endif()

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_d
            ${OPENEXR_LIB}_d
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
            debug/lib/
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        DOC
            "OPENEXR's ${OPENEXR_LIB} debug library path"
    )
    #mark_as_advanced(OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY)

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${OPENEXR_LIB}_STATIC_LIBRARY
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_s
            ${OPENEXR_LIB}_s
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        DOC
            "OPENEXR's ${OPENEXR_LIB} static library path"
    )
    #mark_as_advanced(OPENEXR_${OPENEXR_LIB}_STATIC_LIBRARY)

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${OPENEXR_LIB}_STATIC_DEBUG_LIBRARY
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_s_d
            ${OPENEXR_LIB}_s_d
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
            debug/lib/
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        DOC
            "OPENEXR's ${OPENEXR_LIB} static debug library path"
    )
    #mark_as_advanced(OPENEXR_${OPENEXR_LIB}_STATIC_DEBUG_LIBRARY)

endforeach(OPENEXR_LIB)

# So #include <half.h> works
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR})
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR}/OpenEXR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenEXR
    REQUIRED_VARS
        OPENEXR_INCLUDE_DIRS
        OPENEXR_LIBRARIES
    VERSION_VAR
        OPENEXR_VERSION
)

foreach(OPENEXR_LIB
    IlmImf
    IlmImfUtil)

    if (OPENEXR_${OPENEXR_LIB}_LIBRARY)
      add_library(OpenEXR::${OPENEXR_LIB} SHARED IMPORTED)
      set_target_properties(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED_LOCATION_RELEASE ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
      set_target_properties(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED_LOCATION_DEBUG ${OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY})
      set_target_properties(OpenEXR::${OPENEXR_LIB} PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
      set_property(TARGET   OpenEXR::${OPENEXR_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENEXR_INCLUDE_DIR})
      set_property(TARGET   OpenEXR::${OPENEXR_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENEXR_INCLUDE_DIR}/OpenEXR)
    endif()

    if (OPENEXR_${OPENEXR_LIB}_STATIC_LIBRARY)
      add_library(OpenEXR::${OPENEXR_LIB}_static SHARED IMPORTED)
      set_target_properties(OpenEXR::${OPENEXR_LIB}_static PROPERTIES IMPORTED_LOCATION_RELEASE ${OPENEXR_${OPENEXR_LIB}_STATIC_LIBRARY})
      set_target_properties(OpenEXR::${OPENEXR_LIB}_static PROPERTIES IMPORTED_LOCATION_DEBUG ${OPENEXR_${OPENEXR_LIB}_STATIC_DEBUG_LIBRARY})
      set_target_properties(OpenEXR::${OPENEXR_LIB}_static PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
      set_property(TARGET   OpenEXR::${OPENEXR_LIB}_static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENEXR_INCLUDE_DIR})
      set_property(TARGET   OpenEXR::${OPENEXR_LIB}_static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENEXR_INCLUDE_DIR}/OpenEXR)
    endif()

    if (NOT OPENEXR_${OPENEXR_LIB}_LIBRARY AND NOT OPENEXR_${OPENEXR_LIB}_STATIC_LIBRARY)
      message(WARNING, "${OPENEXR_LIB} was not found.")
    endif()

endforeach()
