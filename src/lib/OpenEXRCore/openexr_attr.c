/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr.h"

#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

typedef struct 
{
    const char *name;
    uint8_t name_len;
    exr_ATTRIBUTE_TYPE_t type;
    size_t exp_size;
} exr_PRIV_ATTR_MAP_t;

static exr_PRIV_ATTR_MAP_t the_predefined_attr_typenames[] =
{
	{ "box2i", 5, EXR_ATTR_BOX2I, sizeof(exr_attr_box2i_t) },
	{ "box2f", 5, EXR_ATTR_BOX2F, sizeof(exr_attr_box2f_t) },
	{ "chlist", 6, EXR_ATTR_CHLIST, sizeof(exr_attr_chlist_t) },
	{ "chromaticities", 14, EXR_ATTR_CHROMATICITIES, sizeof(exr_attr_chromaticities_t) },
	{ "compression", 11, EXR_ATTR_COMPRESSION, 0 },
	{ "double", 6, EXR_ATTR_DOUBLE, 0 },
	{ "envmap", 6, EXR_ATTR_ENVMAP, 0 },
	{ "float", 5, EXR_ATTR_FLOAT, 0 },
	{ "floatvector", 11, EXR_ATTR_FLOAT_VECTOR, sizeof(exr_attr_float_vector_t) },
	{ "int", 3, EXR_ATTR_INT, 0 },
	{ "keycode", 7, EXR_ATTR_KEYCODE, sizeof(exr_attr_keycode_t) },
	{ "lineOrder", 9, EXR_ATTR_LINEORDER, 0 },
	{ "m33f", 4, EXR_ATTR_M33F, sizeof(exr_attr_m33f_t) },
	{ "m33d", 4, EXR_ATTR_M33D, sizeof(exr_attr_m33d_t) },
	{ "m44f", 4, EXR_ATTR_M44F, sizeof(exr_attr_m44f_t) },
	{ "m44d", 4, EXR_ATTR_M44D, sizeof(exr_attr_m44d_t) },
	{ "preview", 7, EXR_ATTR_PREVIEW, sizeof(exr_attr_preview_t) },
	{ "rational", 8, EXR_ATTR_RATIONAL, sizeof(exr_attr_rational_t) },
	{ "string", 6, EXR_ATTR_STRING, sizeof(exr_attr_string_t) },
	{ "stringvector", 12, EXR_ATTR_STRING_VECTOR, sizeof(exr_attr_string_vector_t) },
	{ "tiledesc", 8, EXR_ATTR_TILEDESC, sizeof(exr_attr_tiledesc_t) },
	{ "timecode", 8, EXR_ATTR_TIMECODE, sizeof(exr_attr_timecode_t) },
	{ "v2i", 3, EXR_ATTR_V2I, sizeof(exr_attr_v2i_t) },
	{ "v2f", 3, EXR_ATTR_V2F, sizeof(exr_attr_v2f_t) },
	{ "v2d", 3, EXR_ATTR_V2D, sizeof(exr_attr_v2d_t) },
	{ "v3i", 3, EXR_ATTR_V3I, sizeof(exr_attr_v3i_t) },
	{ "v3f", 3, EXR_ATTR_V3F, sizeof(exr_attr_v3f_t) },
	{ "v3d", 3, EXR_ATTR_V3D, sizeof(exr_attr_v3d_t) }
};
static int the_predefined_attr_count = sizeof(the_predefined_attr_typenames) / sizeof(exr_PRIV_ATTR_MAP_t);

/**************************************/

