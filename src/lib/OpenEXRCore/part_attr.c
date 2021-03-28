/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_part.h"

#include "internal_attr.h"
#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
exr_part_get_attribute_count (
    const exr_context_t ctxt, int part_index, int32_t* count)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!count) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
    *count = part->attributes.num_attributes;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_part_get_attribute_by_index (
    const exr_context_t            ctxt,
    int                            part_index,
    enum exr_attr_list_access_mode mode,
    int32_t                        idx,
    const exr_attribute_t**        outattr)
{
    exr_attribute_t** srclist;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!outattr) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (idx < 0 || idx >= part->attributes.num_attributes)
        return pctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE);

    if (mode == EXR_ATTR_LIST_SORTED_ORDER)
        srclist = part->attributes.sorted_entries;
    else
        srclist = part->attributes.entries;

    *outattr = srclist[idx];
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_part_get_attribute_by_name (
    const exr_context_t     ctxt,
    int                     part_index,
    const char*             name,
    const exr_attribute_t** outattr)
{
    exr_attribute_t* tmpptr;
    exr_result_t     rv;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!outattr) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    rv = exr_attr_list_find_by_name (
        (exr_context_t) ctxt,
        (exr_attribute_list_t*) &(part->attributes),
        name,
        &tmpptr);
    if (rv == EXR_ERR_SUCCESS) *outattr = tmpptr;
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_attribute_list (
    const exr_context_t            ctxt,
    int                            part_index,
    enum exr_attr_list_access_mode mode,
    int32_t*                       count,
    const exr_attribute_t**        outlist)
{
    exr_attribute_t** srclist;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!count) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (mode == EXR_ATTR_LIST_SORTED_ORDER)
        srclist = part->attributes.sorted_entries;
    else
        srclist = part->attributes.entries;

    if (outlist && *count >= part->attributes.num_attributes)
        memcpy (
            outlist,
            srclist,
            sizeof (exr_attribute_t*) * part->attributes.num_attributes);
    *count = part->attributes.num_attributes;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_part_attr_declare_by_type (
    exr_context_t     ctxt,
    int               part_index,
    const char*       name,
    const char*       type,
    exr_attribute_t** outattr)
{
    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (pctxt->mode != EXR_CONTEXT_WRITE)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);

    return exr_attr_list_add_by_type (
        ctxt, &(part->attributes), name, type, 0, NULL, outattr);
}

/**************************************/

exr_result_t
exr_part_attr_declare (
    exr_context_t        ctxt,
    int                  part_index,
    const char*          name,
    exr_attribute_type_t type,
    exr_attribute_t**    outattr)
{
    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (pctxt->mode != EXR_CONTEXT_WRITE)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);

    return exr_attr_list_add (
        ctxt, &(part->attributes), name, type, 0, NULL, outattr);
}

/**************************************/

exr_result_t
exr_part_initialize_required_attr (
    exr_context_t           ctxt,
    int                     part_index,
    const exr_attr_box2i_t* displayWindow,
    const exr_attr_box2i_t* dataWindow,
    float                   pixelaspectratio,
    const exr_attr_v2f_t*   screenWindowCenter,
    float                   screenWindowWidth,
    exr_lineorder_t         lineorder,
    exr_compression_t       ctype)
{
    exr_result_t rv;

    rv = exr_part_set_compression (ctxt, part_index, ctype);
    if (rv != EXR_ERR_SUCCESS) return rv;
    rv = exr_part_set_data_window (ctxt, part_index, dataWindow);
    if (rv != EXR_ERR_SUCCESS) return rv;
    rv = exr_part_set_display_window (ctxt, part_index, displayWindow);
    if (rv != EXR_ERR_SUCCESS) return rv;
    rv = exr_part_set_lineorder (ctxt, part_index, lineorder);
    if (rv != EXR_ERR_SUCCESS) return rv;
    rv = exr_part_set_pixel_aspect_ratio (ctxt, part_index, pixelaspectratio);
    if (rv != EXR_ERR_SUCCESS) return rv;
    rv = exr_part_set_screen_window_center (
        ctxt, part_index, screenWindowCenter);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return exr_part_set_screen_window_width (
        ctxt, part_index, screenWindowWidth);
}

/**************************************/

exr_result_t
exr_part_initialize_required_attr_simple (
    exr_context_t     ctxt,
    int               part_index,
    int32_t           width,
    int32_t           height,
    exr_compression_t ctype)
{
    exr_attr_box2i_t dispWindow = { 0, 0, width - 1, height - 1 };
    exr_attr_v2f_t   swc        = { 0.f, 0.f };
    return exr_part_initialize_required_attr (
        ctxt,
        part_index,
        &dispWindow,
        &dispWindow,
        1.f,
        &swc,
        1.f,
        EXR_LINEORDER_INCREASING_Y,
        ctype);
}

/**************************************/

