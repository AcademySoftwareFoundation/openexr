
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

find_path(ILMBASE_INCLUDE_DIR
    OpenEXR/Iex.h

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
    "IlmBase headers path"
)

if(ILMBASE_INCLUDE_DIR)
  set(ilmbase_config_file "${ILMBASE_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
  if(EXISTS ${ilmbase_config_file})
      file(STRINGS
           ${ilmbase_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_STRING.*$")
      string(REGEX MATCHALL "[0-9.]+" OPENEXR_VERSION ${TMP})

      file(STRINGS
           ${ilmbase_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MAJOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MAJOR_VERSION ${TMP})

      file(STRINGS
           ${ilmbase_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MINOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MINOR_VERSION ${TMP})
  endif()
else()
    message(WARNING, " IlmBase headers not found")
endif()

foreach(ILMBASE_LIB
    Half
    Iex
    IexMath
    Imath
    IlmThread)

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${ILMBASE_LIB}_LIBRARY
        NAMES
            ${ILMBASE_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}
            ${ILMBASE_LIB}
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        DOC
            "OPENEXR's ${ILMBASE_LIB} library path"
    )

    if(OPENEXR_${ILMBASE_LIB}_LIBRARY)
        list(APPEND ILMBASE_LIBRARIES ${OPENEXR_${ILMBASE_LIB}_LIBRARY})
    endif()

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${ILMBASE_LIB}_DEBUG_LIBRARY
        NAMES
            ${ILMBASE_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_d
            ${ILMBASE_LIB}_d
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
            debug/lib/
        DOC
            "OPENEXR's ${ILMBASE_LIB} debug library path"
    )

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${ILMBASE_LIB}_STATIC_LIBRARY
        NAMES
            ${ILMBASE_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_s
            ${ILMBASE_LIB}_s
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        DOC
            "OPENEXR's ${ILMBASE_LIB} static library path"
    )

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${ILMBASE_LIB}_STATIC_DEBUG_LIBRARY
        NAMES
            ${ILMBASE_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_s_d
            ${ILMBASE_LIB}_s_d
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
            debug/lib/
        DOC
            "OPENEXR's ${ILMBASE_LIB} static debug library path"
    )

endforeach(ILMBASE_LIB)

# So #include <half.h> works
list(APPEND ILMBASE_INCLUDE_DIRS ${ILMBASE_INCLUDE_DIR})
list(APPEND ILMBASE_INCLUDE_DIRS ${ILMBASE_INCLUDE_DIR}/OpenEXR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IlmBase
    REQUIRED_VARS
        ILMBASE_INCLUDE_DIR
        ILMBASE_LIBRARIES
    VERSION_VAR
        OPENEXR_VERSION
)

foreach(ILMBASE_LIB
    Half
    Iex
    IexMath
    Imath
    IlmThread)

    if (OPENEXR_${ILMBASE_LIB}_LIBRARY)
        add_library(IlmBase::${ILMBASE_LIB} SHARED IMPORTED)
        set_target_properties(IlmBase::${ILMBASE_LIB} PROPERTIES IMPORTED_LOCATION_RELEASE ${OPENEXR_${ILMBASE_LIB}_LIBRARY})
        set_target_properties(IlmBase::${ILMBASE_LIB} PROPERTIES IMPORTED_LOCATION_DEBUG ${OPENEXR_${ILMBASE_LIB}_DEBUG_LIBRARY})
        set_target_properties(IlmBase::${ILMBASE_LIB} PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
        set_property(TARGET   IlmBase::${ILMBASE_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIR})
        set_property(TARGET   IlmBase::${ILMBASE_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIR}/OpenEXR)
    endif()

    if (OPENEXR_${ILMBASE_LIB}_STATIC_LIBRARY)
        add_library(IlmBase::${ILMBASE_LIB}_static SHARED IMPORTED)
        set_target_properties(IlmBase::${ILMBASE_LIB}_static PROPERTIES IMPORTED_LOCATION_RELEASE ${OPENEXR_${ILMBASE_LIB}_STATIC_LIBRARY})
        set_target_properties(IlmBase::${ILMBASE_LIB}_static PROPERTIES IMPORTED_LOCATION_DEBUG ${OPENEXR_${ILMBASE_LIB}_STATIC_DEBUG_LIBRARY})
        set_target_properties(IlmBase::${ILMBASE_LIB}_static PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
        set_property(TARGET   IlmBase::${ILMBASE_LIB}_static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIR})
        set_property(TARGET   IlmBase::${ILMBASE_LIB}_static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIR}/OpenEXR)
    endif()

    if (NOT OPENEXR_${ILMBASE_LIB}_LIBRARY AND NOT OPENEXR_${ILMBASE_LIB}_STATIC_LIBRARY)
      message(WARNING, "${ILMBASE_LIB} was not found.")
    endif()

endforeach()