static int attr_init( exr_file_t *f, exr_attribute_t *nattr )
{
    switch ( nattr->type )
    {
        case EXR_ATTR_BOX2I:
        {
            exr_attr_box2i_t nil = {0};
            *(nattr->box2i) = nil;
            break;
        }
        case EXR_ATTR_BOX2F:
        {
            exr_attr_box2f_t nil = {0};
            *(nattr->box2f) = nil;
            break;
        }
        case EXR_ATTR_CHLIST:
        {
            exr_attr_chlist_t nil = {0};
            *(nattr->chlist) = nil;
            break;
        }
        case EXR_ATTR_CHROMATICITIES:
        {
            exr_attr_chromaticities_t nil = {0};
            *(nattr->chromaticities) = nil;
            break;
        }
        case EXR_ATTR_COMPRESSION:
        case EXR_ATTR_ENVMAP:
        case EXR_ATTR_LINEORDER:
            nattr->uc = 0;
            break;
        case EXR_ATTR_DOUBLE:
            nattr->d = 0.0;
            break;
        case EXR_ATTR_FLOAT:
            nattr->f = 0.0f;
            break;
        case EXR_ATTR_FLOAT_VECTOR:
        {
            exr_attr_float_vector_t nil = {0};
            *(nattr->floatvector) = nil;
            break;
        }
        case EXR_ATTR_INT:
            nattr->i = 0;
            break;
        case EXR_ATTR_KEYCODE:
        {
            exr_attr_keycode_t nil = {0};
            *(nattr->keycode) = nil;
            break;
        }
        case EXR_ATTR_M33F:
        {
            exr_attr_m33f_t nil = {0};
            *(nattr->m33f) = nil;
            break;
        }
        case EXR_ATTR_M33D:
        {
            exr_attr_m33d_t nil = {0};
            *(nattr->m33d) = nil;
            break;
        }
        case EXR_ATTR_M44F:
        {
            exr_attr_m44f_t nil = {0};
            *(nattr->m44f) = nil;
            break;
        }
        case EXR_ATTR_M44D:
        {
            exr_attr_m44f_t nil = {0};
            *(nattr->m44f) = nil;
            break;
        }
        case EXR_ATTR_PREVIEW:
        {
            exr_attr_preview_t nil = {0};
            *(nattr->preview) = nil;
            break;
        }
        case EXR_ATTR_RATIONAL:
        {
            exr_attr_rational_t nil = {0};
            *(nattr->rational) = nil;
            break;
        }
        case EXR_ATTR_STRING:
        {
            exr_attr_string_t nil = {0};
            *(nattr->string) = nil;
            break;
        }
        case EXR_ATTR_STRING_VECTOR:
        {
            exr_attr_string_vector_t nil = {0};
            *(nattr->stringvector) = nil;
            break;
        }
        case EXR_ATTR_TILEDESC:
        {
            exr_attr_tiledesc_t nil = {0};
            *(nattr->tiledesc) = nil;
            break;
        }
        case EXR_ATTR_TIMECODE:
        {
            exr_attr_timecode_t nil = {0};
            *(nattr->timecode) = nil;
            break;
        }
        case EXR_ATTR_V2I:
        {
            exr_attr_v2i_t nil = {0};
            *(nattr->v2i) = nil;
            break;
        }
        case EXR_ATTR_V2F:
        {
            exr_attr_v2f_t nil = {0};
            *(nattr->v2f) = nil;
            break;
        }
        case EXR_ATTR_V2D:
        {
            exr_attr_v2d_t nil = {0};
            *(nattr->v2d) = nil;
            break;
        }
        case EXR_ATTR_V3I:
        {
            exr_attr_v3i_t nil = {0};
            *(nattr->v3i) = nil;
            break;
        }
        case EXR_ATTR_V3F:
        {
            exr_attr_v3f_t nil = {0};
            *(nattr->v3f) = nil;
            break;
        }
        case EXR_ATTR_V3D:
        {
            exr_attr_v3d_t nil = {0};
            *(nattr->v3d) = nil;
            break;
        }
        case EXR_ATTR_OPAQUE:
        {
            exr_attr_opaquedata_t nil = {0};
            *(nattr->opaque) = nil;
            break;
        }
        case EXR_ATTR_UNKNOWN:
        case EXR_ATTR_LAST_KNOWN_TYPE:
        default:
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_ERR_INVALID_ARGUMENT,
                    "Invalid / unimplemented type (%s) in attr_init",
                    nattr->type_name );
            return EXR_ERR_INVALID_ARGUMENT;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static int attr_destroy( exr_attribute_t *attr )
{
    switch ( attr->type )
    {
        case EXR_ATTR_CHLIST:
            exr_attr_chlist_destroy( attr->chlist );
            break;
        case EXR_ATTR_FLOAT_VECTOR:
            exr_attr_float_vector_destroy( attr->floatvector );
            break;
        case EXR_ATTR_PREVIEW:
            exr_attr_preview_destroy( attr->preview );
            break;
        case EXR_ATTR_STRING:
            exr_attr_string_destroy( attr->string );
            break;
        case EXR_ATTR_STRING_VECTOR:
            exr_attr_string_vector_destroy( attr->stringvector );
            break;
        case EXR_ATTR_OPAQUE:
            exr_attr_opaquedata_destroy( attr->opaque );
            break;
        case EXR_ATTR_BOX2I:
        case EXR_ATTR_BOX2F:
        case EXR_ATTR_CHROMATICITIES:
        case EXR_ATTR_COMPRESSION:
        case EXR_ATTR_ENVMAP:
        case EXR_ATTR_LINEORDER:
        case EXR_ATTR_DOUBLE:
        case EXR_ATTR_FLOAT:
        case EXR_ATTR_INT:
        case EXR_ATTR_KEYCODE:
        case EXR_ATTR_M33F:
        case EXR_ATTR_M33D:
        case EXR_ATTR_M44F:
        case EXR_ATTR_M44D:
        case EXR_ATTR_RATIONAL:
        case EXR_ATTR_TILEDESC:
        case EXR_ATTR_TIMECODE:
        case EXR_ATTR_V2I:
        case EXR_ATTR_V2F:
        case EXR_ATTR_V2D:
        case EXR_ATTR_V3I:
        case EXR_ATTR_V3F:
        case EXR_ATTR_V3D:
        case EXR_ATTR_UNKNOWN:
        case EXR_ATTR_LAST_KNOWN_TYPE:
        default:
            break;
    }
    /* we don't care about the string because they were built into the
     * allocation block of the attribute as necessary */
    priv_free( attr );
    return 0;
}

/**************************************/

void exr_attr_list_destroy(
    exr_attribute_list_t *list )
{
    exr_attribute_list_t nil = {0};

    if ( list )
    {
        if ( list->entries )
        {
            for ( int i = 0; i < list->num_attributes; ++i )
                attr_destroy( list->entries[i] );
            priv_free( list->entries );
        }
        *list = nil;
    }
}

/**************************************/

uint64_t exr_attr_list_compute_size(
    exr_attribute_list_t *list )
{
    uint64_t retval = 0;

    if ( list )
    {
        for ( int i = 0; i < list->num_attributes; ++i )
        {
            exr_attribute_t *cur = list->entries[i];
            retval += (size_t)cur->name_length + 1;
            retval += (size_t)cur->type_name_length + 1;
            retval += sizeof(int32_t);
            switch ( cur->type )
            {
                case EXR_ATTR_BOX2I:
                    retval += sizeof(*(cur->box2i));
                    break;
                case EXR_ATTR_BOX2F:
                    retval += sizeof(*(cur->box2f));
                    break;
                case EXR_ATTR_CHLIST:
                    for ( int c = 0; c < cur->chlist->num_channels; ++c )
                    {
                        retval += cur->chlist->entries[c].name.length + 1;
                        retval += sizeof(int32_t) * 4;
                    }
                    break;
                case EXR_ATTR_CHROMATICITIES:
                    retval += sizeof(*(cur->chromaticities));
                    break;
                case EXR_ATTR_COMPRESSION:
                case EXR_ATTR_ENVMAP:
                case EXR_ATTR_LINEORDER:
                    retval += sizeof(uint8_t);
                    break;
                case EXR_ATTR_DOUBLE:
                    retval += sizeof(double);
                    break;
                case EXR_ATTR_FLOAT:
                    retval += sizeof(float);
                    break;
                case EXR_ATTR_FLOAT_VECTOR:
                    retval += sizeof(float) * cur->floatvector->length;
                    break;
                case EXR_ATTR_INT:
                    retval += sizeof(int32_t);
                    break;
                case EXR_ATTR_KEYCODE:
                    retval += sizeof(*(cur->keycode));
                    break;
                case EXR_ATTR_M33F:
                    retval += sizeof(*(cur->m33f));
                    break;
                case EXR_ATTR_M33D:
                    retval += sizeof(*(cur->m33d));
                    break;
                case EXR_ATTR_M44F:
                    retval += sizeof(*(cur->m44f));
                    break;
                case EXR_ATTR_M44D:
                    retval += sizeof(*(cur->m44d));
                    break;
                case EXR_ATTR_PREVIEW:
                    retval += (size_t)cur->preview->width * (size_t)cur->preview->height * (size_t)4;
                    break;
                case EXR_ATTR_RATIONAL:
                    retval += sizeof(*(cur->rational));
                    break;
                case EXR_ATTR_STRING:
                    retval += (size_t)cur->string->length;
                    break;
                case EXR_ATTR_STRING_VECTOR:
                    for ( int s = 0; s < cur->stringvector->n_strings; ++s )
                    {
                        retval += cur->stringvector->strings[s].length;
                        retval += sizeof(int32_t);
                    }
                    break;
                case EXR_ATTR_TILEDESC:
                    retval += sizeof(*(cur->tiledesc));
                    break;
                case EXR_ATTR_TIMECODE:
                    retval += sizeof(*(cur->timecode));
                    break;
                case EXR_ATTR_V2I:
                    retval += sizeof(*(cur->v2i));
                    break;
                case EXR_ATTR_V2F:
                    retval += sizeof(*(cur->v2f));
                    break;
                case EXR_ATTR_V2D:
                    retval += sizeof(*(cur->v2d));
                    break;
                case EXR_ATTR_V3I:
                    retval += sizeof(*(cur->v3i));
                    break;
                case EXR_ATTR_V3F:
                    retval += sizeof(*(cur->v3f));
                    break;
                case EXR_ATTR_V3D:
                    retval += sizeof(*(cur->v3d));
                    break;
                case EXR_ATTR_OPAQUE:
                    if ( cur->opaque->packed_data )
                        retval += cur->opaque->size;
                    else if ( cur->opaque->unpacked_data )
                    {
                        if ( cur->opaque->pack_func_ptr )
                        {
                            if ( 0 != cur->opaque->pack_func_ptr(
                                     cur->opaque->unpacked_data,
                                     cur->opaque->unpacked_size,
                                     &(cur->opaque->size),
                                     &(cur->opaque->packed_data) ) )
                                return (uint64_t)-1;
                            retval += cur->opaque->size;
                        }
                    }
                    break;
                case EXR_ATTR_UNKNOWN:
                case EXR_ATTR_LAST_KNOWN_TYPE:
                default:
                    return (uint64_t)-1;
                    break;
            }
        }
    }

    return retval;
}

/**************************************/

exr_attribute_t *exr_attr_list_find_by_name(
    exr_file_t *f,
    exr_attribute_list_t *list,
    const char *name )
{
    exr_attribute_t **it = NULL;
    exr_attribute_t **first = NULL;
    exr_attribute_t **end = NULL;
    int step, count, cmp;

    if ( ! name || name[0] == '\0' )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid name passed to find_by_name" );
        return NULL;
    }

    if ( ! list )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid list pointer passed to find_by_name" );
        return NULL;
    }

    first = list->sorted_entries;
    count = list->num_attributes;
    end = first + count;
    /* lower bound search w/ equality check */
    while ( count > 0 ) 
    {
        it = first;
        step = count / 2;
        it += step;
        cmp = strcmp( (*it)->name, name );
        if ( cmp == 0 )
        {
            // early exi
            return (*it);
        }

        if ( cmp < 0 )
        {
            first = ++it;
            count -= step + 1;
        }
        else
            count = step;
    }

    if ( first && first < end &&
         0 == strcmp( (*first)->name, name ) )
    {
        return (*first);
    }
    return NULL;
}