#define REQ_ATTR_GET_IMPL(name, entry, t)                                      \
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);            \
    if (!out)                                                                  \
        return pctxt->print_error (                                            \
            ctxt, EXR_ERR_INVALID_ARGUMENT, "NULL output for '%s'", #name);    \
    if (part->name)                                                            \
    {                                                                          \
        if (part->name->type != t)                                             \
            return pctxt->print_error (                                        \
                ctxt,                                                          \
                EXR_ERR_FILE_BAD_HEADER,                                       \
                "Invalid required attribute type '%s' for '%s'",               \
                part->name->type_name,                                         \
                #name);                                                        \
        *out = part->name->entry;                                              \
        return EXR_ERR_SUCCESS;                                                \
    }                                                                          \
    return EXR_ERR_NO_ATTR_BY_NAME

#define REQ_ATTR_GET_IMPL_DEREF(name, entry, t)                                \
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);            \
    if (!out)                                                                  \
        return pctxt->print_error (                                            \
            ctxt, EXR_ERR_INVALID_ARGUMENT, "NULL output for '%s'", #name);    \
    if (part->name)                                                            \
    {                                                                          \
        if (part->name->type != t)                                             \
            return pctxt->print_error (                                        \
                ctxt,                                                          \
                EXR_ERR_FILE_BAD_HEADER,                                       \
                "Invalid required attribute type '%s' for '%s'",               \
                part->name->type_name,                                         \
                #name);                                                        \
        *out = *(part->name->entry);                                           \
        return EXR_ERR_SUCCESS;                                                \
    }                                                                          \
    return EXR_ERR_NO_ATTR_BY_NAME

#define REQ_ATTR_FIND_CREATE(name, t)                                          \
    exr_attribute_t* attr = NULL;                                              \
    exr_result_t     rv   = EXR_ERR_SUCCESS;                                   \
    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);                  \
    if (pctxt->mode == EXR_CONTEXT_READ)                                       \
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);           \
    if (pctxt->mode == EXR_CONTEXT_WRITING_DATA)                               \
        return pctxt->standard_error (ctxt, EXR_ERR_ALREADY_WROTE_ATTRS);      \
    if (!part->name)                                                           \
    {                                                                          \
        rv = exr_attr_list_add (                                               \
            ctxt, &(part->attributes), #name, t, 0, NULL, &(part->name));      \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        if (part->name->type != t)                                             \
            return pctxt->print_error (                                        \
                ctxt,                                                          \
                EXR_ERR_FILE_BAD_HEADER,                                       \
                "Invalid required attribute type '%s' for '%s'",               \
                part->name->type_name,                                         \
                #name);                                                        \
        attr = part->name;                                                     \
    }

/**************************************/

exr_result_t
exr_part_get_channels (
    const exr_context_t ctxt, int part_index, const exr_attr_chlist_t** out)
{
    REQ_ATTR_GET_IMPL (channels, chlist, EXR_ATTR_CHLIST);
}

/**************************************/

exr_result_t
exr_part_add_channel (
    exr_context_t    ctxt,
    int              part_index,
    const char*      name,
    exr_pixel_type_t ptype,
    uint8_t          islinear,
    int32_t          xsamp,
    int32_t          ysamp)
{
    REQ_ATTR_FIND_CREATE (channels, EXR_ATTR_CHLIST);
    if (rv == EXR_ERR_SUCCESS)
    {
        rv = exr_attr_chlist_add (
            ctxt, attr->chlist, name, ptype, islinear, xsamp, ysamp);
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_set_channels (
    exr_context_t ctxt, int part_index, const exr_attr_chlist_t* channels)
{
    REQ_ATTR_FIND_CREATE (channels, EXR_ATTR_CHLIST);
    if (rv == EXR_ERR_SUCCESS)
    {
        exr_attr_chlist_t clist;
        int               numchans;

        if (!channels)
            return pctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "No channels provided for channel list");

        numchans = channels->num_channels;
        rv       = exr_attr_chlist_init (ctxt, &clist, numchans);
        if (rv != EXR_ERR_SUCCESS) return rv;

        for (int c = 0; c < numchans; ++c)
        {
            const exr_attr_chlist_entry_t* cur = channels->entries + c;

            rv = exr_attr_chlist_add_with_length (
                ctxt,
                &clist,
                cur->name.str,
                cur->name.length,
                cur->pixel_type,
                cur->p_linear,
                cur->x_sampling,
                cur->y_sampling);
            if (rv != EXR_ERR_SUCCESS)
            {
                exr_attr_chlist_destroy (ctxt, &clist);
                return rv;
            }
        }

        exr_attr_chlist_destroy (ctxt, attr->chlist);
        *(attr->chlist) = clist;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_compression (
    exr_context_t ctxt, int part_index, exr_compression_t* out)
{
    REQ_ATTR_GET_IMPL (compression, uc, EXR_ATTR_COMPRESSION);
}

/**************************************/

exr_result_t
exr_part_set_compression (
    exr_context_t ctxt, int part_index, exr_compression_t ctype)
{
    REQ_ATTR_FIND_CREATE (compression, EXR_ATTR_COMPRESSION);
    if (rv == EXR_ERR_SUCCESS)
    {
        attr->uc        = (uint8_t) ctype;
        part->comp_type = ctype;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_data_window (
    exr_context_t ctxt, int part_index, exr_attr_box2i_t* out)
{
    REQ_ATTR_GET_IMPL_DEREF (dataWindow, box2i, EXR_ATTR_BOX2I);
}

/**************************************/

exr_result_t
exr_part_set_data_window (
    exr_context_t ctxt, int part_index, const exr_attr_box2i_t* dw)
{
    REQ_ATTR_FIND_CREATE (dataWindow, EXR_ATTR_BOX2I);
    if (rv != EXR_ERR_SUCCESS) return rv;
    if (!dw)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Missing value for data window assignment");

    *(attr->box2i)    = *dw;
    part->data_window = *dw;
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_display_window (
    exr_context_t ctxt, int part_index, exr_attr_box2i_t* out)
{
    REQ_ATTR_GET_IMPL_DEREF (displayWindow, box2i, EXR_ATTR_BOX2I);
}

/**************************************/

exr_result_t
exr_part_set_display_window (
    exr_context_t ctxt, int part_index, const exr_attr_box2i_t* dw)
{
    REQ_ATTR_FIND_CREATE (displayWindow, EXR_ATTR_BOX2I);
    if (rv != EXR_ERR_SUCCESS) return rv;
    if (!dw)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Missing value for data window assignment");

    *(attr->box2i)       = *dw;
    part->display_window = *dw;

    return rv;
}

/**************************************/

exr_result_t
exr_part_get_lineorder (
    exr_context_t ctxt, int part_index, exr_lineorder_t* out)
{
    REQ_ATTR_GET_IMPL (lineOrder, uc, EXR_ATTR_LINEORDER);
}

/**************************************/

exr_result_t
exr_part_set_lineorder (exr_context_t ctxt, int part_index, exr_lineorder_t lo)
{
    REQ_ATTR_FIND_CREATE (lineOrder, EXR_ATTR_LINEORDER);
    if (rv == EXR_ERR_SUCCESS)
    {
        attr->uc        = (uint8_t) lo;
        part->lineorder = lo;
    }

    return rv;
}

/**************************************/

exr_result_t
exr_part_get_pixel_aspect_ratio (exr_context_t ctxt, int part_index, float* out)
{
    REQ_ATTR_GET_IMPL (pixelAspectRatio, f, EXR_ATTR_FLOAT);
}

/**************************************/

exr_result_t
exr_part_set_pixel_aspect_ratio (exr_context_t ctxt, int part_index, float par)
{
    REQ_ATTR_FIND_CREATE (pixelAspectRatio, EXR_ATTR_FLOAT);
    if (rv == EXR_ERR_SUCCESS) attr->f = par;
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_screen_window_center (
    exr_context_t ctxt, int part_index, exr_attr_v2f_t* out)
{
    REQ_ATTR_GET_IMPL_DEREF (screenWindowCenter, v2f, EXR_ATTR_V2F);
}

/**************************************/

exr_result_t
exr_part_set_screen_window_center (
    exr_context_t ctxt, int part_index, const exr_attr_v2f_t* swc)
{
    REQ_ATTR_FIND_CREATE (screenWindowCenter, EXR_ATTR_V2F);
    if (rv != EXR_ERR_SUCCESS) return rv;
    if (!swc)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Missing value for data window assignment");

    attr->v2f->x = swc->x;
    attr->v2f->y = swc->y;
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_screen_window_width (
    exr_context_t ctxt, int part_index, float* out)
{
    REQ_ATTR_GET_IMPL (screenWindowWidth, f, EXR_ATTR_FLOAT);
}

/**************************************/

exr_result_t
exr_part_set_screen_window_width (exr_context_t ctxt, int part_index, float ssw)
{
    REQ_ATTR_FIND_CREATE (screenWindowWidth, EXR_ATTR_FLOAT);
    if (rv == EXR_ERR_SUCCESS) attr->f = ssw;
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_tile_descriptor (
    const exr_context_t    ctxt,
    int                    part_index,
    uint32_t*              xsize,
    uint32_t*              ysize,
    exr_tile_level_mode_t* level,
    exr_tile_round_mode_t* round)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->tiles)
    {
        const exr_attr_tiledesc_t* out = part->tiles->tiledesc;

        if (part->tiles->type != EXR_ATTR_STRING)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_FILE_BAD_HEADER,
                "Invalid required attribute type '%s' for 'tiles'",
                part->tiles->type_name);

        if (xsize) *xsize = out->x_size;
        if (ysize) *ysize = out->y_size;
        if (level) *level = EXR_GET_TILE_LEVEL_MODE (*out);
        if (round) *round = EXR_GET_TILE_ROUND_MODE (*out);
        return EXR_ERR_SUCCESS;
    }
    return EXR_ERR_NO_ATTR_BY_NAME;
}

/**************************************/

exr_result_t
exr_part_set_tile_descriptor (
    exr_context_t         ctxt,
    int                   part_index,
    uint32_t              x_size,
    uint32_t              y_size,
    exr_tile_level_mode_t level_mode,
    exr_tile_round_mode_t round_mode)
{
    REQ_ATTR_FIND_CREATE (tiles, EXR_ATTR_TILEDESC);
    if (rv == EXR_ERR_SUCCESS)
    {
        attr->tiledesc->x_size = x_size;
        attr->tiledesc->y_size = y_size;
        attr->tiledesc->level_and_round =
            EXR_PACK_TILE_LEVEL_ROUND (level_mode, round_mode);
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_name (exr_context_t ctxt, int part_index, const char** out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!out)
        return pctxt->print_error (
            ctxt, EXR_ERR_INVALID_ARGUMENT, "NULL output for 'name'");

    if (part->name)
    {
        if (part->name->type != EXR_ATTR_STRING)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_FILE_BAD_HEADER,
                "Invalid required attribute type '%s' for 'name'",
                part->name->type_name);
        *out = part->name->string->str;
        return EXR_ERR_SUCCESS;
    }
    return EXR_ERR_NO_ATTR_BY_NAME;
}

exr_result_t
exr_part_set_name (exr_context_t ctxt, int part_index, const char* val)
{
    size_t           bytes;
    REQ_ATTR_FIND_CREATE (name, EXR_ATTR_STRING);

    if (!val || val[0] == '\0')
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid string passed trying to set 'name'");

    bytes = strlen (val);

    if (bytes >= (size_t) INT32_MAX)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "String too large to store (%lu bytes) into 'name'",
            bytes);

    if (rv == EXR_ERR_SUCCESS)
    {
        if (attr->string->length == (int32_t) bytes &&
            attr->string->alloc_size > 0)
        {
            memcpy ((void*) attr->string->str, val, bytes);
        }
        else if (pctxt->mode != EXR_CONTEXT_WRITE)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_MODIFY_SIZE_CHANGE,
                "Existing string 'name' has length %d, requested %d, unable to change",
                attr->string->length,
                (int32_t) bytes);
        }
        else
        {
            rv = exr_attr_string_set_with_length (
                ctxt, attr->string, val, (int32_t) bytes);
        }
    }

    return rv;
}

/**************************************/

exr_result_t
exr_part_get_version (exr_context_t ctxt, int part_index, int32_t* out)
{
    REQ_ATTR_GET_IMPL (version, i, EXR_ATTR_INT);
}

/**************************************/

exr_result_t
exr_part_set_version (exr_context_t ctxt, int part_index, int32_t val)
{
    REQ_ATTR_FIND_CREATE (version, EXR_ATTR_INT);
    if (rv == EXR_ERR_SUCCESS) { attr->i = val; }
    return rv;
}

/**************************************/

exr_result_t
exr_part_set_chunk_count (exr_context_t ctxt, int part_index, int32_t val)
{
    REQ_ATTR_FIND_CREATE (chunkCount, EXR_ATTR_INT);
    if (rv == EXR_ERR_SUCCESS)
    {
        attr->i           = val;
        part->chunk_count = val;
    }
    return rv;
}

/**************************************/

#define ATTR_FIND_ATTR(t, entry)                                               \
    exr_attribute_t* attr;                                                     \
    exr_result_t     rv;                                                       \
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);            \
    if (!name || name[0] == '\0')                                              \
        return pctxt->report_error (                                           \
            ctxt,                                                              \
            EXR_ERR_INVALID_ARGUMENT,                                          \
            "Invalid name for " #entry " attribute query");                    \
    rv = exr_attr_list_find_by_name (                                          \
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);       \
    if (rv != EXR_ERR_SUCCESS) return rv;                                      \
    if (attr->type != t)                                                       \
    return pctxt->print_error (                                                \
        ctxt,                                                                  \
        EXR_ERR_ATTR_TYPE_MISMATCH,                                            \
        "'%s' requested type '" #entry                                         \
        "', but stored attributes is type '%s'",                               \
        name,                                                                  \
        attr->type_name)

#define ATTR_GET_IMPL(t, entry)                                                \
    ATTR_FIND_ATTR (t, entry);                                                 \
    if (!out)                                                                  \
        return pctxt->print_error (                                            \
            ctxt, EXR_ERR_INVALID_ARGUMENT, "NULL output for '%s'", name);     \
    *out = attr->entry;                                                        \
    return rv

#define ATTR_GET_IMPL_DEREF(t, entry)                                          \
    ATTR_FIND_ATTR (t, entry);                                                 \
    if (!out)                                                                  \
        return pctxt->print_error (                                            \
            ctxt, EXR_ERR_INVALID_ARGUMENT, "NULL output for '%s'", name);     \
    *out = *(attr->entry);                                                     \
    return rv

#define ATTR_FIND_CREATE(t, entry)                                             \
    exr_attribute_t* attr = NULL;                                              \
    exr_result_t     rv   = EXR_ERR_SUCCESS;                                   \
    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);                  \
    if (pctxt->mode == EXR_CONTEXT_READ)                                       \
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);           \
    if (pctxt->mode == EXR_CONTEXT_WRITING_DATA)                               \
        return pctxt->standard_error (ctxt, EXR_ERR_ALREADY_WROTE_ATTRS);      \
    rv = exr_attr_list_find_by_name (                                          \
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);       \
    if (rv == EXR_ERR_NO_ATTR_BY_NAME)                                         \
    {                                                                          \
        if (pctxt->mode != EXR_CONTEXT_WRITE) return rv;                       \
                                                                               \
        rv = exr_attr_list_add (                                               \
            ctxt, &(part->attributes), name, t, 0, NULL, &(attr));             \
    }                                                                          \
    else if (rv == EXR_ERR_SUCCESS)                                            \
    {                                                                          \
        if (attr->type != t)                                                   \
            return pctxt->print_error (                                        \
                ctxt,                                                          \
                EXR_ERR_ATTR_TYPE_MISMATCH,                                    \
                "'%s' requested type '" #entry                                 \
                "', but stored attributes is type '%s'",                       \
                name,                                                          \
                attr->type_name);                                              \
    }                                                                          \
    else                                                                       \
        return rv

#define ATTR_SET_IMPL(t, entry)                                                \
    ATTR_FIND_CREATE (t, entry);                                               \
    attr->entry = val;                                                         \
    return rv

#define ATTR_SET_IMPL_DEREF(t, entry)                                          \
    ATTR_FIND_CREATE (t, entry);                                               \
    if (!val)                                                                  \
        return pctxt->print_error (                                            \
            ctxt,                                                              \
            EXR_ERR_INVALID_ARGUMENT,                                          \
            "No input value for setting '%s', type '%s'",                      \
            name,                                                              \
            #entry);                                                           \
    *(attr->entry) = *val;                                                     \
    return rv

/**************************************/

exr_result_t
exr_part_attr_get_box2i (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_box2i_t*   out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_BOX2I, box2i);
}

exr_result_t
exr_part_attr_set_box2i (
    exr_context_t           ctxt,
    int                     part_index,
    const char*             name,
    const exr_attr_box2i_t* val)
{
    if (name && 0 == strcmp (name, EXR_REQ_DATA_STR))
        return exr_part_set_data_window (ctxt, part_index, val);
    if (name && 0 == strcmp (name, EXR_REQ_DISP_STR))
        return exr_part_set_display_window (ctxt, part_index, val);
    ATTR_SET_IMPL_DEREF (EXR_ATTR_BOX2I, box2i);
}

/**************************************/

exr_result_t
exr_part_attr_get_box2f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_box2f_t*   out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_BOX2F, box2f);
}

exr_result_t
exr_part_attr_set_box2f (
    exr_context_t           ctxt,
    int                     part_index,
    const char*             name,
    const exr_attr_box2f_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_BOX2F, box2f);
}

/**************************************/

exr_result_t
exr_part_attr_get_channels (
    const exr_context_t       ctxt,
    int                       part_index,
    const char*               name,
    const exr_attr_chlist_t** out)
{
    ATTR_GET_IMPL (EXR_ATTR_CHLIST, chlist);
}

exr_result_t
exr_part_attr_set_channels (
    exr_context_t            ctxt,
    int                      part_index,
    const char*              name,
    const exr_attr_chlist_t* channels)
{
    exr_attribute_t* attr = NULL;
    exr_result_t     rv   = EXR_ERR_SUCCESS;

    if (name && 0 == strcmp (name, EXR_REQ_CHANNELS_STR))
        return exr_part_set_channels (ctxt, part_index, channels);

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    /* do not support updating channels during update operation... */
    if (pctxt->mode != EXR_CONTEXT_WRITE)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);

    if (!channels)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "No input values for setting '%s', type 'chlist'",
            name);

    rv = exr_attr_list_find_by_name (
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);

    if (rv == EXR_ERR_NO_ATTR_BY_NAME)
    {
        rv = exr_attr_list_add (
            ctxt, &(part->attributes), name, EXR_ATTR_CHLIST, 0, NULL, &(attr));
    }

    if (rv == EXR_ERR_SUCCESS)
    {
        exr_attr_chlist_t clist;
        int               numchans;

        if (!channels)
            return pctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "No channels provided for channel list");

        numchans = channels->num_channels;
        rv       = exr_attr_chlist_init (ctxt, &clist, numchans);
        if (rv != EXR_ERR_SUCCESS) return rv;

        for (int c = 0; c < numchans; ++c)
        {
            const exr_attr_chlist_entry_t* cur = channels->entries + c;

            rv = exr_attr_chlist_add_with_length (
                ctxt,
                &clist,
                cur->name.str,
                cur->name.length,
                cur->pixel_type,
                cur->p_linear,
                cur->x_sampling,
                cur->y_sampling);
            if (rv != EXR_ERR_SUCCESS)
            {
                exr_attr_chlist_destroy (ctxt, &clist);
                return rv;
            }
        }

        exr_attr_chlist_destroy (ctxt, attr->chlist);
        *(attr->chlist) = clist;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_attr_get_chromaticities (
    const exr_context_t        ctxt,
    int                        part_index,
    const char*                name,
    exr_attr_chromaticities_t* out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_CHROMATICITIES, chromaticities);
}

