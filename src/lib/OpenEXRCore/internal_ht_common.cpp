/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_ht_common.h"
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <cctype>

bool
make_channel_map (
    int channel_count, exr_coding_channel_info_t* channels, std::vector<CodestreamChannelInfo>& cs_to_file_ch)
{
    int r_index = -1;
    int g_index = -1;
    int b_index = -1;

    cs_to_file_ch.resize(channel_count);

    for (size_t i = 0; i < channel_count; i++)
    {
        std::string c_name(channels[i].channel_name);

        /* heuristics to determine whether RGB channels are present */
        std::transform(c_name.begin(), c_name.end(), c_name.begin(),
                   [](unsigned char c){ return std::tolower(c); });

        if (c_name == "r" || c_name.compare(0, 3, "red") == 0) { r_index = i; }
        else if (c_name == "g" || c_name.compare(0, 5, "green") == 0) { g_index = i; }
        else if (c_name == "b" || c_name.compare(0, 4, "blue")== 0) { b_index = i; }
    }

    bool isRGB = r_index >= 0 && g_index >= 0 && b_index >= 0 &&
                 channels[r_index].data_type == channels[g_index].data_type &&
                 channels[r_index].data_type == channels[b_index].data_type &&
                 channels[r_index].x_samples == channels[g_index].x_samples &&
                 channels[r_index].x_samples == channels[b_index].x_samples &&
                 channels[r_index].y_samples == channels[g_index].y_samples &&
                 channels[r_index].y_samples == channels[b_index].y_samples;

    if (isRGB)
    {
        cs_to_file_ch[0].file_index = r_index;
        cs_to_file_ch[1].file_index = g_index;
        cs_to_file_ch[2].file_index = b_index;

        int avail_cs_i = 3;
        int offset = 0;
        for (size_t file_i = 0; file_i < channel_count; file_i++)
        {
            int cs_i;
            if (file_i == r_index) {
                cs_i = 0;
            } else if (file_i == g_index) {
                cs_i = 1;
            } else if (file_i == b_index) {
                cs_i = 2;
            } else {
                cs_i = avail_cs_i++;
            }

            cs_to_file_ch[cs_i].file_index = file_i;
            cs_to_file_ch[cs_i].raster_line_offset = offset;
            offset += channels[file_i].width * channels[file_i].bytes_per_element;
        }
    }
    else
    {
        int offset = 0;
        for (size_t file_i = 0; file_i < channel_count; file_i++)
        {
            cs_to_file_ch[file_i].file_index = file_i;
            cs_to_file_ch[file_i].raster_line_offset = offset;
            offset += channels[file_i].width * channels[file_i].bytes_per_element;
        }
    }

    return isRGB;
}