/**************************************/

static int add_to_list(
    exr_file_t *f,
    exr_attribute_list_t *list,
    exr_attribute_t *nattr,
    const char *name )
{
    int cattrsz = list->num_attributes;
    int nattrsz = cattrsz + 1;
    int insertpos;
    exr_attribute_t **attrs = list->entries;
    exr_attribute_t **sorted_attrs = list->sorted_entries;

    if ( nattrsz > list->num_alloced )
    {
        size_t nsize = list->num_alloced * 2;
        if ( nattrsz > nsize )
            nsize = nattrsz + 1;
        attrs = (exr_attribute_t **)priv_alloc( sizeof(exr_attribute_t *) * nsize * 2 );
        if ( ! attrs )
        {
            priv_free( nattr );
            if ( f )
                return EXR_GETFILE(f)->standard_error( f, EXR_ERR_OUT_OF_MEMORY );
            return EXR_ERR_OUT_OF_MEMORY;
        }

        list->num_alloced = (int32_t)nsize;
        sorted_attrs = attrs + nsize;

        for ( int i = 0; i < cattrsz; ++i )
        {
            attrs[i] = list->entries[i];
            sorted_attrs[i] = list->sorted_entries[i];
        }

        priv_free( list->entries );
        list->entries = attrs;
        list->sorted_entries = sorted_attrs;
    }
    attrs[cattrsz] = nattr;
    sorted_attrs[cattrsz] = nattr;
    insertpos = cattrsz - 1;

    // FYI: qsort is shockingly slow, just do a quick search and
    // bubble it up until it's in the correct location
    while ( insertpos >= 0 )
    {
        exr_attribute_t *prev = sorted_attrs[insertpos];

        if ( strcmp( nattr->name, prev->name ) >= 0 )
            break;

        sorted_attrs[insertpos + 1] = prev;
        sorted_attrs[insertpos] = nattr;
        --insertpos;
    }

    list->num_attributes = nattrsz;
    attr_init( f, nattr );

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int validate_attr_arguments(
    exr_file_t *f,
    exr_attribute_list_t *list,
    const char *name,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr )
{
    exr_attribute_t *nattr = NULL;
    if ( ! list )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid list pointer to attr_list_add" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! attr )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid output attribute pointer location to attr_list_add" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    *attr = NULL;

    if ( data_len < 0 )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Extra data storage requested negative length (%d)", data_len );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    else if ( data_len > 0 && ! data_ptr )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Extra data storage output pointer must be provided when requesting extra data (%d)", data_len );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    else if ( data_ptr )
        *data_ptr = NULL;

    if ( ! name || name[0] == '\0' )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid name to add_by_type" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    /* is it already in the list? */
    nattr = exr_attr_list_find_by_name( f, list, name );
    if ( nattr )
    {
        if ( data_ptr && data_len > 0 )
        {
            if ( f )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_ERR_INVALID_ARGUMENT,
                    "Entry '%s' (type %s) already in list but requesting additional data",
                    name, nattr->type_name );
            return EXR_ERR_INVALID_ARGUMENT;
        }

        *attr = nattr;
        return -1;
    }
        
    return EXR_ERR_SUCCESS;
}