exr_result_t
exr_part_attr_set_chromaticities (
    exr_context_t                    ctxt,
    int                              part_index,
    const char*                      name,
    const exr_attr_chromaticities_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_CHROMATICITIES, chromaticities);
}

/**************************************/

exr_result_t
exr_part_attr_get_compression (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_compression_t*  out)
{
    ATTR_GET_IMPL (EXR_ATTR_COMPRESSION, uc);
}

exr_result_t
exr_part_attr_set_compression (
    exr_context_t ctxt, int part_index, const char* name, exr_compression_t val)
{
    if (name && 0 == strcmp (name, EXR_REQ_COMP_STR))
        return exr_part_set_compression (ctxt, part_index, val);

    ATTR_SET_IMPL (EXR_ATTR_COMPRESSION, uc);
}

/**************************************/

exr_result_t
exr_part_attr_get_double (
    const exr_context_t ctxt, int part_index, const char* name, double* out)
{
    ATTR_GET_IMPL (EXR_ATTR_DOUBLE, d);
}

exr_result_t
exr_part_attr_set_double (
    exr_context_t ctxt, int part_index, const char* name, double val)
{
    ATTR_SET_IMPL (EXR_ATTR_DOUBLE, d);
}

/**************************************/

exr_result_t
exr_part_attr_get_envmap (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_envmap_t*       out)
{
    ATTR_GET_IMPL (EXR_ATTR_ENVMAP, uc);
}

