//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#if defined(ANDROID) || defined(__ANDROID_API__)
#    define IMF_TMP_DIR "/sdcard/"
#    define IMF_PATH_SEPARATOR "/"
#elif defined(_WIN32) || defined(_WIN64)
#    define IMF_TMP_DIR                                                        \
        "" // TODO: get this from GetTempPath() or env var $TEMP or $TMP
#    define IMF_PATH_SEPARATOR "\\"
#    include <direct.h> // for _mkdir, _rmdir
#    define mkdir(name, mode) _mkdir (name)
#    define rmdir _rmdir
#else
#    include <sys/stat.h> // for mkdir
#    define IMF_TMP_DIR "/var/tmp/"
#    define IMF_PATH_SEPARATOR "/"
#endif
