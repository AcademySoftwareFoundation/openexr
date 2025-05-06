/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_ht_common.h"
#include <vector>
#include <string>
#include <string.h>
#include <cassert>
#include <algorithm>
#include <cctype>

const std::string RED_CH_FULLNAME = "red";
const std::string GREEN_CH_FULLNAME = "green";
const std::string BLUE_CH_FULLNAME = "blue";

struct RGBChannelParams
{
    const char* r_suffix;
    const char* g_suffix;
    const char* b_suffix;
    int         r_index;
    int         g_index;
    int         b_index;
    const char* prefix;
    size_t      prefix_len;
};

static inline bool areEqual(const char* a, const char* b) {
#ifdef _MSC_VER
    return _stricmp (a, b) == 0;
#else
    return strcasecmp (a, b) == 0;
#endif
}

bool
make_channel_map (
    int                                 channel_count,
    exr_coding_channel_info_t*          channels,
    std::vector<CodestreamChannelInfo>& cs_to_file_ch)
{
    /** Heuristic detection of RGB channels so that the JPEG 2000 Reversible
      * Color Transform (RCT), a decorrelation transform, can be applied.
      *
      * RGB channels are present if the either (a) the names of the channels in
      * their entirety or (b) following the "." character match one of the
      * triplets defined in {params}. Order of the channels and case of the
      * channel names are ignored.
      *
      * Example 1: "main.b", "main.g", "main.r" match
      * Example 2: "MainR", "MainG", "MainB" do not match
      * Example 3: "R", "B", "B" match
      * Example 4: "red", "green", "blue" match
      */

    RGBChannelParams params[] = {
        {"r", "g", "b", -1, -1, -1, NULL, 0},
        {"red", "green", "blue", -1, -1, -1, NULL, 0},
        {"red", "grn", "blu", -1, -1, -1, NULL, 0}};
    constexpr size_t params_count = sizeof (params) / sizeof (params[0]);

    cs_to_file_ch.resize (channel_count);

    for (size_t i = 0; i < channel_count; i++)
    {
        const char* channel_name = channels[i].channel_name;
        const char* suffix       = strrchr (channel_name, '.');
        const char* prefix       = channel_name;
        size_t      prefix_len   = 0;
        if (suffix) {
            suffix += 1;
            prefix_len = suffix - prefix - 1;
        } else {
            suffix = channel_name;
        }

        for (size_t j = 0; j < params_count; j++)
        {
            if (params[j].prefix != NULL &&
                (params[j].prefix_len != prefix_len ||
                 strncmp (params[j].prefix, prefix, params[j].prefix_len)))
            {
                /* skip to the next potential match if a prefix has already been
                record and does not match the channel prefix */
                continue;
            }

            bool match = false;
            if (areEqual (suffix, params[j].r_suffix) &&
                params[j].r_index < 0)
            {
                params[j].r_index = i;
                match = true;
            }
            else if (
                areEqual (suffix, params[j].g_suffix) &&
                params[j].g_index < 0)
            {
                params[j].g_index = i;
                match = true;
            }
            else if (
                areEqual (suffix, params[j].b_suffix) &&
                params[j].b_index < 0)
            {
                params[j].b_index = i;
                match = true;
            }

            if (match) {
                /* record the prefix if one is not already recorded and move to
                the next channel */
                params[j].prefix = prefix;
                params[j].prefix_len = prefix_len;
                break;
            }
        }
    }

    int  r_index;
    int  g_index;
    int  b_index;
    bool isRGB = false;

    for (size_t j = 0; (!isRGB) && j < params_count; j++)
    {
        r_index = params[j].r_index;
        g_index = params[j].g_index;
        b_index = params[j].b_index;

        isRGB = r_index > -1 && g_index > -1 && b_index > -1 &&
                channels[r_index].data_type == channels[g_index].data_type &&
                channels[r_index].data_type == channels[b_index].data_type &&
                channels[r_index].x_samples == channels[g_index].x_samples &&
                channels[r_index].x_samples == channels[b_index].x_samples &&
                channels[r_index].y_samples == channels[g_index].y_samples &&
                channels[r_index].y_samples == channels[b_index].y_samples;
    }

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