exr_result_t
exr_part_attr_set_envmap (
    exr_context_t ctxt, int part_index, const char* name, exr_envmap_t val)
{
    ATTR_SET_IMPL (EXR_ATTR_ENVMAP, uc);
}

/**************************************/

exr_result_t
exr_part_attr_get_float (
    const exr_context_t ctxt, int part_index, const char* name, float* out)
{
    ATTR_GET_IMPL (EXR_ATTR_FLOAT, f);
}

exr_result_t
exr_part_attr_set_float (
    exr_context_t ctxt, int part_index, const char* name, float val)
{
    if (name && 0 == strcmp (name, EXR_REQ_PAR_STR))
        return exr_part_set_pixel_aspect_ratio (ctxt, part_index, val);
    if (name && 0 == strcmp (name, EXR_REQ_SCR_WW_STR))
        return exr_part_set_screen_window_width (ctxt, part_index, val);

    ATTR_SET_IMPL (EXR_ATTR_FLOAT, f);
}

exr_result_t
exr_part_attr_get_float_vector (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    int32_t*            sz,
    const float**       out)
{
    ATTR_FIND_ATTR (EXR_ATTR_FLOAT_VECTOR, floatvector);
    if (sz) *sz = attr->floatvector->length;
    if (out) *out = attr->floatvector->arr;
    return rv;
}

