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
    EXR_TYPE(ATTRIBUTE_TYPE) type;
    size_t exp_size;
} EXR_TYPE(PRIV_ATTR_MAP);

static EXR_TYPE(PRIV_ATTR_MAP) the_predefined_attr_typenames[] =
{
	{ "box2i", 5, EXR_DEF(ATTR_BOX2I), sizeof(EXR_TYPE(attr_box2i)) },
	{ "box2f", 5, EXR_DEF(ATTR_BOX2F), sizeof(EXR_TYPE(attr_box2f)) },
	{ "chlist", 6, EXR_DEF(ATTR_CHLIST), sizeof(EXR_TYPE(attr_chlist)) },
	{ "chromaticities", 14, EXR_DEF(ATTR_CHROMATICITIES), sizeof(EXR_TYPE(attr_chromaticities)) },
	{ "compression", 11, EXR_DEF(ATTR_COMPRESSION), 0 },
	{ "double", 6, EXR_DEF(ATTR_DOUBLE), 0 },
	{ "envmap", 6, EXR_DEF(ATTR_ENVMAP), 0 },
	{ "float", 5, EXR_DEF(ATTR_FLOAT), 0 },
	{ "floatvector", 11, EXR_DEF(ATTR_FLOAT_VECTOR), sizeof(EXR_TYPE(attr_float_vector)) },
	{ "int", 3, EXR_DEF(ATTR_INT), 0 },
	{ "keycode", 7, EXR_DEF(ATTR_KEYCODE), sizeof(EXR_TYPE(attr_keycode)) },
	{ "lineOrder", 9, EXR_DEF(ATTR_LINEORDER), 0 },
	{ "m33f", 4, EXR_DEF(ATTR_M33F), sizeof(EXR_TYPE(attr_m33f)) },
	{ "m33d", 4, EXR_DEF(ATTR_M33D), sizeof(EXR_TYPE(attr_m33d)) },
	{ "m44f", 4, EXR_DEF(ATTR_M44F), sizeof(EXR_TYPE(attr_m44f)) },
	{ "m44d", 4, EXR_DEF(ATTR_M44D), sizeof(EXR_TYPE(attr_m44d)) },
	{ "preview", 7, EXR_DEF(ATTR_PREVIEW), sizeof(EXR_TYPE(attr_preview)) },
	{ "rational", 8, EXR_DEF(ATTR_RATIONAL), sizeof(EXR_TYPE(attr_rational)) },
	{ "string", 6, EXR_DEF(ATTR_STRING), sizeof(EXR_TYPE(attr_string)) },
	{ "stringvector", 12, EXR_DEF(ATTR_STRING_VECTOR), sizeof(EXR_TYPE(attr_string_vector)) },
	{ "tiledesc", 8, EXR_DEF(ATTR_TILEDESC), sizeof(EXR_TYPE(attr_tiledesc)) },
	{ "timecode", 8, EXR_DEF(ATTR_TIMECODE), sizeof(EXR_TYPE(attr_timecode)) },
	{ "v2i", 3, EXR_DEF(ATTR_V2I), sizeof(EXR_TYPE(attr_v2i)) },
	{ "v2f", 3, EXR_DEF(ATTR_V2F), sizeof(EXR_TYPE(attr_v2f)) },
	{ "v2d", 3, EXR_DEF(ATTR_V2D), sizeof(EXR_TYPE(attr_v2d)) },
	{ "v3i", 3, EXR_DEF(ATTR_V3I), sizeof(EXR_TYPE(attr_v3i)) },
	{ "v3f", 3, EXR_DEF(ATTR_V3F), sizeof(EXR_TYPE(attr_v3f)) },
	{ "v3d", 3, EXR_DEF(ATTR_V3D), sizeof(EXR_TYPE(attr_v3d)) }
};
static int the_predefined_attr_count = sizeof(the_predefined_attr_typenames) / sizeof(EXR_TYPE(PRIV_ATTR_MAP));

/**************************************/

