// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include "general_attr.h"

#include <openexr.h>

#include <assert.h>
#include <limits.h>
#include <string.h>

void testAttrSizes( const std::string &tempdir )
{
    assert( sizeof(EXR_TYPE(attr_box2i)) == (4*4) );
    assert( sizeof(EXR_TYPE(attr_box2f)) == (4*4) );
    assert( sizeof(EXR_TYPE(attr_chromaticities)) == (8*4) );
    assert( sizeof(EXR_TYPE(attr_keycode)) == (7*4) );
    assert( sizeof(EXR_TYPE(attr_m33f)) == (9*4) );
    assert( sizeof(EXR_TYPE(attr_m33d)) == (9*8) );
    assert( sizeof(EXR_TYPE(attr_m44f)) == (16*4) );
    assert( sizeof(EXR_TYPE(attr_m44d)) == (16*8) );
    assert( sizeof(EXR_TYPE(attr_rational)) == (2*4) );
    assert( sizeof(EXR_TYPE(attr_tiledesc)) == (2*4+1) );
    assert( sizeof(EXR_TYPE(attr_timecode)) == (2*4) );
    assert( sizeof(EXR_TYPE(attr_v2i)) == (2*4) );
    assert( sizeof(EXR_TYPE(attr_v2f)) == (2*4) );
    assert( sizeof(EXR_TYPE(attr_v2d)) == (2*8) );
    assert( sizeof(EXR_TYPE(attr_v3i)) == (3*4) );
    assert( sizeof(EXR_TYPE(attr_v3f)) == (3*4) );
    assert( sizeof(EXR_TYPE(attr_v3d)) == (3*8) );
}

static void testStringHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_string) s, nil = {0};

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init)( f, NULL, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init)( f, &s, -1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init_static)( f, NULL, "exr") );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init_static)( f, &s, NULL) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init_static_with_length)( f, NULL, "exr", 3) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init_static_with_length)( f, &s, NULL, 4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_init_static_with_length)( f, &s, "exr", -3) );
    EXR_FUN(attr_string_destroy)( NULL );
    EXR_FUN(attr_string_destroy)( &nil );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_init)( f, &s, 4) );
    assert(s.str != NULL);
    assert(s.length == 4);
    assert(s.alloc_size == 5);
    EXR_FUN(attr_string_destroy)( &s );
    assert(s.str == NULL);
    assert(s.length == 0);
    assert(s.alloc_size == 0);
    
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_create)( f, &s, NULL) );
    assert(s.str != NULL && s.str[0] == '\0');
    assert(s.length == 0);
    assert(s.alloc_size == 1);
    EXR_FUN(attr_string_destroy)( &s );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_create)( f, &s, "exr") );
    assert(s.str != NULL && !strcmp(s.str, "exr"));
    assert(s.length == 3);
    assert(s.alloc_size == 4);

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_set)( f, &s, "openexr") );
    assert(s.str != NULL && !strcmp(s.str, "openexr"));
    assert(s.length == 7);
    assert(s.alloc_size == 8);
    
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_set_with_length)( f, &s, "exr", 3) );
    assert(s.str != NULL && !strcmp(s.str, "exr"));
    assert(s.length == 3);
    assert(s.alloc_size == 8);
    
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_set_with_length)( f, &s, "exropenexr", 3) );
    assert(s.str != NULL && !strcmp(s.str, "exr"));
    assert(s.length == 3);
    assert(s.alloc_size == 8);
    
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_set_with_length)( f, &s, NULL, 3) );
    assert(s.str != NULL && s.str[0] == '\0' && s.str[1] == '\0' && s.str[2] == '\0');
    assert(s.length == 3);
    assert(s.str[s.length] == '\0');
    assert(s.alloc_size == 8);
    
    EXR_FUN(attr_string_destroy)( &s );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_create_with_length)( f, &s, "exr", 6) );
    assert(s.str != NULL && !strcmp(s.str, "exr"));
    assert(s.length == 6);
    assert(s.alloc_size == 7);
    EXR_FUN(attr_string_destroy)( &s );
    
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_create_with_length)( f, &s, "openexr", 3) );
    assert(s.str != NULL && !strcmp(s.str, "ope"));
    assert(s.length == 3);
    assert(s.alloc_size == 4);
    EXR_FUN(attr_string_destroy)( &s );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_init_static)( f, &s, "exr") );
    assert(s.str != NULL && !strcmp(s.str, "exr"));
    assert(s.length == 3);
    assert(s.alloc_size == 0);
    EXR_FUN(attr_string_destroy)( &s );
    assert(s.str == NULL);
    assert(s.length == 0);
    assert(s.alloc_size == 0);

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_init_static_with_length)( f, &s, "openexr", 7) );
    assert(s.str != NULL && !strcmp(s.str, "openexr"));
    assert(s.length == 7);
    assert(s.alloc_size == 0);
    EXR_FUN(attr_string_destroy)( &s );
    // make sure we can re-delete something?
    EXR_FUN(attr_string_destroy)( &s );
}