exr_result_t
exr_part_attr_set_float_vector (
    exr_context_t ctxt,
    int           part_index,
    const char*   name,
    int32_t       sz,
    const float*  val)
{
    exr_attribute_t* attr  = NULL;
    exr_result_t     rv    = EXR_ERR_SUCCESS;
    size_t           bytes = (size_t) sz * sizeof (float);

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (pctxt->mode == EXR_CONTEXT_READ)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);
    if (pctxt->mode == EXR_CONTEXT_WRITING_DATA)
        return pctxt->standard_error (ctxt, EXR_ERR_ALREADY_WROTE_ATTRS);

    if (sz < 0 || bytes > (size_t) INT32_MAX)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid size (%d) for float vector '%s'",
            sz,
            name);

    if (!val)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "No input values for setting '%s', type 'floatvector'",
            name);

    rv = exr_attr_list_find_by_name (
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);

    if (rv == EXR_ERR_NO_ATTR_BY_NAME)
    {
        if (pctxt->mode != EXR_CONTEXT_WRITE) return rv;

        rv = exr_attr_list_add (
            ctxt,
            &(part->attributes),
            name,
            EXR_ATTR_FLOAT_VECTOR,
            0,
            NULL,
            &(attr));
        if (rv == EXR_ERR_SUCCESS)
            rv =
                exr_attr_float_vector_create (ctxt, attr->floatvector, val, sz);
    }
    else if (rv == EXR_ERR_SUCCESS)
    {
        if (attr->type != EXR_ATTR_FLOAT_VECTOR)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_ATTR_TYPE_MISMATCH,
                "'%s' requested type 'floatvector', but attribute is type '%s'",
                name,
                attr->type_name);
        if (attr->floatvector->length == sz &&
            attr->floatvector->alloc_size > 0)
        {
            memcpy ((void*) attr->floatvector->arr, val, bytes);
        }
        else if (pctxt->mode != EXR_CONTEXT_WRITE)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_MODIFY_SIZE_CHANGE,
                "Existing float vector '%s' has %d, requested %d, unable to change",
                name,
                attr->floatvector->length,
                sz);
        }
        else
        {
            exr_attr_float_vector_destroy (ctxt, attr->floatvector);
            rv =
                exr_attr_float_vector_create (ctxt, attr->floatvector, val, sz);
        }
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_attr_get_int (
    const exr_context_t ctxt, int part_index, const char* name, int32_t* out)
{
    ATTR_GET_IMPL (EXR_ATTR_INT, i);
}