static int attr_init( EXR_TYPE(FILE) *f, EXR_TYPE(attribute) *nattr )
{
    switch ( nattr->type )
    {
        case EXR_DEF(ATTR_BOX2I):
        {
            EXR_TYPE(attr_box2i) nil = {0};
            *(nattr->box2i) = nil;
            break;
        }
        case EXR_DEF(ATTR_BOX2F):
        {
            EXR_TYPE(attr_box2f) nil = {0};
            *(nattr->box2f) = nil;
            break;
        }
        case EXR_DEF(ATTR_CHLIST):
        {
            EXR_TYPE(attr_chlist) nil = {0};
            *(nattr->chlist) = nil;
            break;
        }
        case EXR_DEF(ATTR_CHROMATICITIES):
        {
            EXR_TYPE(attr_chromaticities) nil = {0};
            *(nattr->chromaticities) = nil;
            break;
        }
        case EXR_DEF(ATTR_COMPRESSION):
        case EXR_DEF(ATTR_ENVMAP):
        case EXR_DEF(ATTR_LINEORDER):
            nattr->uc = 0;
            break;
        case EXR_DEF(ATTR_DOUBLE):
            nattr->d = 0.0;
            break;
        case EXR_DEF(ATTR_FLOAT):
            nattr->f = 0.0f;
            break;
        case EXR_DEF(ATTR_FLOAT_VECTOR):
        {
            EXR_TYPE(attr_float_vector) nil = {0};
            *(nattr->floatvector) = nil;
            break;
        }
        case EXR_DEF(ATTR_INT):
            nattr->i = 0;
            break;
        case EXR_DEF(ATTR_KEYCODE):
        {
            EXR_TYPE(attr_keycode) nil = {0};
            *(nattr->keycode) = nil;
            break;
        }
        case EXR_DEF(ATTR_M33F):
        {
            EXR_TYPE(attr_m33f) nil = {0};
            *(nattr->m33f) = nil;
            break;
        }
        case EXR_DEF(ATTR_M33D):
        {
            EXR_TYPE(attr_m33d) nil = {0};
            *(nattr->m33d) = nil;
            break;
        }
        case EXR_DEF(ATTR_M44F):
        {
            EXR_TYPE(attr_m44f) nil = {0};
            *(nattr->m44f) = nil;
            break;
        }
        case EXR_DEF(ATTR_M44D):
        {
            EXR_TYPE(attr_m44f) nil = {0};
            *(nattr->m44f) = nil;
            break;
        }
        case EXR_DEF(ATTR_PREVIEW):
        {
            EXR_TYPE(attr_preview) nil = {0};
            *(nattr->preview) = nil;
            break;
        }
        case EXR_DEF(ATTR_RATIONAL):
        {
            EXR_TYPE(attr_rational) nil = {0};
            *(nattr->rational) = nil;
            break;
        }
        case EXR_DEF(ATTR_STRING):
        {
            EXR_TYPE(attr_string) nil = {0};
            *(nattr->string) = nil;
            break;
        }
        case EXR_DEF(ATTR_STRING_VECTOR):
        {
            EXR_TYPE(attr_string_vector) nil = {0};
            *(nattr->stringvector) = nil;
            break;
        }
        case EXR_DEF(ATTR_TILEDESC):
        {
            EXR_TYPE(attr_tiledesc) nil = {0};
            *(nattr->tiledesc) = nil;
            break;
        }
        case EXR_DEF(ATTR_TIMECODE):
        {
            EXR_TYPE(attr_timecode) nil = {0};
            *(nattr->timecode) = nil;
            break;
        }
        case EXR_DEF(ATTR_V2I):
        {
            EXR_TYPE(attr_v2i) nil = {0};
            *(nattr->v2i) = nil;
            break;
        }
        case EXR_DEF(ATTR_V2F):
        {
            EXR_TYPE(attr_v2f) nil = {0};
            *(nattr->v2f) = nil;
            break;
        }
        case EXR_DEF(ATTR_V2D):
        {
            EXR_TYPE(attr_v2d) nil = {0};
            *(nattr->v2d) = nil;
            break;
        }
        case EXR_DEF(ATTR_V3I):
        {
            EXR_TYPE(attr_v3i) nil = {0};
            *(nattr->v3i) = nil;
            break;
        }
        case EXR_DEF(ATTR_V3F):
        {
            EXR_TYPE(attr_v3f) nil = {0};
            *(nattr->v3f) = nil;
            break;
        }
        case EXR_DEF(ATTR_V3D):
        {
            EXR_TYPE(attr_v3d) nil = {0};
            *(nattr->v3d) = nil;
            break;
        }
        case EXR_DEF(ATTR_OPAQUE):
        {
            EXR_TYPE(attr_opaquedata) nil = {0};
            *(nattr->opaque) = nil;
            break;
        }
        case EXR_DEF(ATTR_UNKNOWN):
        case EXR_DEF(ATTR_LAST_KNOWN_TYPE):
        default:
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ARGUMENT),
                    "Invalid / unimplemented type (%s) in attr_init",
                    nattr->type_name );
            return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int attr_destroy( EXR_TYPE(attribute) *attr )
{
    switch ( attr->type )
    {
        case EXR_DEF(ATTR_CHLIST):
            EXR_FUN(attr_chlist_destroy)( attr->chlist );
            break;
        case EXR_DEF(ATTR_FLOAT_VECTOR):
            EXR_FUN(attr_float_vector_destroy)( attr->floatvector );
            break;
        case EXR_DEF(ATTR_PREVIEW):
            EXR_FUN(attr_preview_destroy)( attr->preview );
            break;
        case EXR_DEF(ATTR_STRING):
            EXR_FUN(attr_string_destroy)( attr->string );
            break;
        case EXR_DEF(ATTR_STRING_VECTOR):
            EXR_FUN(attr_string_vector_destroy)( attr->stringvector );
            break;
        case EXR_DEF(ATTR_OPAQUE):
            EXR_FUN(attr_opaquedata_destroy)( attr->opaque );
            break;
        case EXR_DEF(ATTR_BOX2I):
        case EXR_DEF(ATTR_BOX2F):
        case EXR_DEF(ATTR_CHROMATICITIES):
        case EXR_DEF(ATTR_COMPRESSION):
        case EXR_DEF(ATTR_ENVMAP):
        case EXR_DEF(ATTR_LINEORDER):
        case EXR_DEF(ATTR_DOUBLE):
        case EXR_DEF(ATTR_FLOAT):
        case EXR_DEF(ATTR_INT):
        case EXR_DEF(ATTR_KEYCODE):
        case EXR_DEF(ATTR_M33F):
        case EXR_DEF(ATTR_M33D):
        case EXR_DEF(ATTR_M44F):
        case EXR_DEF(ATTR_M44D):
        case EXR_DEF(ATTR_RATIONAL):
        case EXR_DEF(ATTR_TILEDESC):
        case EXR_DEF(ATTR_TIMECODE):
        case EXR_DEF(ATTR_V2I):
        case EXR_DEF(ATTR_V2F):
        case EXR_DEF(ATTR_V2D):
        case EXR_DEF(ATTR_V3I):
        case EXR_DEF(ATTR_V3F):
        case EXR_DEF(ATTR_V3D):
        case EXR_DEF(ATTR_UNKNOWN):
        case EXR_DEF(ATTR_LAST_KNOWN_TYPE):
        default:
            break;
    }
    /* we don't care about the string because they were built into the
     * allocation block of the attribute as necessary */
    priv_free( attr );
    return 0;
}

