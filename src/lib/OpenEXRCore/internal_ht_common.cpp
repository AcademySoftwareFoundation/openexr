/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_ht_common.h"

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_coding.h"
#include "internal_structs.h"

#include <vector>
#include <string>

#include "openexr_compression.h"
#include <cassert>

bool
make_channel_map (
    int channel_count, exr_coding_channel_info_t* channels, std::vector<int>& cs_to_file_ch)
{
    int r_index = -1;
    int g_index = -1;
    int b_index = -1;

    cs_to_file_ch.resize(channel_count);

    for (size_t i = 0; i < channel_count; i++)
    {
        assert (channels[i].data_type == EXR_PIXEL_HALF);
        assert (channels[i].x_samples == 1);
        assert (channels[i].y_samples == 1);

        std::string c_name(channels[i].channel_name);

        if (c_name == "R") { r_index = i; }
        else if (c_name == "G") { g_index = i; }
        else if (c_name == "B") { b_index = i; }
    }

    bool isRGB;

    if (r_index >= 0 && g_index >= 0 && b_index >= 0)
    {
        isRGB = true;

        cs_to_file_ch[0] = r_index;
        cs_to_file_ch[1] = g_index;
        cs_to_file_ch[2] = b_index;

        int cs_i = 3;
        for (size_t i = 0; i < channel_count; i++)
        {
            if (i != r_index && i != g_index && i != b_index)
            {
                cs_to_file_ch[cs_i++] = i;
            }
        }
    }
    else
    {
        isRGB = false;

        for (size_t i = 0; i < channel_count; i++)
        {
            cs_to_file_ch[i] = i;
        }
    }

    return isRGB;
}