exr_result_t
exr_part_attr_set_int (
    exr_context_t ctxt, int part_index, const char* name, int32_t val)
{
    if (name && !strcmp (name, EXR_REQ_VERSION_STR))
        return exr_part_set_version (ctxt, part_index, val);
    if (name && !strcmp (name, EXR_REQ_CHUNK_COUNT_STR))
        return exr_part_set_chunk_count (ctxt, part_index, val);

    ATTR_SET_IMPL (EXR_ATTR_INT, i);
}

/**************************************/

exr_result_t
exr_part_attr_get_keycode (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_keycode_t* out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_KEYCODE, keycode);
}

exr_result_t
exr_part_attr_set_keycode (
    exr_context_t             ctxt,
    int                       part_index,
    const char*               name,
    const exr_attr_keycode_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_KEYCODE, keycode);
}

/**************************************/

exr_result_t
exr_part_attr_get_lineorder (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_lineorder_t*    out)
{
    ATTR_GET_IMPL (EXR_ATTR_LINEORDER, uc);
}

exr_result_t
exr_part_attr_set_lineorder (
    exr_context_t ctxt, int part_index, const char* name, exr_lineorder_t val)
{
    if (name && 0 == strcmp (name, EXR_REQ_LO_STR))
        return exr_part_set_lineorder (ctxt, part_index, val);

    ATTR_SET_IMPL (EXR_ATTR_LINEORDER, uc);
}

/**************************************/

exr_result_t
exr_part_attr_get_m33f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m33f_t*    out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_M33F, m33f);
}

exr_result_t
exr_part_attr_set_m33f (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m33f_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_M33F, m33f);
}

/**************************************/

exr_result_t
exr_part_attr_get_m33d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m33d_t*    out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_M33D, m33d);
}

exr_result_t
exr_part_attr_set_m33d (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m33d_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_M33D, m33d);
}

/**************************************/

exr_result_t
exr_part_attr_get_m44f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m44f_t*    out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_M44F, m44f);
}

exr_result_t
exr_part_attr_set_m44f (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m44f_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_M44F, m44f);
}

/**************************************/

exr_result_t
exr_part_attr_get_m44d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m44d_t*    out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_M44D, m44d);
}

exr_result_t
exr_part_attr_set_m44d (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m44d_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_M44D, m44d);
}

/**************************************/

exr_result_t
exr_part_attr_get_preview (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_preview_t* out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_PREVIEW, preview);
}

exr_result_t
exr_part_attr_set_preview (
    exr_context_t             ctxt,
    int                       part_index,
    const char*               name,
    const exr_attr_preview_t* val)
{
    exr_attribute_t* attr = NULL;
    exr_result_t     rv   = EXR_ERR_SUCCESS;

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (pctxt->mode == EXR_CONTEXT_READ)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);
    if (pctxt->mode == EXR_CONTEXT_WRITING_DATA)
        return pctxt->standard_error (ctxt, EXR_ERR_ALREADY_WROTE_ATTRS);

    rv = exr_attr_list_find_by_name (
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);

    if (!val)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "No input value for setting '%s', type 'preview'",
            name);

    if (rv == EXR_ERR_NO_ATTR_BY_NAME)
    {
        if (pctxt->mode != EXR_CONTEXT_WRITE) return rv;

        rv = exr_attr_list_add (
            ctxt,
            &(part->attributes),
            name,
            EXR_ATTR_PREVIEW,
            0,
            NULL,
            &(attr));
        if (rv == EXR_ERR_SUCCESS)
            rv = exr_attr_preview_create (
                ctxt, attr->preview, val->width, val->height, val->rgba);
    }
    else if (rv == EXR_ERR_SUCCESS)
    {
        if (attr->type != EXR_ATTR_PREVIEW)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_ATTR_TYPE_MISMATCH,
                "'%s' requested type 'preview', but attribute is type '%s'",
                name,
                attr->type_name);

        if (attr->preview->width == val->width &&
            attr->preview->height == val->height &&
            attr->preview->alloc_size > 0)
        {
            size_t copybytes = val->width * val->height * 4;
            memcpy ((void*) attr->preview->rgba, val->rgba, copybytes);
        }
        else if (pctxt->mode != EXR_CONTEXT_WRITE)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_MODIFY_SIZE_CHANGE,
                "Existing preview '%s' is %u x %u, requested is %u x %u, unable to change",
                name,
                attr->preview->width,
                attr->preview->height,
                val->width,
                val->height);
        }
        else
        {
            exr_attr_preview_destroy (ctxt, attr->preview);
            rv = exr_attr_preview_create (
                ctxt, attr->preview, val->width, val->height, val->rgba);
        }
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_attr_get_rational (
    const exr_context_t  ctxt,
    int                  part_index,
    const char*          name,
    exr_attr_rational_t* out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_RATIONAL, rational);
}

