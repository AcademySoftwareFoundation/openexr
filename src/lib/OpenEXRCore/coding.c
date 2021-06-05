/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_coding.h"

#include <string.h>

exr_result_t
internal_coding_fill_channel_info (
    exr_coding_channel_info_t **channels,
    int16_t *num_chans,
    exr_coding_channel_info_t *builtinextras,
    const exr_chunk_block_info_t* cinfo,
    const struct _internal_exr_context* pctxt,
    const struct _internal_exr_part *part)
{
    int chans;
    exr_attr_chlist_t*         chanlist;
    exr_coding_channel_info_t* chanfill;

    chanlist = part->channels->chlist;
    chans    = chanlist->num_channels;
    if (chans <= 5) { chanfill = builtinextras; }
    else
    {
        chanfill = pctxt->alloc_fn (
            (size_t) (chans) * sizeof (exr_coding_channel_info_t));
        if (chanfill == NULL)
            return pctxt->standard_error (pctxt, EXR_ERR_OUT_OF_MEMORY);
        memset (
            chanfill,
            0,
            (size_t) (chans) * sizeof (exr_coding_channel_info_t));
    }

    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        exr_coding_channel_info_t*     decc = (chanfill + c);

        decc->channel_name = curc->name.str;

        if (curc->y_sampling > 1)
        {
            if (cinfo->height == 1)
                decc->height = ((cinfo->start_y % curc->y_sampling) == 0) ? 1
                                                                          : 0;
            else
                decc->height = cinfo->height / curc->y_sampling;
        }
        else
            decc->height = cinfo->height;

        if (curc->x_sampling > 1)
            decc->width = cinfo->width / curc->x_sampling;
        else
            decc->width = cinfo->width;

        decc->x_samples         = curc->x_sampling;
        decc->y_samples         = curc->y_sampling;
        decc->bytes_per_element = (curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4;
        decc->data_type         = (uint16_t) (curc->pixel_type);

        /* initialize these so they don't trip us up during decoding
         * when the user also chooses to skip a channel */
        decc->user_bytes_per_element = decc->bytes_per_element;
        decc->user_data_type         = decc->data_type;
        /* but leave the rest as zero for the user to fill in */
    }

    *channels = chanfill;
    *num_chans = (int16_t) chans;

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_coding_update_channel_info (
    exr_coding_channel_info_t *channels,
    int16_t num_chans,
    const exr_chunk_block_info_t* cinfo,
    const struct _internal_exr_context* pctxt,
    const struct _internal_exr_part *part)
{
    int                        chans;
    exr_attr_chlist_t*         chanlist;

    chanlist    = part->channels->chlist;
    chans       = chanlist->num_channels;

    if (num_chans != chans)
        return pctxt->print_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Mismatch in channel counts: stored %d, incoming %d",
            num_chans,
            chans);

    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        exr_coding_channel_info_t*     ccic = (channels + c);

        ccic->channel_name = curc->name.str;

        if (curc->y_sampling > 1)
        {
            if (cinfo->height == 1)
                ccic->height = ((cinfo->start_y % curc->y_sampling) == 0) ? 1
                                                                          : 0;
            else
                ccic->height = cinfo->height / curc->y_sampling;
        }
        else
            ccic->height = cinfo->height;

        if (curc->x_sampling > 1)
            ccic->width = cinfo->width / curc->x_sampling;
        else
            ccic->width = cinfo->width;
        ccic->x_samples = curc->x_sampling;
        ccic->y_samples = curc->y_sampling;

        ccic->bytes_per_element = (curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4;
        ccic->data_type         = (uint16_t) (curc->pixel_type);
    }

    return EXR_ERR_SUCCESS;
}