/**************************************/

void EXR_FUN(attr_list_destroy)(
    EXR_TYPE(attribute_list) *list )
{
    EXR_TYPE(attribute_list) nil = {0};

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

uint64_t EXR_FUN(attr_list_compute_size)(
    EXR_TYPE(attribute_list) *list )
{
    uint64_t retval = 0;

    if ( list )
    {
        for ( int i = 0; i < list->num_attributes; ++i )
        {
            EXR_TYPE(attribute) *cur = list->entries[i];
            retval += (size_t)cur->name_length + 1;
            retval += (size_t)cur->type_name_length + 1;
            retval += sizeof(int32_t);
            switch ( cur->type )
            {
                case EXR_DEF(ATTR_BOX2I):
                    retval += sizeof(*(cur->box2i));
                    break;
                case EXR_DEF(ATTR_BOX2F):
                    retval += sizeof(*(cur->box2f));
                    break;
                case EXR_DEF(ATTR_CHLIST):
                    for ( int c = 0; c < cur->chlist->num_channels; ++c )
                    {
                        retval += cur->chlist->entries[c].name.length + 1;
                        retval += sizeof(int32_t) * 4;
                    }
                    break;
                case EXR_DEF(ATTR_CHROMATICITIES):
                    retval += sizeof(*(cur->chromaticities));
                    break;
                case EXR_DEF(ATTR_COMPRESSION):
                case EXR_DEF(ATTR_ENVMAP):
                case EXR_DEF(ATTR_LINEORDER):
                    retval += sizeof(uint8_t);
                    break;
                case EXR_DEF(ATTR_DOUBLE):
                    retval += sizeof(double);
                    break;
                case EXR_DEF(ATTR_FLOAT):
                    retval += sizeof(float);
                    break;
                case EXR_DEF(ATTR_FLOAT_VECTOR):
                    retval += sizeof(float) * cur->floatvector->length;
                    break;
                case EXR_DEF(ATTR_INT):
                    retval += sizeof(int32_t);
                    break;
                case EXR_DEF(ATTR_KEYCODE):
                    retval += sizeof(*(cur->keycode));
                    break;
                case EXR_DEF(ATTR_M33F):
                    retval += sizeof(*(cur->m33f));
                    break;
                case EXR_DEF(ATTR_M33D):
                    retval += sizeof(*(cur->m33d));
                    break;
                case EXR_DEF(ATTR_M44F):
                    retval += sizeof(*(cur->m44f));
                    break;
                case EXR_DEF(ATTR_M44D):
                    retval += sizeof(*(cur->m44d));
                    break;
                case EXR_DEF(ATTR_PREVIEW):
                    retval += (size_t)cur->preview->width * (size_t)cur->preview->height * (size_t)4;
                    break;
                case EXR_DEF(ATTR_RATIONAL):
                    retval += sizeof(*(cur->rational));
                    break;
                case EXR_DEF(ATTR_STRING):
                    retval += (size_t)cur->string->length;
                    break;
                case EXR_DEF(ATTR_STRING_VECTOR):
                    for ( int s = 0; s < cur->stringvector->n_strings; ++s )
                    {
                        retval += cur->stringvector->strings[s].length;
                        retval += sizeof(int32_t);
                    }
                    break;
                case EXR_DEF(ATTR_TILEDESC):
                    retval += sizeof(*(cur->tiledesc));
                    break;
                case EXR_DEF(ATTR_TIMECODE):
                    retval += sizeof(*(cur->timecode));
                    break;
                case EXR_DEF(ATTR_V2I):
                    retval += sizeof(*(cur->v2i));
                    break;
                case EXR_DEF(ATTR_V2F):
                    retval += sizeof(*(cur->v2f));
                    break;
                case EXR_DEF(ATTR_V2D):
                    retval += sizeof(*(cur->v2d));
                    break;
                case EXR_DEF(ATTR_V3I):
                    retval += sizeof(*(cur->v3i));
                    break;
                case EXR_DEF(ATTR_V3F):
                    retval += sizeof(*(cur->v3f));
                    break;
                case EXR_DEF(ATTR_V3D):
                    retval += sizeof(*(cur->v3d));
                    break;
                case EXR_DEF(ATTR_OPAQUE):
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
                case EXR_DEF(ATTR_UNKNOWN):
                case EXR_DEF(ATTR_LAST_KNOWN_TYPE):
                default:
                    return (uint64_t)-1;
                    break;
            }
        }
    }

    return retval;
}

/**************************************/

EXR_TYPE(attribute) *EXR_FUN(attr_list_find_by_name)(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    const char *name )
{
    EXR_TYPE(attribute) **it = NULL;
    EXR_TYPE(attribute) **first = NULL;
    EXR_TYPE(attribute) **end = NULL;
    int step, count, cmp;

    if ( ! name || name[0] == '\0' )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid name passed to find_by_name" );
        return NULL;
    }

    if ( ! list )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
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
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    EXR_TYPE(attribute) *nattr,
    const char *name )
{
    int cattrsz = list->num_attributes;
    int nattrsz = cattrsz + 1;
    int insertpos;
    EXR_TYPE(attribute) **attrs = list->entries;
    EXR_TYPE(attribute) **sorted_attrs = list->sorted_entries;

    if ( nattrsz > list->num_alloced )
    {
        size_t nsize = list->num_alloced * 2;
        if ( nattrsz > nsize )
            nsize = nattrsz + 1;
        attrs = (EXR_TYPE(attribute) **)priv_alloc( sizeof(EXR_TYPE(attribute) *) * nsize * 2 );
        if ( ! attrs )
        {
            priv_free( nattr );
            if ( f )
                return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_OUT_OF_MEMORY) );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }

        list->num_alloced = nsize;
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
        EXR_TYPE(attribute) *prev = sorted_attrs[insertpos];

        if ( strcmp( nattr->name, prev->name ) >= 0 )
            break;

        sorted_attrs[insertpos + 1] = prev;
        sorted_attrs[insertpos] = nattr;
        --insertpos;
    }

    list->num_attributes = nattrsz;
    attr_init( f, nattr );

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int validate_attr_arguments(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    const char *name,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr )
{
    EXR_TYPE(attribute) *nattr = NULL;
    if ( ! list )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid list pointer to attr_list_add" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! attr )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid output attribute pointer location to attr_list_add" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *attr = NULL;

    if ( data_len < 0 )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Extra data storage requested negative length (%d)", data_len );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    else if ( data_len > 0 && ! data_ptr )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Extra data storage output pointer must be provided when requesting extra data (%d)", data_len );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    else if ( data_ptr )
        *data_ptr = NULL;

    if ( ! name || name[0] == '\0' )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid name to add_by_type" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    /* is it already in the list? */
    nattr = EXR_FUN(attr_list_find_by_name)( f, list, name );
    if ( nattr )
    {
        if ( data_ptr && data_len > 0 )
        {
            if ( f )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ARGUMENT),
                    "Entry '%s' (type %s) already in list but requesting additional data",
                    name, nattr->type_name );
            return EXR_DEF(ERR_INVALID_ARGUMENT);
        }

        *attr = nattr;
        return -1;
    }
        
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static void check_attr_handler(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute) *attr )
{
    if ( f && attr->type == EXR_DEF(ATTR_OPAQUE) )
    {
        EXR_TYPE(attribute) *handler = EXR_FUN(attr_list_find_by_name)(
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

int EXR_FUN(attr_list_add_by_type)(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    const char *name,
    const char *type,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr )
{
    int rval = EXR_DEF(ERR_INVALID_ARGUMENT);
    const EXR_TYPE(PRIV_ATTR_MAP) *known = NULL;
    int32_t nlen, tlen, mlen = EXR_SHORTNAME_MAXLEN;
    size_t attrblocksz = sizeof(EXR_TYPE(attribute));
    uint8_t *ptr = NULL;
    EXR_TYPE(attribute) *nattr = NULL;
    EXR_TYPE(attribute) nil = {0};

    if ( ! type || type[0] == '\0' )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid type to add_by_type" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    rval = validate_attr_arguments( f, list, name, data_len, data_ptr, attr );
    if ( rval != EXR_DEF(ERR_SUCCESS) )
    {
        if ( rval < 0 )
        {
            if ( 0 != strcmp( type, (*attr)->type_name ) )
            {
                nattr = *attr;
                *attr = NULL;
                if ( f )
                    return EXR_GETFILE(f)->print_error(
                        f, EXR_DEF(ERR_INVALID_ARGUMENT),
                        "Entry '%s' already in list but with different type ('%s' vs requested '%s')",
                        name, nattr->type_name, type );
                return EXR_DEF(ERR_INVALID_ARGUMENT);
            }
            return EXR_DEF(ERR_SUCCESS);
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
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Provided name '%s' too long for file (len %d, max %d)",
                name, nlen, mlen );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    tlen = (int32_t)strlen( type );
    if ( tlen > mlen )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Provided type name '%s' too long for file (len %d, max %d)",
                type, nlen, mlen );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
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
                return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_OUT_OF_MEMORY) );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }
        nattr = (EXR_TYPE(attribute) *)ptr;
        *nattr = nil;
        ptr += sizeof(EXR_TYPE(attribute));

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
        attrblocksz += sizeof(EXR_TYPE(attr_opaquedata));
        attrblocksz += data_len;
        ptr = (uint8_t *)priv_alloc( attrblocksz );
        if ( ! ptr )
        {
            if ( f )
                return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_OUT_OF_MEMORY) );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }
        nattr = (EXR_TYPE(attribute) *)ptr;
        *nattr = nil;
        ptr += sizeof(EXR_TYPE(attribute));

        memcpy( ptr, name, nlen + 1 );
        nattr->name = (char *)ptr;
        ptr += nlen + 1;

        memcpy( ptr, type, tlen + 1 );
        nattr->type_name = (char *)ptr;
        ptr += tlen + 1;

        nattr->name_length = (uint8_t)nlen;
        nattr->type_name_length = (uint8_t)tlen;
        nattr->type = EXR_DEF(ATTR_OPAQUE);
        nattr->opaque = (EXR_TYPE(attr_opaquedata) *)ptr;
        ptr += sizeof(EXR_TYPE(attr_opaquedata));
    }
    rval = add_to_list( f, list, nattr, name );
    if ( rval == EXR_DEF(ERR_SUCCESS) )
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

