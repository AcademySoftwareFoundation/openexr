/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_HT_COMMON_H
#define OPENEXR_PRIVATE_HT_COMMON_H

#include <vector>
#include <stdlib.h>
#include "openexr_coding.h"

struct CodestreamChannelInfo {
    int file_index;
    size_t raster_line_offset;
};

bool
make_channel_map (
    int channel_count, exr_coding_channel_info_t* channels, std::vector<CodestreamChannelInfo>& cs_to_file_ch);

#endif /* OPENEXR_PRIVATE_HT_COMMON_H */
