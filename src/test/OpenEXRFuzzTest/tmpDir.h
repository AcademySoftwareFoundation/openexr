//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#if defined(_WIN32) || defined(_WIN64) || defined(__ANDROID__)
    #define IMF_TMP_DIR ""
#else
    #define IMF_TMP_DIR "/var/tmp/"
#endif