static EXR_TYPE(FILE) *createDummyFile( const char *test )
{
    EXR_TYPE(FILE) *f = NULL;;
    // we won't actually write to this and so don't need a proper
    // stream but need it to test a path with a valid file.
    if ( EXR_DEF(ERR_SUCCESS) != EXR_FUN(start_write_stream)(
             &f, test, NULL, NULL, NULL, NULL ) )
    {
        assert(false);
    }

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(add_part)(f, "dummy", EXR_DEF(STORAGE_SCANLINE)));

    return f;
}

void testAttrStrings( const std::string &tempdir )
{
    testStringHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<string>" );
    testStringHelper( f );
    EXR_FUN(close)( &f );
}

static void testStringVectorHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_string_vector) sv, nil = {0};
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init)( f, NULL, 4) );
    EXR_FUN(attr_string_vector_destroy)( NULL );
    EXR_FUN(attr_string_vector_destroy)( &nil );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init)( f, &sv, -4) );

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init_entry)( f, NULL, 0, 3) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init_entry)( f, &nil, 0, 3) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_vector_init)( f, &sv, 1) );
    assert(sv.n_strings == 1);
    assert(sv.alloc_size == 1);
    assert(sv.strings != NULL);

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init_entry)( f, &sv, -1, 3) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init_entry)( f, &sv, 0, -3) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_init_entry)( f, &sv, 1, 3) );

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_set_entry_with_length)( f, &sv, -1, NULL, -1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_set_entry_with_length)( f, &sv, 0, NULL, -1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_string_vector_set_entry_with_length)( f, &sv, 1, NULL, -1) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_vector_set_entry)( f, &sv, 0, NULL));
    assert(sv.strings[0].length == 0);
    assert(sv.strings[0].alloc_size == 1);
    assert(sv.strings[0].str[0] == '\0');
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_vector_set_entry)( f, &sv, 0, "exr"));
    assert(sv.strings[0].length == 3);
    assert(sv.strings[0].alloc_size == 4);
    assert(0 == strcmp(sv.strings[0].str, "exr"));

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_string_vector_add_entry)( f, &sv, "openexr"));
    assert(sv.n_strings == 2);
    assert(sv.alloc_size == 2);

    assert(sv.strings[0].length == 3);
    assert(sv.strings[0].alloc_size == 4);
    assert(0 == strcmp(sv.strings[0].str, "exr"));
    assert(sv.strings[1].length == 7);
    assert(sv.strings[1].alloc_size == 8);
    assert(0 == strcmp(sv.strings[1].str, "openexr"));

    EXR_FUN(attr_string_vector_destroy)( &sv );
    assert(sv.n_strings == 0);
    assert(sv.alloc_size == 0);
    assert(sv.strings == NULL);
    // make sure we can re-delete something?
    EXR_FUN(attr_string_vector_destroy)( &sv );
}

void testAttrStringVectors( const std::string &tempdir )
{
    testStringVectorHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<stringvector>" );
    testStringVectorHelper( f );
    EXR_FUN(close)( &f );
}

