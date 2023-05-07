//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMF_INTERNAL_DWA_HELPERS_H_HAS_BEEN_INCLUDED
#    error "only include internal_dwa_helpers.h"
#endif

/**************************************/

typedef struct _Classifier
{
    const char*      _suffix;
    CompressorScheme _scheme;
    exr_pixel_type_t _type;
    int              _cscIdx;
    bool             _caseInsensitive;
    bool             _stringStatic;
} Classifier;

static Classifier sDefaultChannelRules[] = {
    {"R", LOSSY_DCT, EXR_PIXEL_HALF, 0, false, true},
    {"R", LOSSY_DCT, EXR_PIXEL_FLOAT, 0, false, true},
    {"G", LOSSY_DCT, EXR_PIXEL_HALF, 1, false, true},
    {"G", LOSSY_DCT, EXR_PIXEL_FLOAT, 1, false, true},
    {"B", LOSSY_DCT, EXR_PIXEL_HALF, 2, false, true},
    {"B", LOSSY_DCT, EXR_PIXEL_FLOAT, 2, false, true},
    {"Y", LOSSY_DCT, EXR_PIXEL_HALF, -1, false, true},
    {"Y", LOSSY_DCT, EXR_PIXEL_FLOAT, -1, false, true},
    {"BY", LOSSY_DCT, EXR_PIXEL_HALF, -1, false, true},
    {"BY", LOSSY_DCT, EXR_PIXEL_FLOAT, -1, false, true},
    {"RY", LOSSY_DCT, EXR_PIXEL_HALF, -1, false, true},
    {"RY", LOSSY_DCT, EXR_PIXEL_FLOAT, -1, false, true},
    {"A", RLE, EXR_PIXEL_UINT, -1, false, true},
    {"A", RLE, EXR_PIXEL_HALF, -1, false, true},
    {"A", RLE, EXR_PIXEL_FLOAT, -1, false, true}};

static Classifier sLegacyChannelRules[] = {
    {"r", LOSSY_DCT, EXR_PIXEL_HALF, 0, true, true},
    {"r", LOSSY_DCT, EXR_PIXEL_FLOAT, 0, true, true},
    {"red", LOSSY_DCT, EXR_PIXEL_HALF, 0, true, true},
    {"red", LOSSY_DCT, EXR_PIXEL_FLOAT, 0, true, true},
    {"g", LOSSY_DCT, EXR_PIXEL_HALF, 1, true, true},
    {"g", LOSSY_DCT, EXR_PIXEL_FLOAT, 1, true, true},
    {"grn", LOSSY_DCT, EXR_PIXEL_HALF, 1, true, true},
    {"grn", LOSSY_DCT, EXR_PIXEL_FLOAT, 1, true, true},
    {"green", LOSSY_DCT, EXR_PIXEL_HALF, 1, true, true},
    {"green", LOSSY_DCT, EXR_PIXEL_FLOAT, 1, true, true},
    {"b", LOSSY_DCT, EXR_PIXEL_HALF, 2, true, true},
    {"b", LOSSY_DCT, EXR_PIXEL_FLOAT, 2, true, true},
    {"blu", LOSSY_DCT, EXR_PIXEL_HALF, 2, true, true},
    {"blu", LOSSY_DCT, EXR_PIXEL_FLOAT, 2, true, true},
    {"blue", LOSSY_DCT, EXR_PIXEL_HALF, 2, true, true},
    {"blue", LOSSY_DCT, EXR_PIXEL_FLOAT, 2, true, true},
    {"y", LOSSY_DCT, EXR_PIXEL_HALF, -1, true, true},
    {"y", LOSSY_DCT, EXR_PIXEL_FLOAT, -1, true, true},
    {"by", LOSSY_DCT, EXR_PIXEL_HALF, -1, true, true},
    {"by", LOSSY_DCT, EXR_PIXEL_FLOAT, -1, true, true},
    {"ry", LOSSY_DCT, EXR_PIXEL_HALF, -1, true, true},
    {"ry", LOSSY_DCT, EXR_PIXEL_FLOAT, -1, true, true},
    {"a", RLE, EXR_PIXEL_UINT, -1, true, true},
    {"a", RLE, EXR_PIXEL_HALF, -1, true, true},
    {"a", RLE, EXR_PIXEL_FLOAT, -1, true, true}};