exr_result_t
exr_part_attr_set_rational (
    exr_context_t              ctxt,
    int                        part_index,
    const char*                name,
    const exr_attr_rational_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_RATIONAL, rational);
}

/**************************************/

exr_result_t
exr_part_attr_get_string (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    int32_t*            length,
    const char**        out)
{
    ATTR_FIND_ATTR (EXR_ATTR_STRING, string);
    if (length) *length = attr->string->length;
    if (out) *out = attr->string->str;
    return rv;
}

exr_result_t
exr_part_attr_set_string (
    exr_context_t ctxt, int part_index, const char* name, const char* val)
{
    size_t           bytes;
    exr_attribute_t* attr = NULL;
    exr_result_t     rv   = EXR_ERR_SUCCESS;

    if (name && !strcmp (name, EXR_REQ_NAME_STR))
        return exr_part_set_name (ctxt, part_index, name);

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (name && !strcmp (name, EXR_REQ_TYPE_STR))
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Part type attribute must be implicitly only when adding a part");

    if (pctxt->mode == EXR_CONTEXT_READ)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);
    if (pctxt->mode == EXR_CONTEXT_WRITING_DATA)
        return pctxt->standard_error (ctxt, EXR_ERR_ALREADY_WROTE_ATTRS);

    rv = exr_attr_list_find_by_name (
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);

    if (!val || val[0] == '\0')
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid string passed trying to set '%s'",
            name);

    bytes = strlen (val);

    if (bytes > (size_t) INT32_MAX)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "String too large to store (%lu bytes) into '%s'",
            bytes,
            name);

    if (rv == EXR_ERR_NO_ATTR_BY_NAME)
    {
        if (pctxt->mode != EXR_CONTEXT_WRITE) return rv;

        rv = exr_attr_list_add (
            ctxt, &(part->attributes), name, EXR_ATTR_STRING, 0, NULL, &(attr));
        if (rv == EXR_ERR_SUCCESS)
            rv = exr_attr_string_create_with_length (
                ctxt, attr->string, val, (int32_t) bytes);
    }
    else if (rv == EXR_ERR_SUCCESS)
    {
        if (attr->type != EXR_ATTR_STRING)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_ATTR_TYPE_MISMATCH,
                "'%s' requested type 'string', but attribute is type '%s'",
                name,
                attr->type_name);
        if (attr->string->length == (int32_t) bytes &&
            attr->string->alloc_size > 0)
        {
            memcpy ((void*) attr->string->str, val, bytes);
        }
        else if (pctxt->mode != EXR_CONTEXT_WRITE)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_MODIFY_SIZE_CHANGE,
                "Existing string '%s' has length %d, requested %d, unable to change",
                name,
                attr->string->length,
                (int32_t) bytes);
        }
        else
        {
            rv = exr_attr_string_set_with_length (
                ctxt, attr->string, val, (int32_t) bytes);
        }
    }
    return rv;
}

exr_result_t
exr_part_attr_get_string_vector (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    int32_t*            size,
    const char**        out)
{
    ATTR_FIND_ATTR (EXR_ATTR_STRING_VECTOR, stringvector);
    if (!size)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "size parameter required to query stringvector");
    if (out)
    {
        if (*size < attr->stringvector->n_strings)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "'%s' array buffer too small (%d) to hold string values (%d)",
                name,
                attr->type_name);
        for (int32_t i = 0; i < attr->stringvector->n_strings; ++i)
            out[i] = attr->stringvector->strings[i].str;
    }
    *size = attr->stringvector->n_strings;
    return rv;
}