int EXR_FUN(attr_list_add)(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    const char *name,
    EXR_TYPE(ATTRIBUTE_TYPE) type,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr )
{
    int rval = EXR_DEF(ERR_INVALID_ARGUMENT);
    const EXR_TYPE(PRIV_ATTR_MAP) *known = NULL;
    int32_t nlen, tidx, mlen = 255;
    size_t attrblocksz = sizeof(EXR_TYPE(attribute));
    uint8_t *ptr = NULL;
    EXR_TYPE(attribute) *nattr = NULL;
    EXR_TYPE(attribute) nil = {0};

    rval = validate_attr_arguments( f, list, name, data_len, data_ptr, attr );
    if ( rval != EXR_DEF(ERR_SUCCESS) )
    {
        if ( rval < 0 )
        {
            if ( (*attr)->type != type )
            {
                nattr = *attr;
                *attr = NULL;
                if ( f )
                    return EXR_GETFILE(f)->print_error(
                        f, EXR_DEF(ERR_INVALID_ARGUMENT),
                        "Entry '%s' already in list but with different type ('%s')",
                        name, nattr->type_name );
                return EXR_DEF(ERR_INVALID_ARGUMENT);
            }
            return EXR_DEF(ERR_SUCCESS);
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
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Provided name '%s' too long for file (len %d, max %d)",
                name, nlen, mlen );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    tidx = ((int)type) - 1;
    if ( tidx < 0 || tidx >= the_predefined_attr_count )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid type enum for '%s' in create by builtin type (type %d)",
                name, (int)type );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    known = &(the_predefined_attr_typenames[tidx]);

    attrblocksz += nlen + 1;
    attrblocksz += known->exp_size;
    attrblocksz += data_len;
    ptr = (uint8_t *)priv_alloc( attrblocksz );
    if ( ! ptr )
    {
        if ( f )
            return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_OUT_OF_MEMORY) );
        return EXR_DEF(ERR_OUT_OF_MEMORY);
    }

    nattr = (EXR_TYPE(attribute) *)ptr;
    *nattr = nil;

    ptr += sizeof(EXR_TYPE(attribute));
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

