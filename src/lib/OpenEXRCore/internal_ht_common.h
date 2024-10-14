/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_HT_COMMON_H
#define OPENEXR_PRIVATE_HT_COMMON_H

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_coding.h"
#include "internal_structs.h"

#include <vector>
#include <string>

#include "openexr_compression.h"

bool
make_channel_map (
    int channel_count, exr_coding_channel_info_t* channels, std::vector<int>& cs_to_file_ch);

#endif /* OPENEXR_PRIVATE_HT_COMMON_H */