static void testFloatVectorHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_float_vector) fv, nil = {0};
    float fdata[] = 
        {
            1.f, 2.f, 3.f, 4.f
        };
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_init)( f, NULL, 4) );
    EXR_FUN(attr_float_vector_destroy)( NULL );
    EXR_FUN(attr_float_vector_destroy)( &nil );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_init)( f, &fv, -4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_create)( f, NULL, fdata, 4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_create)( f, &fv, NULL, 4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_create)( f, &fv, fdata, -4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_create)( f, &fv, fdata, INT32_MAX / 2) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_init_static)( f, NULL, fdata, 4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_init_static)( f, &fv, NULL, 4) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_float_vector_init_static)( f, &fv, fdata, -4) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_float_vector_init)( f, &fv, 4) );
    assert(fv.length == 4);
    assert(fv.alloc_size == 4);
    assert(fv.arr != NULL);
    EXR_FUN(attr_float_vector_destroy)( &fv );
    assert(fv.length == 0);
    assert(fv.alloc_size == 0);
    assert(fv.arr == NULL);

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_float_vector_create)( f, &fv, fdata, 4) );
    assert(fv.length == 4);
    assert(fv.alloc_size == 4);
    assert(fv.arr[0] == 1.f);
    assert(fv.arr[1] == 2.f);
    assert(fv.arr[2] == 3.f);
    assert(fv.arr[3] == 4.f);
    EXR_FUN(attr_float_vector_destroy)( &fv );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_float_vector_init_static)( f, &fv, fdata, 4) );
    assert(fv.length == 4);
    assert(fv.alloc_size == 0);
    assert(fv.arr == fdata);
    EXR_FUN(attr_float_vector_destroy)( &fv );
    // make sure we can re-delete something?
    EXR_FUN(attr_float_vector_destroy)( &fv );
}

void testAttrFloatVectors( const std::string &tempdir )
{
    testFloatVectorHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<floatvector>" );
    testFloatVectorHelper( f );
    EXR_FUN(close)( &f );
}

static void testChlistHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_chlist) cl = {0};

    EXR_FUN(attr_chlist_destroy)( NULL );
    EXR_FUN(attr_chlist_destroy)( &cl );

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, NULL, "foo", EXR_DEF(PIXEL_HALF), 1, 1, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_LAST_TYPE), 1, 1, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", (EXR_TYPE(PIXEL_TYPE))-1, 1, 1, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_HALF), 2, 1, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_HALF), 1, 0, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_HALF), 1, -1, 1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_HALF), 1, 1, 0) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_HALF), 1, 1, -1) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_chlist_add)( f, &cl, "foo", EXR_DEF(PIXEL_HALF), 1, 1, 2) );
    assert(cl.num_channels == 1);
    assert(0 == strcmp(cl.entries[0].name.str, "foo"));
    assert(cl.entries[0].pixel_type == EXR_DEF(PIXEL_HALF));
    assert(cl.entries[0].p_linear == EXR_DEF(PIXEL_HALF));
    assert(cl.entries[0].x_sampling == 1);
    assert(cl.entries[0].y_sampling == 2);
    EXR_FUN(attr_chlist_destroy)( &cl );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_chlist_add)( f, &cl, "R", EXR_DEF(PIXEL_HALF), 1, 1, 1) );
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_chlist_add)( f, &cl, "G", EXR_DEF(PIXEL_HALF), 1, 1, 1) );
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_chlist_add)( f, &cl, "B", EXR_DEF(PIXEL_HALF), 1, 1, 1) );
    assert(cl.num_channels == 3);
    assert(0 == strcmp(cl.entries[0].name.str, "B"));
    assert(0 == strcmp(cl.entries[1].name.str, "G"));
    assert(0 == strcmp(cl.entries[2].name.str, "R"));

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "B", EXR_DEF(PIXEL_HALF), 1, 1, 1) );
    assert(cl.num_channels == 3);

    /* without a file, max will be 31 */
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_chlist_add)( f, &cl, "reallongreallongreallonglongname", EXR_DEF(PIXEL_HALF), 1, 1, 1) );
    EXR_FUN(attr_chlist_destroy)( &cl );

    // make sure we can re-delete something?
    EXR_FUN(attr_chlist_destroy)( &cl );
}

void testAttrChlists( const std::string &tempdir )
{
    testChlistHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<chlist>" );
    testChlistHelper( f );
    EXR_FUN(close)( &f );
}