exr_result_t
exr_part_attr_set_string_vector (
    exr_context_t ctxt,
    int           part_index,
    const char*   name,
    int32_t       size,
    const char**  val)
{
    exr_attribute_t* attr = NULL;
    exr_result_t     rv   = EXR_ERR_SUCCESS;

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (pctxt->mode == EXR_CONTEXT_READ)
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);
    if (pctxt->mode == EXR_CONTEXT_WRITING_DATA)
        return pctxt->standard_error (ctxt, EXR_ERR_ALREADY_WROTE_ATTRS);

    if (size < 0)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid size (%d) for string vector '%s'",
            size,
            name);

    if (!val)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "No input string values for setting '%s', type 'stringvector'",
            name);

    rv = exr_attr_list_find_by_name (
        ctxt, (exr_attribute_list_t*) &(part->attributes), name, &attr);

    if (rv == EXR_ERR_NO_ATTR_BY_NAME)
    {
        if (pctxt->mode != EXR_CONTEXT_WRITE) return rv;

        rv = exr_attr_list_add (
            ctxt,
            &(part->attributes),
            name,
            EXR_ATTR_STRING_VECTOR,
            0,
            NULL,
            &(attr));
        if (rv == EXR_ERR_SUCCESS)
            rv = exr_attr_string_vector_init (ctxt, attr->stringvector, size);
        for (int32_t i = 0; rv == EXR_ERR_SUCCESS && i < size; ++i)
            rv = exr_attr_string_vector_set_entry (
                ctxt, attr->stringvector, i, val[i]);
    }
    else if (rv == EXR_ERR_SUCCESS)
    {
        if (attr->type != EXR_ATTR_STRING_VECTOR)
            return pctxt->print_error (
                ctxt,
                EXR_ERR_ATTR_TYPE_MISMATCH,
                "'%s' requested type 'stringvector', but attribute is type '%s'",
                name,
                attr->type_name);
        if (attr->stringvector->n_strings == size &&
            attr->stringvector->alloc_size > 0)
        {
            if (pctxt->mode != EXR_CONTEXT_WRITE)
            {
                for (int32_t i = 0; rv == EXR_ERR_SUCCESS && i < size; ++i)
                {
                    size_t curlen;
                    if (!val[i])
                        return pctxt->print_error (
                            ctxt,
                            EXR_ERR_INVALID_ARGUMENT,
                            "'%s' received NULL string in string vector",
                            name);

                    curlen = strlen (val[i]);
                    if (curlen !=
                        (size_t) attr->stringvector->strings[i].length)
                        return pctxt->print_error (
                            ctxt,
                            EXR_ERR_INVALID_ARGUMENT,
                            "'%s' string %d in string vector is different size (old %d new %d), unable to update",
                            name,
                            i,
                            attr->stringvector->strings[i].length,
                            (int32_t) curlen);

                    rv = exr_attr_string_vector_set_entry_with_length (
                        ctxt, attr->stringvector, i, val[i], (int32_t) curlen);
                }
            }
            else
            {
                for (int32_t i = 0; rv == EXR_ERR_SUCCESS && i < size; ++i)
                    rv = exr_attr_string_vector_set_entry (
                        ctxt, attr->stringvector, i, val[i]);
            }
        }
        else if (pctxt->mode != EXR_CONTEXT_WRITE)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_MODIFY_SIZE_CHANGE,
                "Existing string vector '%s' has %d strings, but given %d, unable to change",
                name,
                attr->stringvector->n_strings,
                size);
        }
        else
        {
            for (int32_t i = 0; rv == EXR_ERR_SUCCESS && i < size; ++i)
                rv = exr_attr_string_vector_set_entry (
                    ctxt, attr->stringvector, i, val[i]);
        }
    }
    return rv;
}

/**************************************/

exr_result_t
exr_part_attr_get_tiledesc (
    const exr_context_t  ctxt,
    int                  part_index,
    const char*          name,
    exr_attr_tiledesc_t* out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_TILEDESC, tiledesc);
}

exr_result_t
exr_part_attr_set_tiledesc (
    exr_context_t              ctxt,
    int                        part_index,
    const char*                name,
    const exr_attr_tiledesc_t* val)
{
    if (name && 0 == strcmp (name, EXR_REQ_TILES_STR))
    {
        if (!val) return EXR_ERR_INVALID_ARGUMENT;
        return exr_part_set_tile_descriptor (
            ctxt,
            part_index,
            val->x_size,
            val->y_size,
            EXR_GET_TILE_LEVEL_MODE (*val),
            EXR_GET_TILE_ROUND_MODE (*val));
    }

    ATTR_SET_IMPL_DEREF (EXR_ATTR_TILEDESC, tiledesc);
}

/**************************************/

exr_result_t
exr_part_attr_get_timecode (
    const exr_context_t  ctxt,
    int                  part_index,
    const char*          name,
    exr_attr_timecode_t* out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_TIMECODE, timecode);
}

exr_result_t
exr_part_attr_set_timecode (
    exr_context_t              ctxt,
    int                        part_index,
    const char*                name,
    const exr_attr_timecode_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_TIMECODE, timecode);
}

/**************************************/

exr_result_t
exr_part_attr_get_v2i (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v2i_t*     out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_V2I, v2i);
}

exr_result_t
exr_part_attr_set_v2i (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v2i_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_V2I, v2i);
}

/**************************************/

exr_result_t
exr_part_attr_get_v2f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v2f_t*     out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_V2F, v2f);
}

exr_result_t
exr_part_attr_set_v2f (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v2f_t* val)
{
    if (name && 0 == strcmp (name, EXR_REQ_SCR_WC_STR))
        return exr_part_set_screen_window_center (ctxt, part_index, val);

    ATTR_SET_IMPL_DEREF (EXR_ATTR_V2F, v2f);
}

/**************************************/

exr_result_t
exr_part_attr_get_v2d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v2d_t*     out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_V2D, v2d);
}

exr_result_t
exr_part_attr_set_v2d (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v2d_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_V2D, v2d);
}

/**************************************/

exr_result_t
exr_part_attr_get_v3i (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v3i_t*     out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_V3I, v3i);
}

exr_result_t
exr_part_attr_set_v3i (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v3i_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_V3I, v3i);
}

/**************************************/

exr_result_t
exr_part_attr_get_v3f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v3f_t*     out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_V3F, v3f);
}

exr_result_t
exr_part_attr_set_v3f (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v3f_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_V3F, v3f);
}

/**************************************/

exr_result_t
exr_part_attr_get_v3d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v3d_t*     out)
{
    ATTR_GET_IMPL_DEREF (EXR_ATTR_V3D, v3d);
}

exr_result_t
exr_part_attr_set_v3d (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v3d_t* val)
{
    ATTR_SET_IMPL_DEREF (EXR_ATTR_V3D, v3d);
}

/**************************************/

exr_result_t exr_part_attr_get_user (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    const char**        type,
    int32_t*            size,
    const void**        out)
{
    return EXR_ERR_UNKNOWN;
}

exr_result_t exr_part_attr_set_user (
    exr_context_t ctxt,
    int           part_index,
    const char*   name,
    const char*   type,
    int32_t       size,
    const void*   out)
{
    return EXR_ERR_UNKNOWN;
}
