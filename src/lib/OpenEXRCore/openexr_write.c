// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "openexr.h"

#include "openexr_priv_constants.h"
#include "openexr_priv_file.h"

#include <string.h>

/**************************************/

int EXR_FUN(set_longname_support)(
    EXR_TYPE(FILE) *file, int onoff )
{
    EXR_TYPE(PRIV_FILE) *pf = EXR_GETFILE(file);
    EXR_TYPE(PRIV_PART) *curp;
    uint8_t oldval, newval;

    if ( ! file )
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    if ( ! pf->write_fn )
        return pf->report_error( file, EXR_DEF(ERR_INVALID_ARGUMENT),
                                 "File not open for write" );

    oldval = pf->max_name_length;
    newval = EXR_SHORTNAME_MAXLEN;
    if ( onoff )
        newval = 255;

    if ( oldval > newval )
    {
        for ( int p = 0; p < pf->num_parts; ++p )
        {
            EXR_TYPE(PRIV_PART) *curp = pf->parts[p];
            for ( int a = 0; a < curp->attributes.num_attributes; ++a )
            {
                EXR_TYPE(attribute) *curattr = curp->attributes.entries[a];
                if ( curattr->name_length > newval ||
                     curattr->type_name_length > newval )
                {
                    return pf->print_error(
                        file, EXR_DEF(ERR_NAME_TOO_LONG),
                        "Part %d, attribute '%s' (type '%s') has a name too long for new longname setting (%d)",
                        curp->part_index, curattr->name, curattr->type_name, (int)newval );
                }
                if ( curattr->type == EXR_DEF(ATTR_CHLIST) )
                {
                    EXR_TYPE(attr_chlist) *chs = curattr->chlist;
                    for ( int c = 0; c < chs->num_channels; ++c )
                    {
                        if ( chs->entries[c].name.length > newval )
                        {
                            return pf->print_error(
                                file, EXR_DEF(ERR_NAME_TOO_LONG),
                                "Part %d, channel '%s' has a name too long for new longname setting (%d)",
                                curp->part_index, chs->entries[c].name.str, (int)newval );
                        }
                    }
                }
            }
        }
    }
    pf->max_name_length = newval;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN (add_part) (
    EXR_TYPE (FILE) * file, const char* partname, EXR_TYPE (STORAGE_TYPE) type)
{
    EXR_TYPE (PRIV_PART) * part;
    uint8_t*    namestr;
    int         rv;
    int32_t     attrsz  = -1;
    const char* typestr = NULL;

    if (!file) return EXR_DEF (ERR_INVALID_ARGUMENT);

    rv = priv_add_part (file, &part);
    if (rv != EXR_DEF (ERR_SUCCESS)) return rv;

    switch (type)
    {
        case EXR_DEF (STORAGE_SCANLINE):
            typestr = "scanlineimage";
            attrsz  = 13;
            break;
        case EXR_DEF (STORAGE_TILED):
            typestr = "tiledimage";
            attrsz  = 10;
            break;
        case EXR_DEF (STORAGE_DEEP_SCANLINE):
            typestr = "deepscanline";
            attrsz  = 12;
            break;
        case EXR_DEF (STORAGE_DEEP_TILED):
            typestr = "deeptile";
            attrsz  = 8;
            break;
        default:
            return EXR_GETFILE (file)->print_error (
                file,
                EXR_DEF (ERR_INVALID_ATTR),
                "Invalid storage type %d for new part",
                (int) type);
    }

    rv = EXR_FUN (attr_list_add_static_name) (
        file,
        &(part->attributes),
        EXR_REQ_TYPE_STR,
        EXR_DEF (ATTR_STRING),
        attrsz + 1,
        &namestr,
        &(part->type));

    if (rv != EXR_DEF (ERR_SUCCESS)) return rv;

    memcpy (namestr, typestr, attrsz + 1);

    if (partname)
    {
        size_t pnamelen = strlen (partname);
        if (pnamelen >= INT32_MAX)
        {
            return EXR_GETFILE (file)->print_error (
                file,
                EXR_DEF (ERR_INVALID_ATTR),
                "Part name '%s': Invalid name length %lu",
                partname,
                pnamelen);
        }

        rv = EXR_FUN (attr_list_add_static_name) (
            file,
            &(part->attributes),
            EXR_REQ_NAME_STR,
            EXR_DEF (ATTR_STRING),
            pnamelen + 1,
            &namestr,
            &(part->name));

        if (rv == EXR_DEF (ERR_SUCCESS))
        {
            memcpy (namestr, partname, pnamelen + 1);
        }
    }

    return rv;
}