/**************************************/

static void check_attr_handler(
    exr_file_t *f,
    exr_attribute_t *attr )
{
    if ( f && attr->type == EXR_ATTR_OPAQUE )
    {
        exr_attribute_t *handler = exr_attr_list_find_by_name(
            f, &(EXR_GETFILE(f)->custom_handlers), attr->type_name );
        if ( handler )
        {
            attr->opaque->unpack_func_ptr = handler->opaque->unpack_func_ptr;
            attr->opaque->pack_func_ptr = handler->opaque->pack_func_ptr;
            attr->opaque->destroy_func_ptr = handler->opaque->destroy_func_ptr;
        }
    }
}

/**************************************/

int exr_attr_list_add_by_type(
    exr_file_t *f,
    exr_attribute_list_t *list,
    const char *name,
    const char *type,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr )
{
    int rval = EXR_ERR_INVALID_ARGUMENT;
    const exr_PRIV_ATTR_MAP_t *known = NULL;
    int32_t nlen, tlen, mlen = EXR_SHORTNAME_MAXLEN;
    size_t attrblocksz = sizeof(exr_attribute_t);
    uint8_t *ptr = NULL;
    exr_attribute_t *nattr = NULL;
    exr_attribute_t nil = {0};

    if ( ! type || type[0] == '\0' )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid type to add_by_type" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    rval = validate_attr_arguments( f, list, name, data_len, data_ptr, attr );
    if ( rval != EXR_ERR_SUCCESS )
    {
        if ( rval < 0 )
        {
            if ( 0 != strcmp( type, (*attr)->type_name ) )
            {
                nattr = *attr;
                *attr = NULL;
                if ( f )
                    return EXR_GETFILE(f)->print_error(
                        f, EXR_ERR_INVALID_ARGUMENT,
                        "Entry '%s' already in list but with different type ('%s' vs requested '%s')",
                        name, nattr->type_name, type );
                return EXR_ERR_INVALID_ARGUMENT;
            }
            return EXR_ERR_SUCCESS;
        }
        return rval;
    }

    if ( f )
        mlen = (int32_t)EXR_GETFILE(f)->max_name_length;
    nlen = (int32_t)strlen( name );

    if ( nlen > mlen )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Provided name '%s' too long for file (len %d, max %d)",
                name, nlen, mlen );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    
    tlen = (int32_t)strlen( type );
    if ( tlen > mlen )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Provided type name '%s' too long for file (len %d, max %d)",
                type, nlen, mlen );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    for ( int i = 0; i < the_predefined_attr_count; ++i )
    {
        if ( 0 == strcmp( type, the_predefined_attr_typenames[i].name ) )
        {
            known = &(the_predefined_attr_typenames[i]);
            break;
        }
    }

    if ( known )
    {
        attrblocksz += nlen + 1;
        attrblocksz += known->exp_size;
        attrblocksz += data_len;
        ptr = (uint8_t *)priv_alloc( attrblocksz );
        if ( ! ptr )
        {
            if ( f )
                return EXR_GETFILE(f)->standard_error( f, EXR_ERR_OUT_OF_MEMORY );
            return EXR_ERR_OUT_OF_MEMORY;
        }
        nattr = (exr_attribute_t *)ptr;
        *nattr = nil;
        ptr += sizeof(exr_attribute_t);

        memcpy( ptr, name, nlen + 1 );
        nattr->name = (char *)ptr;
        ptr += nlen + 1;

        nattr->type_name = known->name;
        nattr->name_length = (uint8_t)nlen;
        nattr->type_name_length = (uint8_t)tlen;
        nattr->type = known->type;
        if ( known->exp_size > 0 )
        {
            nattr->rawptr = ptr;
            ptr += known->exp_size;
        }
    }
    else
    {
        attrblocksz += nlen + 1;
        attrblocksz += tlen + 1;
        attrblocksz += sizeof(exr_attr_opaquedata_t);
        attrblocksz += data_len;
        ptr = (uint8_t *)priv_alloc( attrblocksz );
        if ( ! ptr )
        {
            if ( f )
                return EXR_GETFILE(f)->standard_error( f, EXR_ERR_OUT_OF_MEMORY );
            return EXR_ERR_OUT_OF_MEMORY;
        }
        nattr = (exr_attribute_t *)ptr;
        *nattr = nil;
        ptr += sizeof(exr_attribute_t);

        memcpy( ptr, name, nlen + 1 );
        nattr->name = (char *)ptr;
        ptr += nlen + 1;

        memcpy( ptr, type, tlen + 1 );
        nattr->type_name = (char *)ptr;
        ptr += tlen + 1;

        nattr->name_length = (uint8_t)nlen;
        nattr->type_name_length = (uint8_t)tlen;
        nattr->type = EXR_ATTR_OPAQUE;
        nattr->opaque = (exr_attr_opaquedata_t *)ptr;
        ptr += sizeof(exr_attr_opaquedata_t);
    }
    rval = add_to_list( f, list, nattr, name );
    if ( rval == EXR_ERR_SUCCESS )
    {
        *attr = nattr;
        if ( data_ptr )
        {
            if ( data_len > 0 )
                *data_ptr = ptr;
            else
                *data_ptr = NULL;
        }
        check_attr_handler( f, nattr );
    }
    return rval;
}