int EXR_FUN(attr_list_add_static_name)(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    const char *name,
    EXR_TYPE(ATTRIBUTE_TYPE) type,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr )
{
    int rval = EXR_DEF(ERR_INVALID_ARGUMENT);
    const EXR_TYPE(PRIV_ATTR_MAP) *known = NULL;
    int32_t nlen, tidx, mlen = EXR_SHORTNAME_MAXLEN;
    size_t attrblocksz = sizeof(EXR_TYPE(attribute));
    uint8_t *ptr = NULL;
    EXR_TYPE(attribute) *nattr = NULL;
    EXR_TYPE(attribute) nil = {0};

    rval = validate_attr_arguments( f, list, name, data_len, data_ptr, attr );
    if ( rval != EXR_DEF(ERR_SUCCESS) )
    {
        if ( rval < 0 )
        {
            if ( (*attr)->type != type )
            {
                nattr = *attr;
                *attr = NULL;
                if ( f )
                    return EXR_GETFILE(f)->print_error(
                        f, EXR_DEF(ERR_INVALID_ARGUMENT),
                        "Entry '%s' already in list but with different type ('%s')",
                        name, nattr->type_name );
                return EXR_DEF(ERR_INVALID_ARGUMENT);
            }
            return EXR_DEF(ERR_SUCCESS);
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
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Provided name '%s' too long for file (len %d, max %d)",
                name, nlen, mlen );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    tidx = ((int)type) - 1;
    if ( tidx < 0 || tidx >= the_predefined_attr_count )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid type enum for '%s' in create by builtin type (type %d)",
                name, (int)type );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    known = &(the_predefined_attr_typenames[tidx]);

    attrblocksz += known->exp_size;
    attrblocksz += data_len;
    ptr = (uint8_t *)priv_alloc( attrblocksz );
    if ( ! ptr )
        return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_OUT_OF_MEMORY) );
    nattr = (EXR_TYPE(attribute) *)ptr;
    *nattr = nil;
    ptr += sizeof(EXR_TYPE(attribute));
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
    if ( rval == EXR_DEF(ERR_SUCCESS) )
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

int EXR_FUN(attr_list_remove)(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(attribute_list) *list,
    EXR_TYPE(attribute) *attr )
{
    int cattrsz, attridx = -1;
    EXR_TYPE(attribute) **attrs;

    if ( ! attr )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "NULL attribute passed to remove" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! list )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid list pointer to remove attribute" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
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
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Attribute not in list" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
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

    return EXR_DEF(ERR_SUCCESS);
}