static inline void
Classifier_destroy (Classifier* p)
{
    if (p->_suffix && !p->_stringStatic) internal_exr_free ((char*) p->_suffix);
}

static exr_result_t
Classifier_read (Classifier* out, const uint8_t** ptr, uint64_t* size)
{
    const uint8_t* curin = *ptr;
    size_t         len   = 0;
    uint8_t        value;
    uint8_t        type;

    if (*size <= 3) return EXR_ERR_CORRUPT_CHUNK;

    //throw IEX_NAMESPACE::InputExc ("Error uncompressing DWA data"
    //" (truncated rule).");

    {
        // maximum length of string plus one byte for terminating NULL
        char  suffix[128 + 1];
        char* mem;
        memset (suffix, 0, 128 + 1);
        for (; len < 128 + 1; ++len)
        {
            if (len > (*size - 3)) return EXR_ERR_CORRUPT_CHUNK;
            if (curin[len] == '\0') break;
            suffix[len] = curin[len];
        }
        len += 1;
        if (len == 128 + 1) return EXR_ERR_CORRUPT_CHUNK;

        mem = internal_exr_alloc (len);
        if (!mem) return EXR_ERR_OUT_OF_MEMORY;

        memcpy (mem, suffix, len);
        out->_suffix       = mem;
        out->_stringStatic = false;
    }

    if (*size < len + 2 * sizeof (uint8_t))
    {
        return EXR_ERR_CORRUPT_CHUNK;
        //throw IEX_NAMESPACE::InputExc ("Error uncompressing DWA data"
        //" (truncated rule).");
    }

    curin += len;

    value = curin[0];
    type  = curin[1];

    curin += 2;

    *ptr = curin;
    *size -= len + 2 * sizeof (uint8_t);

    out->_cscIdx = (int) (value >> 4) - 1;
    if (out->_cscIdx < -1 || out->_cscIdx >= 3)
    {
        return EXR_ERR_CORRUPT_CHUNK;
        //throw IEX_NAMESPACE::InputExc ("Error uncompressing DWA data"
        //" (corrupt cscIdx rule).");
    }

    out->_scheme = (CompressorScheme) ((value >> 2) & 3);
    if (out->_scheme < 0 || out->_scheme >= NUM_COMPRESSOR_SCHEMES)
    {
        return EXR_ERR_CORRUPT_CHUNK;
        //throw IEX_NAMESPACE::InputExc ("Error uncompressing DWA data"
        //" (corrupt scheme rule).");
    }

    out->_caseInsensitive = (value & 1 ? true : false);

    if (type >= EXR_PIXEL_LAST_TYPE)
    {
        return EXR_ERR_CORRUPT_CHUNK;
        //throw IEX_NAMESPACE::InputExc ("Error uncompressing DWA data"
        //" (corrupt rule).");
    }

    out->_type = (exr_pixel_type_t) type;
    return EXR_ERR_SUCCESS;
}

static inline int
Classifier_match (
    const Classifier* me, const char* suffix, const exr_pixel_type_t type)
{
    if (me->_type != type) return false;

    if (me->_caseInsensitive) return strcasecmp (suffix, me->_suffix) == 0;

    return strcmp (suffix, me->_suffix) == 0;
}

static inline uint64_t
Classifier_size (const Classifier* me)
{
    return strlen (me->_suffix) + 1 + 2 * sizeof (uint8_t);
}

static inline uint64_t
Classifier_write (const Classifier* me, uint8_t** ptr)
{
    uint8_t* outptr    = *ptr;
    uint8_t  value     = 0;
    uint64_t sizeBytes = strlen (me->_suffix) + 1;

    memcpy (outptr, me->_suffix, sizeBytes);
    outptr += sizeBytes;

    value |= ((uint8_t) (me->_cscIdx + 1) & 15) << 4;
    value |= ((uint8_t) me->_scheme & 3) << 2;
    value |= (uint8_t) me->_caseInsensitive & 1;

    outptr[0] = value;
    outptr[1] = (uint8_t) me->_type;
    outptr += 2;
    *ptr = outptr;
    return sizeBytes + 2;
}

static inline const char*
Classifier_find_suffix (const char* channel_name)
{
    const char* suffix = strrchr (channel_name, '.');
    if (suffix) { suffix += 1; }
    else { suffix = channel_name; }
    return suffix;
}