/**************************************/

int exr_attr_list_add(
    exr_file_t *f,
    exr_attribute_list_t *list,
    const char *name,
    exr_ATTRIBUTE_TYPE_t type,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr )
{
    int rval = EXR_ERR_INVALID_ARGUMENT;
    const exr_PRIV_ATTR_MAP_t *known = NULL;
    int32_t nlen, tidx, mlen = 255;
    size_t attrblocksz = sizeof(exr_attribute_t);
    uint8_t *ptr = NULL;
    exr_attribute_t *nattr = NULL;
    exr_attribute_t nil = {0};

    rval = validate_attr_arguments( f, list, name, data_len, data_ptr, attr );
    if ( rval != EXR_ERR_SUCCESS )
    {
        if ( rval < 0 )
        {
            if ( (*attr)->type != type )
            {
                nattr = *attr;
                *attr = NULL;
                if ( f )
                    return EXR_GETFILE(f)->print_error(
                        f, EXR_ERR_INVALID_ARGUMENT,
                        "Entry '%s' already in list but with different type ('%s')",
                        name, nattr->type_name );
                return EXR_ERR_INVALID_ARGUMENT;
            }
            return EXR_ERR_SUCCESS;
        }
        return rval;
    }

    if ( f )
        mlen = (int32_t)EXR_GETFILE(f)->max_name_length;
    nlen = (int32_t)strlen( name );
    if ( nlen > mlen )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Provided name '%s' too long for file (len %d, max %d)",
                name, nlen, mlen );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    tidx = ((int)type) - 1;
    if ( tidx < 0 || tidx >= the_predefined_attr_count )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid type enum for '%s' in create by builtin type (type %d)",
                name, (int)type );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    known = &(the_predefined_attr_typenames[tidx]);

    attrblocksz += nlen + 1;
    attrblocksz += known->exp_size;
    attrblocksz += data_len;
    ptr = (uint8_t *)priv_alloc( attrblocksz );
    if ( ! ptr )
    {
        if ( f )
            return EXR_GETFILE(f)->standard_error( f, EXR_ERR_OUT_OF_MEMORY );
        return EXR_ERR_OUT_OF_MEMORY;
    }

    nattr = (exr_attribute_t *)ptr;
    *nattr = nil;

    ptr += sizeof(exr_attribute_t);
    memcpy( ptr, name, nlen + 1 );
    nattr->name = (char *)ptr;

    ptr += nlen + 1;
    nattr->type_name = known->name;
    nattr->name_length = (uint8_t)nlen;
    nattr->type_name_length = known->name_len;
    nattr->type = known->type;
    if ( known->exp_size > 0 )
    {
        nattr->rawptr = ptr;
        ptr += known->exp_size;
    }
    else
        nattr->rawptr = NULL;

    rval = add_to_list( f, list, nattr, name );
    if ( rval == 0 )
    {
        *attr = nattr;
        if ( data_ptr )
        {
            if ( data_len > 0 )
                *data_ptr = ptr;
            else
                *data_ptr = NULL;
        }
    }
    return rval;
}

