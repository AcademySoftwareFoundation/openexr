/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_base.h"
#include "openexr_errors.h"

/**************************************/

void
exr_get_library_version (int* maj, int* min, int* patch, const char** extra)
{
    if (maj) *maj = OPENEXR_VERSION_MAJOR;
    if (min) *min = OPENEXR_VERSION_MINOR;
    if (patch) *patch = OPENEXR_VERSION_PATCH;
#ifdef OPENEXR_VERSION_EXTRA
    if (extra) *extra = OPENEXR_VERSION_EXTRA;
#else
    if (extra) *extra = "";
#endif
}

/**************************************/

static const char* the_default_errors[] = {
    "Success",
    "Unable to allocate memory",
    "Invalid argument to function",
    "Argument to function out of valid range",
    "Unable to open file (path does not exist or permission denied)",
    "File is not an OpenEXR file or has a bad header value",
    "File not opened for read",
    "File not opened for write",
    "Error reading from stream",
    "Error writing to stream",
    "Text too long for file flags",
    "Missing required attribute in part header",
    "Invalid attribute in part header",
    "Mismatch in chunk data vs programmatic value",
    "Attribute type mismatch",
    "Attribute type vs. size mismatch",
    "Attempt to use a scanline accessor function for a tiled image",
    "Attempt to use a tiled accessor function for a scanline image",
    "Unknown error code"
};
static int the_default_error_count =
    sizeof (the_default_errors) / sizeof (const char*);

/**************************************/

const char*
exr_get_default_error_message (exr_result_t code)
{
    int idx = (int) code;
    if (idx < 0 || idx >= the_default_error_count)
        idx = the_default_error_count - 1;
    return the_default_errors[idx];
}

/**************************************/

static int sMaxW = 0;
static int sMaxH = 0;

void
exr_set_default_maximum_image_size (int w, int h)
{
    if (w >= 0 && h >= 0)
    {
        sMaxW = w;
        sMaxH = h;
    }
}

/**************************************/

void
exr_get_default_maximum_image_size (int* w, int* h)
{
    if (w) *w = sMaxW;
    if (h) *h = sMaxH;
}

/**************************************/

static int sTileMaxW = 0;
static int sTileMaxH = 0;

void
exr_set_default_maximum_tile_size (int w, int h)
{
    if (w >= 0 && h >= 0)
    {
        sTileMaxW = w;
        sTileMaxH = h;
    }
}

/**************************************/

void
exr_get_default_maximum_tile_size (int* w, int* h)
{
    if (w) *w = sTileMaxW;
    if (h) *h = sTileMaxH;
}