static void testPreviewHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_preview) p;
    uint8_t data1x1[] = 
        {
            0xDE, 0xAD, 0xBE, 0xEF
        };
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_preview_init)( f, NULL, 64, 64) );
    EXR_FUN(attr_preview_destroy)( NULL );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_preview_init)( f, &p, (uint32_t)-1, 64) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_preview_init)( f, &p, 64, (uint32_t)-1) );
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_preview_init)( f, &p, 1, 1) );
    assert(p.width == 1);
    assert(p.height == 1);
    assert(p.alloc_size == 4);
    assert(p.rgba != NULL);
    EXR_FUN(attr_preview_destroy)( &p );
    assert(p.width == 0);
    assert(p.height == 0);
    assert(p.alloc_size == 0);
    assert(p.rgba == NULL);

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_preview_create)( f, &p, 1, 1, data1x1) );
    assert(p.width == 1);
    assert(p.height == 1);
    assert(p.alloc_size == 4);
    assert(p.rgba != NULL);
    assert(p.rgba[0] == 0xDE);
    assert(p.rgba[1] == 0xAD);
    assert(p.rgba[2] == 0xBE);
    assert(p.rgba[3] == 0xEF);
    EXR_FUN(attr_preview_destroy)( &p );
    // make sure we can re-delete something?
    EXR_FUN(attr_preview_destroy)( &p );
}

void testAttrPreview( const std::string &tempdir )
{
    testPreviewHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<preview>" );
    testPreviewHelper( f );
    EXR_FUN(close)( &f );
}

static void testOpaqueHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_opaquedata) o;
    uint8_t data4[] = 
        {
            0xDE, 0xAD, 0xBE, 0xEF
        };
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_opaquedata_init)( NULL, NULL, 4) );
    EXR_FUN(attr_opaquedata_destroy)( NULL );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_opaquedata_init)( NULL, &o, (uint32_t)-1) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_opaquedata_init)( NULL, &o, (size_t)-1) );
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_opaquedata_init)( NULL, &o, 4) );
    assert(o.size == 4);
    assert(o.alloc_size == 4);
    assert(o.packed_data != NULL);
    EXR_FUN(attr_opaquedata_destroy)( &o );
    assert(o.size == 0);
    assert(o.alloc_size == 0);
    assert(o.packed_data == NULL);

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_opaquedata_create)( NULL, &o, 4, data4) );
    assert(o.size == 4);
    assert(o.alloc_size == 4);
    assert(o.packed_data != NULL);
    assert(0 == memcmp(o.packed_data, data4, 4));
    EXR_FUN(attr_opaquedata_destroy)( &o );
    // make sure we can re-delete something?
    EXR_FUN(attr_opaquedata_destroy)( &o );
}

void testAttrOpaque( const std::string &tempdir )
{
    testOpaqueHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<opaque>" );
    testOpaqueHelper( f );
    EXR_FUN(close)( &f );
}

static int test_unpack( const void *, int32_t, int32_t *, void ** )
{
    return 0;
}

static int test_pack( const void *, int32_t, int32_t *, void ** )
{
    return 0;
}

static void test_hdlr_destroy( void *, int32_t )
{
}

void testAttrHandler( const std::string &tempdir )
{
    // NULL file doesn't make sense for this one...
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(register_attr_handler)(
               NULL, "mytype", &test_unpack, &test_pack, &test_hdlr_destroy ) );

    EXR_TYPE(FILE) *f = createDummyFile( "<attr_handler>" );
    EXR_TYPE(attribute) *foo = NULL, *bar = NULL;
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_declare_by_type)( f, 0, "foo", "mytype", &foo ) );
    assert(foo != NULL);
    assert(foo->opaque->unpack_func_ptr == NULL);

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(register_attr_handler)(
               f, NULL, &test_unpack, &test_pack, &test_hdlr_destroy ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(register_attr_handler)(
               f, "", &test_unpack, &test_pack, &test_hdlr_destroy ) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(register_attr_handler)(
               f, "mytype", &test_unpack, &test_pack, &test_hdlr_destroy ) );
    assert(foo->opaque->unpack_func_ptr == &test_unpack);
    assert(foo->opaque->pack_func_ptr == &test_pack);
    assert(foo->opaque->destroy_func_ptr == &test_hdlr_destroy);

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_declare_by_type)( f, 0, "bar", "mytype", &bar ) );
    assert(bar != NULL);
    assert(bar->opaque->unpack_func_ptr == &test_unpack);
    assert(bar->opaque->pack_func_ptr == &test_pack);
    assert(bar->opaque->destroy_func_ptr == &test_hdlr_destroy);

    EXR_FUN(close)( &f );
}