/**************************************/

int exr_attr_list_add_static_name(
    exr_file_t *f,
    exr_attribute_list_t *list,
    const char *name,
    exr_ATTRIBUTE_TYPE_t type,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr )
{
    int rval = EXR_ERR_INVALID_ARGUMENT;
    const exr_PRIV_ATTR_MAP_t *known = NULL;
    int32_t nlen, tidx, mlen = EXR_SHORTNAME_MAXLEN;
    size_t attrblocksz = sizeof(exr_attribute_t);
    uint8_t *ptr = NULL;
    exr_attribute_t *nattr = NULL;
    exr_attribute_t nil = {0};

    rval = validate_attr_arguments( f, list, name, data_len, data_ptr, attr );
    if ( rval != EXR_ERR_SUCCESS )
    {
        if ( rval < 0 )
        {
            if ( (*attr)->type != type )
            {
                nattr = *attr;
                *attr = NULL;
                if ( f )
                    return EXR_GETFILE(f)->print_error(
                        f, EXR_ERR_INVALID_ARGUMENT,
                        "Entry '%s' already in list but with different type ('%s')",
                        name, nattr->type_name );
                return EXR_ERR_INVALID_ARGUMENT;
            }
            return EXR_ERR_SUCCESS;
        }
        return rval;
    }

    if ( f )
        mlen = (int32_t)EXR_GETFILE(f)->max_name_length;
    nlen = (int32_t)strlen( name );
    if ( nlen > mlen )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Provided name '%s' too long for file (len %d, max %d)",
                name, nlen, mlen );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    tidx = ((int)type) - 1;
    if ( tidx < 0 || tidx >= the_predefined_attr_count )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid type enum for '%s' in create by builtin type (type %d)",
                name, (int)type );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    known = &(the_predefined_attr_typenames[tidx]);

    attrblocksz += known->exp_size;
    attrblocksz += data_len;
    ptr = (uint8_t *)priv_alloc( attrblocksz );
    if ( ! ptr )
        return EXR_GETFILE(f)->standard_error( f, EXR_ERR_OUT_OF_MEMORY );
    nattr = (exr_attribute_t *)ptr;
    *nattr = nil;
    ptr += sizeof(exr_attribute_t);
    nattr->name = name;
    nattr->type_name = known->name;
    nattr->name_length = (uint8_t)nlen;
    nattr->type_name_length = known->name_len;
    nattr->type = known->type;
    if ( known->exp_size > 0 )
    {
        nattr->rawptr = ptr;
        ptr += known->exp_size;
    }

    rval = add_to_list( f, list, nattr, name );
    if ( rval == EXR_ERR_SUCCESS )
    {
        *attr = nattr;
        if ( data_ptr )
        {
            if ( data_len > 0 )
                *data_ptr = ptr;
            else
                *data_ptr = NULL;
        }
    }
    return rval;
}

/**************************************/

int exr_attr_list_remove(
    exr_file_t *f,
    exr_attribute_list_t *list,
    exr_attribute_t *attr )
{
    int cattrsz, attridx = -1;
    exr_attribute_t **attrs;

    if ( ! attr )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "NULL attribute passed to remove" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! list )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid list pointer to remove attribute" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    cattrsz = list->num_attributes;
    attrs = list->entries;
    for ( int i = 0; i < cattrsz; ++i )
    {
        if ( attrs[i] == attr )
        {
            attridx = i;
            break;
        }
    }

    if ( attridx == -1 )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Attribute not in list" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    list->entries[attridx] = NULL;
    for ( int i = attridx; i < (cattrsz - 1); ++i )
        attrs[i] = attrs[i + 1];
    list->num_attributes = cattrsz - 1;

    attrs = list->sorted_entries;
    attridx = 0;
    for ( int i = 0; i < cattrsz; ++i )
    {
        if ( attrs[i] == attr )
            continue;
        attrs[attridx++] = attrs[i];
    }

    attr_destroy( attr );

    return EXR_ERR_SUCCESS;
}
