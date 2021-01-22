/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_base.h"

#include "openexr_attr.h"
#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

#include <stdio.h>
#include <string.h>

/**************************************/

void exr_get_library_version( int *maj, int *min, int *patch, const char **extra )
{
    if ( maj )
        *maj = OPENEXR_VERSION_MAJOR;
    if ( min )
        *min = OPENEXR_VERSION_MINOR;
    if ( patch )
        *patch = OPENEXR_VERSION_PATCH;
#ifdef OPENEXR_VERSION_EXTRA
    if ( extra )
        *extra = OPENEXR_VERSION_EXTRA;
#else
    if ( extra )
        *extra = "";
#endif
}

/**************************************/

static const char *the_default_errors[] =
{
    "Success",
    "Unable to allocate memory",
    "Invalid argument to function",
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
static int the_default_error_count = sizeof(the_default_errors) / sizeof(const char *);

/**************************************/

const char *exr_get_default_error_message( int code )
{
    code = code < 0 ? -code : code;
    if ( code >= the_default_error_count )
        code = the_default_error_count - 1;
    return the_default_errors[code];
}

/**************************************/

static int sMaxW = 0;
static int sMaxH = 0;

void exr_set_maximum_image_size( int w, int h )
{
    if ( w >= 0 && h >= 0 )
    {
        sMaxW = w;
        sMaxH = h;
    }
}

/**************************************/

int exr_get_maximum_image_width()
{
    return sMaxW;
}

/**************************************/

int exr_get_maximum_image_height()
{
    return sMaxH;
}

/**************************************/

static int sTileMaxW = 0;
static int sTileMaxH = 0;
void exr_set_maximum_tile_size( int w, int h )
{
    if ( w >= 0 && h >= 0 )
    {
        sTileMaxW = w;
        sTileMaxH = h;
    }
}

/**************************************/

int exr_get_maximum_tile_width()
{
    return sTileMaxW;
}

/**************************************/

int exr_get_maximum_tile_height()
{
    return sTileMaxH;
}

/**************************************/

static void
print_attr( exr_attribute_t *a, int verbose )
{
    printf( "%s: ", a->name );
    if ( verbose )
        printf( "%s ", a->type_name );
	switch ( a->type )
	{
		case EXR_ATTR_BOX2I:
			printf( "[ %d, %d - %d %d ] %d x %d",
                    a->box2i->x_min, a->box2i->y_min, a->box2i->x_max, a->box2i->y_max,
                    a->box2i->x_max - a->box2i->x_min + 1,
                    a->box2i->y_max - a->box2i->y_min + 1 );
			break;
		case EXR_ATTR_BOX2F:
			printf( "[ %g, %g - %g %g ]",
                    a->box2f->x_min, a->box2f->y_min, a->box2f->x_max, a->box2f->y_max );
			break;
		case EXR_ATTR_CHLIST:
			printf( "%d channels\n", a->chlist->num_channels );
            for ( int c = 0; c < a->chlist->num_channels; ++c )
            {
                if ( c > 0 )
                    printf( "\n" );
                printf( "   '%s': %s samp %d %d", a->chlist->entries[c].name.str,
                        ( a->chlist->entries[c].pixel_type == EXR_PIXEL_UINT ? "uint" :
                          ( a->chlist->entries[c].pixel_type == EXR_PIXEL_HALF ? "half" :
                            a->chlist->entries[c].pixel_type == EXR_PIXEL_FLOAT ? "float" :
                            "<UNKNOWN>" ) ),
                        a->chlist->entries[c].x_sampling,
                        a->chlist->entries[c].y_sampling );
            }
			break;
		case EXR_ATTR_CHROMATICITIES:
			printf( "r[%g, %g] g[%g, %g] b[%g, %g] w[%g, %g]",
					a->chromaticities->red_x, a->chromaticities->red_y,
					a->chromaticities->green_x, a->chromaticities->green_y,
					a->chromaticities->blue_x, a->chromaticities->blue_y,
					a->chromaticities->white_x, a->chromaticities->white_y );
			break;
		case EXR_ATTR_COMPRESSION:
        {
            static char *compressionnames[] = {
                "none",
                "rle",
                "zips",
                "zip",
                "piz",
                "pxr24",
                "b44",
                "b44a",
                "dwaa",
                "dwab"
                };
			printf( "'%s'", (a->uc < 10 ? compressionnames[a->uc] : "<UNKNOWN>") );
            if ( verbose )
                printf( " (0x%02X)", a->uc );
			break;
        }
		case EXR_ATTR_DOUBLE:
			printf( "%g", a->d );
			break;
		case EXR_ATTR_ENVMAP:
			printf( "%s", a->uc == 0 ? "latlong" : "cube" );
			break;
		case EXR_ATTR_FLOAT:
			printf( "%g", a->f );
			break;
		case EXR_ATTR_FLOAT_VECTOR:
			printf( "[%d entries]:\n   ", a->floatvector->length );
            for ( int i = 0; i < a->floatvector->length; ++i )
                printf( " %g", a->floatvector->arr[i] );
			break;
		case EXR_ATTR_INT:
			printf( "%d", a->i );
			break;
		case EXR_ATTR_KEYCODE:
			printf( "mfgc %d film %d prefix %d count %d perf_off %d ppf %d ppc %d",
					a->keycode->film_mfc_code, a->keycode->film_type, a->keycode->prefix,
					a->keycode->count, a->keycode->perf_offset, a->keycode->perfs_per_frame,
					a->keycode->perfs_per_count );
			break;
		case EXR_ATTR_LINEORDER:
			printf( "%d (%s)", (int)a->uc,
					a->uc == EXR_LINEORDER_INCREASING_Y ? "increasing" :
					( a->uc == EXR_LINEORDER_DECREASING_Y ? "decreasing" :
					  ( a->uc == EXR_LINEORDER_RANDOM_Y ? "random" : "<UNKNOWN>" ) ) );
			break;
		case EXR_ATTR_M33F:
			printf( "[ [%g %g %g] [%g %g %g] [%g %g %g] ]",
					a->m33f->m[0], a->m33f->m[1], a->m33f->m[2],
					a->m33f->m[3], a->m33f->m[4], a->m33f->m[5],
					a->m33f->m[6], a->m33f->m[7], a->m33f->m[8] );
			break;
		case EXR_ATTR_M33D:
			printf( "[ [%g %g %g] [%g %g %g] [%g %g %g] ]",
					a->m33d->m[0], a->m33d->m[1], a->m33f->m[2],
					a->m33d->m[3], a->m33d->m[4], a->m33f->m[5],
					a->m33d->m[6], a->m33d->m[7], a->m33f->m[8] );
			break;
		case EXR_ATTR_M44F:
			printf( "[ [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] ]",
					a->m44f->m[0], a->m44f->m[1], a->m44f->m[2], a->m44f->m[3],
					a->m44f->m[4], a->m44f->m[5], a->m44f->m[6], a->m44f->m[7],
					a->m44f->m[8], a->m44f->m[9], a->m44f->m[10], a->m44f->m[11],
					a->m44f->m[12], a->m44f->m[13], a->m44f->m[14], a->m44f->m[15] );
			break;
		case EXR_ATTR_M44D:
			printf( "[ [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] ]",
					a->m44d->m[0], a->m44d->m[1], a->m44d->m[2], a->m44d->m[3],
					a->m44d->m[4], a->m44d->m[5], a->m44d->m[6], a->m44d->m[7],
					a->m44d->m[8], a->m44d->m[9], a->m44d->m[10], a->m44d->m[11],
					a->m44d->m[12], a->m44d->m[13], a->m44d->m[14], a->m44d->m[15] );
			break;
		case EXR_ATTR_PREVIEW:
			printf( "%u x %u", a->preview->width, a->preview->height );
			break;
		case EXR_ATTR_RATIONAL:
			printf( "%d / %u", a->rational->num, a->rational->denom );
			if ( a->rational->denom != 0 )
				printf( " (%g)", (double)( a->rational->num ) / (double)( a->rational->denom ) );
			break;
		case EXR_ATTR_STRING:
			printf( "'%s'", a->string->str ? a->string->str : "<NULL>" );
			break;
		case EXR_ATTR_STRING_VECTOR:
			printf( "[%d entries]:\n", a->stringvector->n_strings );
            for ( int i = 0; i < a->stringvector->n_strings; ++i )
            {
                if ( i > 0 )
                    printf( "\n" );
                printf( "    '%s'", a->stringvector->strings[i].str );
            }
			break;
		case EXR_ATTR_TILEDESC:
		{
			static const char *lvlModes[] = { "single image", "mipmap", "ripmap" };
			uint8_t lvlMode = EXR_GET_TILE_LEVEL_MODE( *(a->tiledesc) );
			uint8_t rndMode = EXR_GET_TILE_ROUND_MODE( *(a->tiledesc) );
			printf( "size %u x %u level %u (%s) round %u (%s)",
					a->tiledesc->x_size, a->tiledesc->y_size,
					lvlMode, lvlMode < 3 ? lvlModes[lvlMode] : "<UNKNOWN>",
					rndMode, rndMode == 0 ? "down" : "up" );
			break;
		}
		case EXR_ATTR_TIMECODE:
			printf( "time %u user %u", a->timecode->time_and_flags, a->timecode->user_data );
			break;
		case EXR_ATTR_V2I:
			printf( "[ %d, %d ]", a->v2i->x, a->v2i->y );
			break;
		case EXR_ATTR_V2F:
			printf( "[ %g, %g ]", a->v2f->x, a->v2f->y );
			break;
		case EXR_ATTR_V2D:
			printf( "[ %g, %g ]", a->v2d->x, a->v2d->y );
			break;
		case EXR_ATTR_V3I:
			printf( "[ %d, %d, %d ]", a->v3i->x, a->v3i->y, a->v3i->z );
			break;
		case EXR_ATTR_V3F:
			printf( "[ %g, %g, %g ]", a->v3f->x, a->v3f->y, a->v3f->z );
			break;
		case EXR_ATTR_V3D:
			printf( "[ %g, %g, %g ]", a->v3d->x, a->v3d->y, a->v3d->z );
			break;
		case EXR_ATTR_OPAQUE:
			printf( "(size %d unp size %d hdlrs %p %p %p)",
                    a->opaque->size, a->opaque->unpacked_size,
                    a->opaque->unpack_func_ptr,
                    a->opaque->pack_func_ptr,
                    a->opaque->destroy_func_ptr );
			break;
		case EXR_ATTR_UNKNOWN:
		default:
			printf( "<ERROR Unknown type '%s'>", a->type_name );
			break;
	}
}

/**************************************/

void exr_print_info( exr_file_t *file, int verbose )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    if ( f )
    {
        if ( verbose )
        {
            printf( "File '%s': ver %d flags%s%s%s%s\n",
                    f->filename.str, (int)f->version,
                    f->is_singlepart_tiled ? " singletile" : "",
                    f->max_name_length == EXR_LONGNAME_MAXLEN ? " longnames" : " shortnames",
                    f->has_nonimage_data ? " deep" : "",
                    f->is_multipart ? " multipart" : "" );
            printf( " parts: %d\n", f->num_parts );
        }
        else
        {
            printf( "File '%s':\n", f->filename.str );
        }

        for ( int partidx = 0; partidx < f->num_parts; ++partidx )
        {
            exr_PRIV_PART_t *curpart = f->parts[partidx];
            if ( verbose || f->is_multipart || curpart->name )
                printf( " part %d: %s\n", partidx + 1, curpart->name ? curpart->name->string->str : "<single>" );
            if ( verbose )
            {
                for ( int a = 0; a < curpart->attributes.num_attributes; ++a )
                {
                    if ( a > 0 )
                        printf( "\n" );
                    printf( "  " );
                    print_attr( curpart->attributes.entries[a], verbose );
                }
                printf( "\n" );
            }
            else
            {
                if ( curpart->type )
                {
                    printf( "  " );
                    print_attr( curpart->type, verbose );
                }
                printf( "  " );
                print_attr( curpart->compression, verbose );
                if ( curpart->tiles )
                {
                    printf( "\n  " );
                    print_attr( curpart->tiles, verbose );
                }
                printf( "\n  " );
                print_attr( curpart->displayWindow, verbose );
                printf( "\n  " );
                print_attr( curpart->dataWindow, verbose );
                printf( "\n  " );
                print_attr( curpart->channels, verbose );
                printf( "\n" );
            }
            if ( curpart->tiles )
            {
                printf( "  tiled image has levels: x %d y %d\n",
                        curpart->num_tile_levels_x,
                        curpart->num_tile_levels_y );
                printf( "    x tile count:" );
                for ( int l = 0; l < curpart->num_tile_levels_x; ++l )
                    printf( " %d (sz %d)", curpart->tile_level_tile_count_x[l],
                            curpart->tile_level_tile_size_x[l] );
                printf( "\n    y tile count:" );
                for ( int l = 0; l < curpart->num_tile_levels_y; ++l )
                    printf( " %d (sz %d)", curpart->tile_level_tile_count_y[l],
                            curpart->tile_level_tile_size_y[l] );
                printf( "\n" );
            }
        }
    }
    else
        printf( "ERROR: NULL file handle\n" );
}