static void testAttrListHelper( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attribute_list) al = {0};
    EXR_TYPE(attribute) *out;
    uint8_t *extra;

    EXR_FUN(attr_list_destroy)( NULL );
    EXR_FUN(attr_list_destroy)( &al );
    assert(NULL == EXR_FUN(attr_list_find_by_name)( f, &al, "exr" ) );
    assert(0 == EXR_FUN(attr_list_compute_size)( NULL ) );
    assert(0 == EXR_FUN(attr_list_compute_size)( &al ) );

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, NULL, "myattr", "mytype", 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, NULL, "mytype", 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "", "mytype", 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "myattr", NULL, 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "myattr", "", 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "myattr", "mytype", 0, NULL, NULL ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "myattr", "mytype", -1, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "myattr", "mytype", 1, NULL, &out ) );

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, NULL, "myattr", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, NULL, EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "myattr", EXR_DEF(ATTR_LAST_KNOWN_TYPE), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "myattr", EXR_DEF(ATTR_UNKNOWN), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "myattr", (EXR_TYPE(ATTRIBUTE_TYPE))-1, 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), 0, NULL, NULL ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), -1, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), 1, NULL, &out ) );

    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, NULL, "myattr", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, NULL, EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_LAST_KNOWN_TYPE), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_UNKNOWN), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", (EXR_TYPE(ATTRIBUTE_TYPE))-1, 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), 0, NULL, NULL ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), -1, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), 1, NULL, &out ) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_remove)( f, NULL, NULL ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_remove)( f, &al, NULL ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_remove)( f, NULL, out ) );
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_remove)( f, &al, out ) );
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_remove)( f, &al, out ) );
    assert( al.num_attributes == 0 );
    
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "myattr", EXR_DEF(ATTR_STRING), 42, &extra, &out ) );
    assert(extra != NULL);
    // by destroying the list here, if extra is leaking, valgrind will find something
    EXR_FUN(attr_list_destroy)( &al );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "a", "mytype", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_OPAQUE));
    assert(EXR_DEF(ERR_INVALID_ARGUMENT) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "a", "box2i", 0, NULL, &out ) );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "b", "box2i", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_BOX2I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "c", "box2f", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_BOX2F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "d", "chlist", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_CHLIST));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "e", "chromaticities", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_CHROMATICITIES));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "f", "compression", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_COMPRESSION));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "g", "double", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_DOUBLE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "h", "envmap", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_ENVMAP));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "i", "float", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_FLOAT));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "j", "floatvector", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_FLOAT_VECTOR));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "k", "int", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_INT));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "l", "keycode", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_KEYCODE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "m", "lineOrder", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_LINEORDER));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "n", "m33f", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M33F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "o", "m33d", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M33D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "p", "m44f", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M44F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "q", "m44d", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M44D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "r", "preview", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_PREVIEW));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "s", "rational", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_RATIONAL));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "t", "string", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_STRING));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "u", "stringvector", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_STRING_VECTOR));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "v", "tiledesc", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_TILEDESC));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "w", "timecode", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_TIMECODE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "x", "v2i", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "y", "v2f", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "z", "v2d", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "0", "v3i", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "1", "v3f", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_by_type)(
               f, &al, "2", "v3d", 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3D));
    assert( al.num_attributes == 29 );

    EXR_FUN(attr_list_destroy)( &al );
    // double check double delete
    EXR_FUN(attr_list_destroy)( &al );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "b", EXR_DEF(ATTR_BOX2I), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_BOX2I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "c", EXR_DEF(ATTR_BOX2F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_BOX2F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "d", EXR_DEF(ATTR_CHLIST), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_CHLIST));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "e", EXR_DEF(ATTR_CHROMATICITIES), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_CHROMATICITIES));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "f", EXR_DEF(ATTR_COMPRESSION), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_COMPRESSION));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "g", EXR_DEF(ATTR_DOUBLE), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_DOUBLE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "h", EXR_DEF(ATTR_ENVMAP), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_ENVMAP));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "i", EXR_DEF(ATTR_FLOAT), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_FLOAT));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "j", EXR_DEF(ATTR_FLOAT_VECTOR), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_FLOAT_VECTOR));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "k", EXR_DEF(ATTR_INT), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_INT));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "l", EXR_DEF(ATTR_KEYCODE), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_KEYCODE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "m", EXR_DEF(ATTR_LINEORDER), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_LINEORDER));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "n", EXR_DEF(ATTR_M33F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M33F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "o", EXR_DEF(ATTR_M33D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M33D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "p", EXR_DEF(ATTR_M44F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M44F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "q", EXR_DEF(ATTR_M44D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M44D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "r", EXR_DEF(ATTR_PREVIEW), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_PREVIEW));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "s", EXR_DEF(ATTR_RATIONAL), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_RATIONAL));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "t", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_STRING));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "u", EXR_DEF(ATTR_STRING_VECTOR), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_STRING_VECTOR));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "v", EXR_DEF(ATTR_TILEDESC), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_TILEDESC));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "w", EXR_DEF(ATTR_TIMECODE), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_TIMECODE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "x", EXR_DEF(ATTR_V2I), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "y", EXR_DEF(ATTR_V2F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "z", EXR_DEF(ATTR_V2D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "0", EXR_DEF(ATTR_V3I), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "1", EXR_DEF(ATTR_V3F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add)(
               f, &al, "2", EXR_DEF(ATTR_V3D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3D));
    assert( al.num_attributes == 28 );

    EXR_FUN(attr_list_destroy)( &al );

    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "b", EXR_DEF(ATTR_BOX2I), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_BOX2I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "c", EXR_DEF(ATTR_BOX2F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_BOX2F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "d", EXR_DEF(ATTR_CHLIST), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_CHLIST));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "e", EXR_DEF(ATTR_CHROMATICITIES), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_CHROMATICITIES));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "f", EXR_DEF(ATTR_COMPRESSION), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_COMPRESSION));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "g", EXR_DEF(ATTR_DOUBLE), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_DOUBLE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "h", EXR_DEF(ATTR_ENVMAP), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_ENVMAP));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "i", EXR_DEF(ATTR_FLOAT), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_FLOAT));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "j", EXR_DEF(ATTR_FLOAT_VECTOR), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_FLOAT_VECTOR));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "k", EXR_DEF(ATTR_INT), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_INT));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "l", EXR_DEF(ATTR_KEYCODE), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_KEYCODE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "m", EXR_DEF(ATTR_LINEORDER), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_LINEORDER));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "n", EXR_DEF(ATTR_M33F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M33F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "o", EXR_DEF(ATTR_M33D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M33D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "p", EXR_DEF(ATTR_M44F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M44F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "q", EXR_DEF(ATTR_M44D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_M44D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "r", EXR_DEF(ATTR_PREVIEW), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_PREVIEW));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "s", EXR_DEF(ATTR_RATIONAL), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_RATIONAL));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "t", EXR_DEF(ATTR_STRING), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_STRING));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "u", EXR_DEF(ATTR_STRING_VECTOR), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_STRING_VECTOR));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "v", EXR_DEF(ATTR_TILEDESC), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_TILEDESC));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "w", EXR_DEF(ATTR_TIMECODE), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_TIMECODE));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "x", EXR_DEF(ATTR_V2I), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "y", EXR_DEF(ATTR_V2F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "z", EXR_DEF(ATTR_V2D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V2D));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "0", EXR_DEF(ATTR_V3I), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3I));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "1", EXR_DEF(ATTR_V3F), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3F));
    assert(EXR_DEF(ERR_SUCCESS) == EXR_FUN(attr_list_add_static_name)(
               f, &al, "2", EXR_DEF(ATTR_V3D), 0, NULL, &out ) );
    assert(out->type == EXR_DEF(ATTR_V3D));
    assert( al.num_attributes == 28 );

    EXR_FUN(attr_list_destroy)( &al );
}

void testAttrLists( const std::string &tempdir )
{
    testAttrListHelper( NULL );
    EXR_TYPE(FILE) *f = createDummyFile( "<attr_lists>" );
    testAttrListHelper( f );
    EXR_FUN(close)( &f );
}


