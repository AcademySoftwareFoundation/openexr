// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "general_attr.h"

#include "test_value.h"
#include <openexr.h>

#include <limits.h>
#include <string.h>

// These are hidden by the library currently, but we want to test them...
// this causes a compiler warning about a memcpy beyond valid size but
// we shouldn't actually do the memcpy because of the max size check
#if defined(__GNUC__) && __GNUC__ > 7
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#if defined(OPENEXR_ENABLE_API_VISIBILITY)
#    include "../../lib/OpenEXRCore/attributes.c"
#    include "../../lib/OpenEXRCore/channel_list.c"
#    include "../../lib/OpenEXRCore/float_vector.c"
#    include "../../lib/OpenEXRCore/internal_attr.h"
#    include "../../lib/OpenEXRCore/internal_xdr.h"
#    include "../../lib/OpenEXRCore/opaque.c"
#    include "../../lib/OpenEXRCore/preview.c"
#    include "../../lib/OpenEXRCore/string.c"
#    include "../../lib/OpenEXRCore/string_vector.c"
#else
#    include "../../lib/OpenEXRCore/internal_attr.h"
#    include "../../lib/OpenEXRCore/internal_xdr.h"
#endif

int64_t
dummy_write (
    exr_const_context_t         ctxt,
    void*                       userdata,
    const void*                 buffer,
    uint64_t                    sz,
    uint64_t                    offset,
    exr_stream_error_func_ptr_t error_cb)
{
    return -1;
}

static int s_malloc_fail_on = 0;
static void*
failable_malloc (size_t bytes)
{
    if (s_malloc_fail_on == 1) return NULL;
    if (s_malloc_fail_on > 0) --s_malloc_fail_on;
    return malloc (bytes);
}

static void
failable_free (void* p)
{
    if (!p) abort ();
    free (p);
}

static void
set_malloc_fail_on (int count)
{
    s_malloc_fail_on = count;
}

static void
set_malloc_fail_off ()
{
    s_malloc_fail_on = 0;
}

static exr_context_t
createDummyFile (const char* test)
{
    exr_context_t             f     = NULL;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;

    // we won't actually write to this and so don't need a proper
    // stream but need a writable context to test with.
    cinit.write_fn = dummy_write;
    cinit.alloc_fn = failable_malloc;
    cinit.free_fn = failable_free;

    EXRCORE_TEST_RVAL (
        exr_start_write (&f, test, EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (exr_add_part (f, "dummy", EXR_STORAGE_SCANLINE, NULL));
    return f;
}

void
testAttrSizes (const std::string& tempdir)
{
    EXRCORE_TEST (sizeof (exr_attr_box2i_t) == (4 * 4));
    EXRCORE_TEST (sizeof (exr_attr_box2f_t) == (4 * 4));
    EXRCORE_TEST (sizeof (exr_attr_chromaticities_t) == (8 * 4));
    EXRCORE_TEST (sizeof (exr_attr_keycode_t) == (7 * 4));
    EXRCORE_TEST (sizeof (exr_attr_m33f_t) == (9 * 4));
    EXRCORE_TEST (sizeof (exr_attr_m33d_t) == (9 * 8));
    EXRCORE_TEST (sizeof (exr_attr_m44f_t) == (16 * 4));
    EXRCORE_TEST (sizeof (exr_attr_m44d_t) == (16 * 8));
    EXRCORE_TEST (sizeof (exr_attr_rational_t) == (2 * 4));
    EXRCORE_TEST (sizeof (exr_attr_tiledesc_t) == (2 * 4 + 1));
    EXRCORE_TEST (sizeof (exr_attr_timecode_t) == (2 * 4));
    EXRCORE_TEST (sizeof (exr_attr_v2i_t) == (2 * 4));
    EXRCORE_TEST (sizeof (exr_attr_v2f_t) == (2 * 4));
    EXRCORE_TEST (sizeof (exr_attr_v2d_t) == (2 * 8));
    EXRCORE_TEST (sizeof (exr_attr_v3i_t) == (3 * 4));
    EXRCORE_TEST (sizeof (exr_attr_v3f_t) == (3 * 4));
    EXRCORE_TEST (sizeof (exr_attr_v3d_t) == (3 * 8));
    EXRCORE_TEST (
        (offsetof (exr_attr_opaquedata_t, unpack_func_ptr) &
         alignof (exr_result_t (*) (
             exr_context_t, const void*, int32_t, int32_t*, void**))) == 0);
}

static void
testStringHelper (exr_context_t f)
{
    exr_attr_string_t s, nil = {0};

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_string_init (NULL, &s, 1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_string_destroy (NULL, &s));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_init (f, NULL, 1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_init (f, &s, -1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_init_static (NULL, &s, "exr"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_init_static (f, NULL, "exr"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_init_static (f, &s, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_init_static_with_length (NULL, &s, "exr", 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_init_static_with_length (f, NULL, "exr", 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_init_static_with_length (f, &s, NULL, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_init_static_with_length (f, &s, "exr", -3));
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, NULL));
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &nil));

    EXRCORE_TEST_RVAL (exr_attr_string_init (f, &s, 4));
    EXRCORE_TEST (s.str != NULL);
    EXRCORE_TEST (s.length == 4);
    EXRCORE_TEST (s.alloc_size == 5);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));
    EXRCORE_TEST (s.str == NULL);
    EXRCORE_TEST (s.length == 0);
    EXRCORE_TEST (s.alloc_size == 0);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_create (f, NULL, "exr"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_string_create (NULL, &s, "exr"));

    EXRCORE_TEST_RVAL (exr_attr_string_create (f, &s, NULL));
    EXRCORE_TEST (s.str != NULL && s.str[0] == '\0');
    EXRCORE_TEST (s.length == 0);
    EXRCORE_TEST (s.alloc_size == 1);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    EXRCORE_TEST_RVAL (exr_attr_string_create_with_length (f, &s, NULL, 10));
    EXRCORE_TEST (s.str != NULL && s.str[0] == '\0');
    EXRCORE_TEST (s.length == 10);
    EXRCORE_TEST (s.alloc_size == 11);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_string_create (f, &s, "exr"));
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    EXRCORE_TEST_RVAL (exr_attr_string_create (f, &s, "exr"));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "exr"));
    EXRCORE_TEST (s.length == 3);
    EXRCORE_TEST (s.alloc_size == 4);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_set (f, NULL, "exr"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_string_set (NULL, &s, "exr"));

    EXRCORE_TEST_RVAL (exr_attr_string_set (f, &s, "openexr"));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "openexr"));
    EXRCORE_TEST (s.length == 7);
    EXRCORE_TEST (s.alloc_size == 8);

    EXRCORE_TEST_RVAL (exr_attr_string_set_with_length (f, &s, "exr", 3));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "exr"));
    EXRCORE_TEST (s.length == 3);
    EXRCORE_TEST (s.alloc_size == 8);

    EXRCORE_TEST_RVAL (
        exr_attr_string_set_with_length (f, &s, "exropenexr", 3));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "exr"));
    EXRCORE_TEST (s.length == 3);
    EXRCORE_TEST (s.alloc_size == 8);

    EXRCORE_TEST_RVAL (exr_attr_string_set_with_length (f, &s, NULL, 3));
    EXRCORE_TEST (
        s.str != NULL && s.str[0] == '\0' && s.str[1] == '\0' &&
        s.str[2] == '\0');
    EXRCORE_TEST (s.length == 3);
    EXRCORE_TEST (s.str[s.length] == '\0');
    EXRCORE_TEST (s.alloc_size == 8);

    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    EXRCORE_TEST_RVAL (exr_attr_string_create_with_length (f, &s, "exr", 6));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "exr"));
    EXRCORE_TEST (s.length == 6);
    EXRCORE_TEST (s.alloc_size == 7);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    EXRCORE_TEST_RVAL (
        exr_attr_string_create_with_length (f, &s, "openexr", 3));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "ope"));
    EXRCORE_TEST (s.length == 3);
    EXRCORE_TEST (s.alloc_size == 4);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    EXRCORE_TEST_RVAL (exr_attr_string_init_static (f, &s, "exr"));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "exr"));
    EXRCORE_TEST (s.length == 3);
    EXRCORE_TEST (s.alloc_size == 0);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));
    EXRCORE_TEST (s.str == NULL);
    EXRCORE_TEST (s.length == 0);
    EXRCORE_TEST (s.alloc_size == 0);

    EXRCORE_TEST_RVAL (
        exr_attr_string_init_static_with_length (f, &s, "openexr", 7));
    EXRCORE_TEST (s.str != NULL && !strcmp (s.str, "openexr"));
    EXRCORE_TEST (s.length == 7);
    EXRCORE_TEST (s.alloc_size == 0);
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));
    // make sure we can re-delete something?
    EXRCORE_TEST_RVAL (exr_attr_string_destroy (f, &s));

    size_t nbytes = (size_t) INT32_MAX + 1;
    char*  tmp    = (char*) malloc (nbytes + 1);
    // might be on a low memory machine, in which case just skip the test
    if (tmp)
    {
        memset (tmp, 'B', nbytes);
        tmp[nbytes] = '\0';

        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_MISSING_CONTEXT_ARG,
            exr_attr_string_init_static (NULL, &s, tmp));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INVALID_ARGUMENT, exr_attr_string_init_static (f, &s, tmp));

        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_MISSING_CONTEXT_ARG,
            exr_attr_string_create (NULL, &s, tmp));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INVALID_ARGUMENT, exr_attr_string_create (f, &s, tmp));

        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_string_set (NULL, &s, tmp));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INVALID_ARGUMENT, exr_attr_string_set (f, &s, tmp));
        free (tmp);
    }
}

void
testAttrStrings (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testStringHelper (NULL);
    exr_context_t f = createDummyFile ("<string>");
    testStringHelper (f);
    exr_print_context_info (f, 0);
    exr_print_context_info (f, 1);
    exr_finish (&f);
}

static void
testStringVectorHelper (exr_context_t f)
{
    // TODO: Find a good way to test adding until we grow past the memory limit (i.e. INT32_MAX/2 entries)
    exr_attr_string_vector_t sv, nil = {0};
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_vector_init (NULL, &sv, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_vector_destroy (NULL, &sv));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_vector_init (f, NULL, 4));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, NULL));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &nil));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_vector_init (f, &sv, -4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_init (f, &sv, INT32_MAX / 2));
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_string_vector_init (f, &sv, 1));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_init_entry (f, NULL, 0, 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_init_entry (f, &nil, 0, 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_vector_init_entry (NULL, &nil, 0, 3));

    EXRCORE_TEST_RVAL (exr_attr_string_vector_init (f, &sv, 1));
    EXRCORE_TEST (sv.n_strings == 1);
    EXRCORE_TEST (sv.alloc_size == 1);
    EXRCORE_TEST (sv.strings != NULL);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_init_entry (f, &sv, -1, 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_init_entry (f, &sv, 0, -3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_init_entry (f, &sv, 1, 3));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_set_entry_with_length (f, &sv, -1, NULL, -1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_set_entry_with_length (f, &sv, -1, "exr", 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_set_entry_with_length (f, &sv, 0, NULL, -1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_set_entry_with_length (f, &sv, 1, NULL, -1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_vector_set_entry_with_length (NULL, &sv, 1, NULL, -1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_set_entry_with_length (f, NULL, 1, NULL, -1));

    EXRCORE_TEST_RVAL (exr_attr_string_vector_set_entry (f, &sv, 0, NULL));
    EXRCORE_TEST (sv.strings[0].length == 0);
    EXRCORE_TEST (sv.strings[0].alloc_size == 1);
    EXRCORE_TEST (sv.strings[0].str[0] == '\0');
    EXRCORE_TEST_RVAL (exr_attr_string_vector_set_entry (f, &sv, 0, "exr"));
    EXRCORE_TEST (sv.strings[0].length == 3);
    EXRCORE_TEST (sv.strings[0].alloc_size == 4);
    EXRCORE_TEST (0 == strcmp (sv.strings[0].str, "exr"));

    EXRCORE_TEST_RVAL (exr_attr_string_vector_add_entry (f, &sv, "openexr"));
    EXRCORE_TEST (sv.n_strings == 2);
    EXRCORE_TEST (sv.alloc_size == 2);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_vector_add_entry_with_length (NULL, &sv, "foo", 3));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_string_vector_add_entry_with_length (f, NULL, "foo", 3));
    EXRCORE_TEST (sv.strings[0].length == 3);
    EXRCORE_TEST (sv.strings[0].alloc_size == 4);
    EXRCORE_TEST (0 == strcmp (sv.strings[0].str, "exr"));
    EXRCORE_TEST (sv.strings[1].length == 7);
    EXRCORE_TEST (sv.strings[1].alloc_size == 8);
    EXRCORE_TEST (0 == strcmp (sv.strings[1].str, "openexr"));
    exr_attr_string_vector_t sv2;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_string_vector_copy (NULL, &sv2, &sv));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_vector_copy (f, &sv2, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_string_vector_copy (f, NULL, &sv));
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_string_vector_copy (f, &sv2, &sv));
    EXRCORE_TEST_RVAL_FAIL_MALLOC_AFTER (
        1, EXR_ERR_OUT_OF_MEMORY, exr_attr_string_vector_copy (f, &sv2, &sv));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &sv2));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_copy (f, &sv2, &sv));
    EXRCORE_TEST (sv2.n_strings == 2);
    EXRCORE_TEST (sv2.alloc_size == 2);

    EXRCORE_TEST (sv2.strings[0].length == 3);
    EXRCORE_TEST (sv2.strings[0].alloc_size == 4);
    EXRCORE_TEST (0 == strcmp (sv2.strings[0].str, "exr"));
    EXRCORE_TEST (sv2.strings[1].length == 7);
    EXRCORE_TEST (sv2.strings[1].alloc_size == 8);
    EXRCORE_TEST (0 == strcmp (sv2.strings[1].str, "openexr"));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &sv2));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_init (f, &sv2, 0));
    EXRCORE_TEST_RVAL (
        exr_attr_string_vector_add_entry_with_length (f, &sv2, "foo", 3));
    EXRCORE_TEST_RVAL (
        exr_attr_string_vector_add_entry_with_length (f, &sv2, "bar", 3));
    EXRCORE_TEST_RVAL (
        exr_attr_string_vector_add_entry_with_length (f, &sv2, "baz", 3));
    EXRCORE_TEST_RVAL (
        exr_attr_string_vector_add_entry_with_length (f, &sv2, "frob", 4));
    EXRCORE_TEST_RVAL (
        exr_attr_string_vector_add_entry_with_length (f, &sv2, "nitz", 4));
    EXRCORE_TEST (sv2.n_strings == 5);
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &sv2));
    EXRCORE_TEST_RVAL (exr_attr_string_vector_init (f, &sv2, 0));
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_string_vector_add_entry_with_length (f, &sv2, "foo", 3));

    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &sv));
    EXRCORE_TEST (sv.n_strings == 0);
    EXRCORE_TEST (sv.alloc_size == 0);
    EXRCORE_TEST (sv.strings == NULL);
    // make sure we can re-delete something?
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &sv));

    // might be on a low memory machine, in which case just skip the test
    if (EXR_ERR_SUCCESS ==
        exr_attr_string_vector_init (
            f, &sv, (int) (INT32_MAX / (int) sizeof (exr_attr_string_t))))
    {
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_OUT_OF_MEMORY,
            exr_attr_string_vector_add_entry_with_length (f, &sv, "exr", 3));
    }
    EXRCORE_TEST_RVAL (exr_attr_string_vector_destroy (f, &sv));
}

void
testAttrStringVectors (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testStringVectorHelper (NULL);
    exr_context_t f = createDummyFile ("<stringvector>");
    testStringVectorHelper (f);
    exr_print_context_info (f, 0);
    exr_print_context_info (f, 1);
    exr_finish (&f);
}

static void
testFloatVectorHelper (exr_context_t f)
{
    exr_attr_float_vector_t fv, nil = {0};
    float                   fdata[] = {1.f, 2.f, 3.f, 4.f};
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_float_vector_init (NULL, &fv, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_float_vector_init (f, NULL, 4));
    exr_attr_float_vector_destroy (f, NULL);
    exr_attr_float_vector_destroy (f, &nil);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_float_vector_init (f, &fv, -4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_create (f, NULL, fdata, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_create (f, &fv, NULL, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_create (f, &fv, fdata, -4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_float_vector_create (NULL, &fv, fdata, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_create (f, &fv, fdata, INT32_MAX / 2));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_init_static (f, NULL, fdata, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_init_static (f, &fv, NULL, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_float_vector_init_static (f, &fv, fdata, -4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_float_vector_init_static (NULL, &fv, fdata, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_float_vector_destroy (NULL, &fv));

    EXRCORE_TEST_RVAL (exr_attr_float_vector_init (f, &fv, 4));
    EXRCORE_TEST (fv.length == 4);
    EXRCORE_TEST (fv.alloc_size == 4);
    EXRCORE_TEST (fv.arr != NULL);
    EXRCORE_TEST_RVAL (exr_attr_float_vector_destroy (f, &fv));
    EXRCORE_TEST (fv.length == 0);
    EXRCORE_TEST (fv.alloc_size == 0);
    EXRCORE_TEST (fv.arr == NULL);

    EXRCORE_TEST_RVAL (exr_attr_float_vector_create (f, &fv, fdata, 4));
    EXRCORE_TEST (fv.length == 4);
    EXRCORE_TEST (fv.alloc_size == 4);
    EXRCORE_TEST (fv.arr[0] == 1.f);
    EXRCORE_TEST (fv.arr[1] == 2.f);
    EXRCORE_TEST (fv.arr[2] == 3.f);
    EXRCORE_TEST (fv.arr[3] == 4.f);
    EXRCORE_TEST_RVAL (exr_attr_float_vector_destroy (f, &fv));

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_float_vector_create (f, &fv, fdata, 4));

    EXRCORE_TEST_RVAL (exr_attr_float_vector_init_static (f, &fv, fdata, 4));
    EXRCORE_TEST (fv.length == 4);
    EXRCORE_TEST (fv.alloc_size == 0);
    EXRCORE_TEST (fv.arr == fdata);
    EXRCORE_TEST_RVAL (exr_attr_float_vector_destroy (f, &fv));
    // make sure we can re-delete something?
    EXRCORE_TEST_RVAL (exr_attr_float_vector_destroy (f, &fv));
}

void
testAttrFloatVectors (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testFloatVectorHelper (NULL);
    exr_context_t f = createDummyFile ("<floatvector>");
    testFloatVectorHelper (f);
    exr_print_context_info (f, 0);
    exr_print_context_info (f, 1);
    exr_finish (&f);
}

static void
testChlistHelper (exr_context_t f)
{
    exr_attr_chlist_t cl = {0};

    exr_attr_chlist_destroy (f, NULL);
    exr_attr_chlist_destroy (f, &cl);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_chlist_init (NULL, NULL, 0));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_chlist_init (f, NULL, 0));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_chlist_init (f, &cl, -1));
    EXRCORE_TEST_RVAL (exr_attr_chlist_init (f, &cl, 0));
    exr_attr_chlist_destroy (f, &cl);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_chlist_destroy (NULL, &cl));

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_chlist_init (f, &cl, 3));
    exr_attr_chlist_destroy (f, &cl);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_chlist_add (
            NULL,
            &cl,
            "foo",
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f,
            NULL,
            "foo",
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f,
            &cl,
            "foo",
            EXR_PIXEL_LAST_TYPE,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f,
            &cl,
            "foo",
            (exr_pixel_type_t) -1,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f,
            &cl,
            "foo",
            EXR_PIXEL_HALF,
            (exr_perceptual_treatment_t) 2,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f, &cl, "foo", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LOGARITHMIC, 0, 1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f,
            &cl,
            "foo",
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            -1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f, &cl, "foo", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 0));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f, &cl, "foo", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LINEAR, 1, -1));

    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f, &cl, "foo", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LINEAR, 1, 2));
    EXRCORE_TEST (cl.num_channels == 1);
    EXRCORE_TEST (0 == strcmp (cl.entries[0].name.str, "foo"));
    EXRCORE_TEST (cl.entries[0].pixel_type == EXR_PIXEL_HALF);
    EXRCORE_TEST (cl.entries[0].p_linear == (uint8_t) EXR_PERCEPTUALLY_LINEAR);
    EXRCORE_TEST (cl.entries[0].x_sampling == 1);
    EXRCORE_TEST (cl.entries[0].y_sampling == 2);
    EXRCORE_TEST_RVAL (exr_attr_chlist_destroy (f, &cl));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f, &cl, "", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LINEAR, 1, 1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f, &cl, NULL, EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LINEAR, 1, 1));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add_with_length (
            f, &cl, "R", 0, EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LINEAR, 1, 1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_chlist_add_with_length (
            f, &cl, "R", 1024, EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LINEAR, 1, 1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add_with_length (
            f,
            &cl,
            "R",
            1,
            EXR_PIXEL_LAST_TYPE,
            EXR_PERCEPTUALLY_LINEAR,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add_with_length (
            f,
            &cl,
            "R",
            1,
            EXR_PIXEL_HALF,
            (exr_perceptual_treatment_t) 7,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add_with_length (
            f,
            &cl,
            "R",
            1,
            EXR_PIXEL_FLOAT,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            0,
            1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add_with_length (
            f,
            &cl,
            "R",
            1,
            EXR_PIXEL_UINT,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            -1));
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_chlist_add_with_length (
            f,
            &cl,
            "R",
            1,
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));
    EXRCORE_TEST_RVAL_FAIL_MALLOC_AFTER (
        1,
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_chlist_add_with_length (
            f,
            &cl,
            "R",
            1,
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));

    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f, &cl, "R", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1));
    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f, &cl, "G", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1));
    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f, &cl, "B", EXR_PIXEL_UINT, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1));
    EXRCORE_TEST (cl.num_channels == 3);
    EXRCORE_TEST (0 == strcmp (cl.entries[0].name.str, "B"));
    EXRCORE_TEST (0 == strcmp (cl.entries[1].name.str, "G"));
    EXRCORE_TEST (0 == strcmp (cl.entries[2].name.str, "R"));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_chlist_add (
            f, &cl, "B", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1));
    EXRCORE_TEST (cl.num_channels == 3);

    exr_attr_chlist_t cl2 = {0};
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_chlist_duplicate (NULL, &cl2, &cl));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_chlist_duplicate (f, NULL, &cl));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_chlist_duplicate (f, &cl2, NULL));
    EXRCORE_TEST_RVAL (exr_attr_chlist_duplicate (f, &cl2, &cl));
    EXRCORE_TEST (cl2.num_channels == 3);
    EXRCORE_TEST (0 == strcmp (cl2.entries[0].name.str, "B"));
    EXRCORE_TEST (0 == strcmp (cl2.entries[1].name.str, "G"));
    EXRCORE_TEST (0 == strcmp (cl2.entries[2].name.str, "R"));
    EXRCORE_TEST_RVAL (exr_attr_chlist_destroy (f, &cl2));
    EXRCORE_TEST_RVAL_FAIL_MALLOC_AFTER (
        1, EXR_ERR_OUT_OF_MEMORY, exr_attr_chlist_duplicate (f, &cl2, &cl));
    EXRCORE_TEST_RVAL (exr_attr_chlist_destroy (f, &cl2));

    /* without a file, max will be 31 */
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_chlist_add (
            f,
            &cl,
            "reallongreallongreallonglongname",
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));
    EXRCORE_TEST_RVAL (exr_attr_chlist_destroy (f, &cl));

    // make sure we can re-delete something?
    EXRCORE_TEST_RVAL (exr_attr_chlist_destroy (f, &cl));
}

void
testAttrChlists (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testChlistHelper (NULL);
    exr_context_t f = createDummyFile ("<chlist>");
    testChlistHelper (f);
    exr_print_context_info (f, 0);
    exr_print_context_info (f, 1);
    exr_finish (&f);
}

static void
testPreviewHelper (exr_context_t f)
{
    exr_attr_preview_t p;
    uint8_t            data1x1[] = {0xDE, 0xAD, 0xBE, 0xEF};
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_preview_init (NULL, NULL, 64, 64));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_preview_init (f, NULL, 64, 64));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_preview_destroy (NULL, &p));
    EXRCORE_TEST_RVAL (exr_attr_preview_destroy (f, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_preview_init (f, &p, (uint32_t) -1, 64));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_preview_init (f, &p, 64, (uint32_t) -1));
    EXRCORE_TEST_RVAL (exr_attr_preview_init (f, &p, 1, 1));
    EXRCORE_TEST (p.width == 1);
    EXRCORE_TEST (p.height == 1);
    EXRCORE_TEST (p.alloc_size == 4);
    EXRCORE_TEST (p.rgba != NULL);
    EXRCORE_TEST_RVAL (exr_attr_preview_destroy (f, &p));
    EXRCORE_TEST (p.width == 0);
    EXRCORE_TEST (p.height == 0);
    EXRCORE_TEST (p.alloc_size == 0);
    EXRCORE_TEST (p.rgba == NULL);
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_preview_init (f, &p, 1, 1));

    EXRCORE_TEST_RVAL (exr_attr_preview_create (f, &p, 1, 1, data1x1));
    EXRCORE_TEST (p.width == 1);
    EXRCORE_TEST (p.height == 1);
    EXRCORE_TEST (p.alloc_size == 4);
    EXRCORE_TEST (p.rgba != NULL);
    EXRCORE_TEST (p.rgba[0] == 0xDE);
    EXRCORE_TEST (p.rgba[1] == 0xAD);
    EXRCORE_TEST (p.rgba[2] == 0xBE);
    EXRCORE_TEST (p.rgba[3] == 0xEF);
    EXRCORE_TEST_RVAL (exr_attr_preview_destroy (f, &p));
    // make sure we can re-delete something?
    EXRCORE_TEST_RVAL (exr_attr_preview_destroy (f, &p));

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_preview_create (f, &p, 1, 1, data1x1));
}

void
testAttrPreview (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testPreviewHelper (NULL);
    exr_context_t f = createDummyFile ("<preview>");
    testPreviewHelper (f);
    exr_print_context_info (f, 0);
    exr_print_context_info (f, 1);
    exr_finish (&f);
}

static void
testOpaqueHelper (exr_context_t f)
{
    exr_attr_opaquedata_t o;
    uint8_t               data4[] = {0xDE, 0xAD, 0xBE, 0xEF};
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_opaquedata_init (NULL, NULL, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_opaquedata_init (f, NULL, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_init (f, &o, (size_t) INT32_MAX + 1));
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_attr_opaquedata_init (f, &o, 4));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_opaquedata_destroy (NULL, &o));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, NULL));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_init (f, &o, (uint32_t) -1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_init (f, &o, (size_t) -1));

    EXRCORE_TEST_RVAL (exr_attr_opaquedata_init (f, &o, 4));
    EXRCORE_TEST (o.size == 4);
    EXRCORE_TEST (o.packed_alloc_size == 4);
    EXRCORE_TEST (o.packed_data != NULL);
    exr_attr_opaquedata_destroy (f, &o);
    EXRCORE_TEST (o.size == 0);
    EXRCORE_TEST (o.packed_alloc_size == 0);
    EXRCORE_TEST (o.packed_data == NULL);

    EXRCORE_TEST_RVAL (exr_attr_opaquedata_create (f, &o, 4, data4));
    EXRCORE_TEST (o.size == 4);
    EXRCORE_TEST (o.packed_alloc_size == 4);
    EXRCORE_TEST (o.packed_data != NULL);
    EXRCORE_TEST (0 == memcmp (o.packed_data, data4, 4));

    exr_attr_opaquedata_t o2;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_opaquedata_copy (NULL, &o2, &o));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_opaquedata_copy (f, &o2, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_opaquedata_copy (f, NULL, &o));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_copy (f, &o2, &o));
    EXRCORE_TEST (o2.size == 4);
    EXRCORE_TEST (o2.packed_alloc_size == 4);
    EXRCORE_TEST (o2.packed_data != NULL);
    EXRCORE_TEST (0 == memcmp (o2.packed_data, data4, 4));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, &o2));

    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, &o));
    // make sure we can re-delete something?
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, &o));

    EXRCORE_TEST_RVAL (exr_attr_opaquedata_init (f, &o, 0));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_opaquedata_set_unpacked (NULL, &o, data4, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_set_unpacked (f, NULL, data4, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_set_unpacked (f, &o, data4, -1));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_set_unpacked (f, &o, data4, 4));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, &o));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_init (f, &o, 0));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_set_unpacked (f, &o, data4, 4));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_set_unpacked (f, &o, NULL, 0));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_create (f, &o, 4, data4));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_set_unpacked (f, &o, data4, 4));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_copy (f, &o2, &o));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, &o2));
    EXRCORE_TEST_RVAL (exr_attr_opaquedata_destroy (f, &o));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_opaquedata_copy (f, &o2, NULL));
}

void
testAttrOpaque (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testOpaqueHelper (NULL);
    exr_context_t f = createDummyFile ("<opaque>");
    testOpaqueHelper (f);
    exr_print_context_info (f, 0);
    exr_print_context_info (f, 1);
    exr_finish (&f);
}

int s_unpack_fail = 0;
static exr_result_t
test_unpack (exr_context_t, const void*, int32_t, int32_t* ns, void** ptr)
{
    if (ns) *ns = 4;
    static uint8_t data4[] = {0xDE, 0xAD, 0xBE, 0xEF};
    if (ptr) *ptr = data4;
    if (s_unpack_fail > 0) return EXR_ERR_CORRUPT_CHUNK;
    return EXR_ERR_SUCCESS;
}

int s_pack_fail = 0;
static exr_result_t
test_pack (exr_context_t, const void*, int32_t, int32_t* nsize, void* ptr)
{
    *nsize = 1;
    if (ptr) *((char*) ptr) = 'E';
    if (s_pack_fail == 1) return EXR_ERR_CORRUPT_CHUNK;
    if (s_pack_fail > 0) --s_pack_fail;
    return EXR_ERR_SUCCESS;
}

static void
test_hdlr_destroy (exr_context_t, void*, int32_t)
{}

void
testAttrHandler (const std::string& tempdir)
{
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_register_attr_type_handler (
            NULL, "mytype", &test_unpack, &test_pack, &test_hdlr_destroy));

    int32_t nsz = -1;
    void*   packed;
    uint8_t data4[] = {0xDE, 0xAD, 0xBE, 0xEF};

    exr_context_t    f   = createDummyFile ("<attr_handler>");
    exr_attribute_t *foo = NULL, *bar = NULL;
    EXRCORE_TEST_RVAL (exr_attr_declare_by_type (f, 0, "foo", "mytype", &foo));
    EXRCORE_TEST (foo != NULL);
    EXRCORE_TEST (foo->opaque->unpack_func_ptr == NULL);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_pack (f, foo->opaque, &nsz, &packed));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_unpack (f, foo->opaque, &nsz, &packed));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_opaquedata_unpack (NULL, foo->opaque, &nsz, &packed));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_unpack (f, NULL, &nsz, &packed));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_register_attr_type_handler (
            f, NULL, &test_unpack, &test_pack, &test_hdlr_destroy));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_register_attr_type_handler (
            f, "", &test_unpack, &test_pack, &test_hdlr_destroy));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_register_attr_type_handler (
            f,
            "reallongreallongreallonglongname",
            &test_unpack,
            &test_pack,
            &test_hdlr_destroy));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_register_attr_type_handler (
            f, "box2i", &test_unpack, &test_pack, &test_hdlr_destroy));
    EXRCORE_TEST_RVAL (exr_register_attr_type_handler (
        f, "mytype", &test_unpack, &test_pack, &test_hdlr_destroy));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_register_attr_type_handler (
            f, "mytype", &test_unpack, &test_pack, &test_hdlr_destroy));
    EXRCORE_TEST (foo->opaque->unpack_func_ptr == &test_unpack);
    EXRCORE_TEST (foo->opaque->pack_func_ptr == &test_pack);
    EXRCORE_TEST (foo->opaque->destroy_unpacked_func_ptr == &test_hdlr_destroy);

    EXRCORE_TEST_RVAL (exr_attr_declare_by_type (f, 0, "bar", "mytype", &bar));
    EXRCORE_TEST (bar != NULL);
    EXRCORE_TEST (bar->opaque->unpack_func_ptr == &test_unpack);
    EXRCORE_TEST (bar->opaque->pack_func_ptr == &test_pack);
    EXRCORE_TEST (bar->opaque->destroy_unpacked_func_ptr == &test_hdlr_destroy);

    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_set_unpacked (f, foo->opaque, data4, 4));
    // make sure it keeps the unpacked value
    s_unpack_fail = 1;
    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_unpack (f, foo->opaque, &nsz, &packed));
    s_unpack_fail = 0;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_opaquedata_pack (NULL, foo->opaque, &nsz, &packed));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_pack (f, NULL, &nsz, &packed));
    EXRCORE_TEST (nsz == 0);
    EXRCORE_TEST (packed == NULL);

    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_pack (f, foo->opaque, &nsz, &packed));
    // make sure it keeps the old value
    s_pack_fail = 1;
    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_pack (f, foo->opaque, &nsz, &packed));
    s_pack_fail = 0;

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_opaquedata_set_packed (NULL, bar->opaque, data4, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_set_packed (f, NULL, data4, 4));
    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_set_packed (f, bar->opaque, data4, 4));
    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_set_packed (f, bar->opaque, data4, 4));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_opaquedata_set_packed (f, bar->opaque, data4, -1));
    EXRCORE_TEST (bar->opaque->unpack_func_ptr == &test_unpack);
    EXRCORE_TEST (bar->opaque->pack_func_ptr == &test_pack);
    EXRCORE_TEST (bar->opaque->destroy_unpacked_func_ptr == &test_hdlr_destroy);
    EXRCORE_TEST (bar->opaque->unpacked_data == NULL);

    s_unpack_fail = 1;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_CORRUPT_CHUNK,
        exr_attr_opaquedata_unpack (f, bar->opaque, &nsz, &packed));
    s_unpack_fail = 0;
    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_unpack (f, bar->opaque, &nsz, &packed));
    EXRCORE_TEST (bar->opaque->unpacked_data != NULL);

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_opaquedata_set_packed (f, bar->opaque, data4, 4));
    // make sure state didn't change
    EXRCORE_TEST (bar->opaque->unpacked_data != NULL);

    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_set_packed (f, bar->opaque, data4, 4));

    EXRCORE_TEST_RVAL (
        exr_attr_opaquedata_set_unpacked (f, foo->opaque, data4, 4));
    s_pack_fail = 1;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_CORRUPT_CHUNK,
        exr_attr_opaquedata_pack (f, foo->opaque, &nsz, &packed));
    s_pack_fail = 2;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_CORRUPT_CHUNK,
        exr_attr_opaquedata_pack (f, foo->opaque, &nsz, &packed));
    s_pack_fail = 0;
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_opaquedata_pack (f, foo->opaque, &nsz, &packed));

    exr_finish (&f);
}

static void
testAttrListHelper (exr_context_t f)
{
    exr_attribute_list_t al = {0};
    exr_attribute_t*     out;
    exr_attribute_t*     out2;
    uint8_t*             extra;
    uint64_t             sz;

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_list_destroy (NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_list_destroy (NULL, &al));
    EXRCORE_TEST_RVAL (exr_attr_list_destroy (f, NULL));
    EXRCORE_TEST_RVAL (exr_attr_list_destroy (f, &al));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NO_ATTR_BY_NAME,
        exr_attr_list_find_by_name (f, &al, "exr", &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_list_find_by_name (NULL, &al, "exr", &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_find_by_name (f, NULL, "exr", &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_find_by_name (f, &al, "exr", NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_find_by_name (f, &al, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_find_by_name (f, &al, "", &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_compute_size (f, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_compute_size (f, &al, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_list_compute_size (NULL, &al, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_compute_size (f, NULL, &sz));
    EXRCORE_TEST_RVAL (exr_attr_list_compute_size (f, &al, &sz));
    EXRCORE_TEST (0 == sz);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, NULL, "myattr", "mytype", 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, NULL, "mytype", 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "", "mytype", 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", NULL, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "", 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "mytype", 0, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "mytype", -1, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "mytype", 1, NULL, &out));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, NULL, "myattr", "mytype", 1, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "mytype", 1, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "mytype", -1, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "myattr", "mytype", 1, NULL, &out));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_list_add_by_type (
            f,
            &al,
            "reallongreallongreallonglongname",
            "box2i",
            0,
            NULL,
            &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_list_add_by_type (
            f, &al, "x", "reallongreallongreallonglongname", 0, NULL, &out));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_list_add (
            NULL, &al, "myattr", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, NULL, "myattr", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, NULL, EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (
            f, &al, "myattr", EXR_ATTR_LAST_KNOWN_TYPE, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "myattr", EXR_ATTR_UNKNOWN, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (
            f, &al, "myattr", (exr_attribute_type_t) -1, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "myattr", EXR_ATTR_STRING, 0, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "myattr", EXR_ATTR_STRING, -1, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "myattr", EXR_ATTR_STRING, 1, NULL, &out));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, NULL, "myattr", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, NULL, EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "myattr", EXR_ATTR_LAST_KNOWN_TYPE, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "myattr", EXR_ATTR_UNKNOWN, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "myattr", (exr_attribute_type_t) -1, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "myattr", EXR_ATTR_STRING, 0, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "myattr", EXR_ATTR_STRING, -1, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "myattr", EXR_ATTR_STRING, 1, NULL, &out));

    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "myattr", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "xxx", EXR_ATTR_STRING, 0, NULL, &out2));
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "yyy", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_attr_list_remove (NULL, &al, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_remove (f, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_remove (f, &al, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_remove (f, NULL, out));
    EXRCORE_TEST_RVAL (exr_attr_list_remove (f, &al, out2));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_attr_list_remove (f, &al, out2));
    EXRCORE_TEST (al.num_attributes == 2);

    EXRCORE_TEST_RVAL (exr_attr_list_add_by_type (
        f, &al, "tnst42", "string", 42, &extra, &out));
    EXRCORE_TEST (extra != NULL);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "tnst0", "string", 0, &extra, &out));
    EXRCORE_TEST (extra == NULL);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "nst42", EXR_ATTR_STRING, 42, &extra, &out));
    EXRCORE_TEST (extra != NULL);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "nst0", EXR_ATTR_STRING, 0, &extra, &out));
    EXRCORE_TEST (extra == NULL);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "t42", EXR_ATTR_STRING, 42, &extra, &out));
    EXRCORE_TEST (extra != NULL);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "t0", EXR_ATTR_STRING, 0, &extra, &out));
    EXRCORE_TEST (extra == NULL);
    // by destroying the list here, if extra is leaking, valgrind will find something
    exr_attr_list_destroy (f, &al);

    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "a", "mytype", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_OPAQUE);
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_list_add_by_type (f, &al, "zzzz", "mytype", 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_list_add_by_type (NULL, &al, "a", "box2i", 0, NULL, &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "a", "box2i", 0, NULL, &out));

    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "b", "box2i", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2I);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "b", "box2i", 0, NULL, &out));
    EXRCORE_TEST (al.num_attributes == 2);
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2I);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_by_type (f, &al, "b", "box2i", 3, &extra, &out));
    EXRCORE_TEST (al.num_attributes == 2);

    EXRCORE_TEST (al.num_alloced == 2);
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_list_add_by_type (f, &al, "c", "box2f", 0, NULL, &out));
    EXRCORE_TEST (al.num_attributes == 2);
    EXRCORE_TEST (al.num_alloced == 2);
    EXRCORE_TEST_RVAL_FAIL_MALLOC_AFTER (
        1,
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_list_add_by_type (f, &al, "c", "box2f", 0, NULL, &out));
    EXRCORE_TEST (al.num_attributes == 2);

    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "c", "box2f", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "d", "chlist", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_CHLIST);
    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f,
        out->chlist,
        "R",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f,
        out->chlist,
        "G",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_attr_chlist_add (
        f,
        out->chlist,
        "B",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));

    EXRCORE_TEST_RVAL (exr_attr_list_add_by_type (
        f, &al, "e", "chromaticities", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_CHROMATICITIES);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "f", "compression", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_COMPRESSION);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "g", "double", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_DOUBLE);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "h", "envmap", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_ENVMAP);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "i", "float", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_FLOAT);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "j", "floatvector", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_FLOAT_VECTOR);
    EXRCORE_TEST_RVAL (exr_attr_float_vector_init (f, out->floatvector, 4));

    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "k", "int", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_INT);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "l", "keycode", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_KEYCODE);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "m", "lineOrder", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_LINEORDER);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "n", "m33f", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M33F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "o", "m33d", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M33D);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "p", "m44f", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M44F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "q", "m44d", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M44D);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "r", "preview", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_PREVIEW);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "s", "rational", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_RATIONAL);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "t", "string", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_STRING);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "u", "stringvector", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_STRING_VECTOR);
    EXRCORE_TEST_RVAL (
        exr_attr_string_vector_add_entry (f, out->stringvector, "openexr"));

    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "v", "tiledesc", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_TILEDESC);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "w", "timecode", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_TIMECODE);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "x", "v2i", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2I);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "y", "v2f", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "z", "v2d", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2D);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "0", "v3i", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3I);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "1", "v3f", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add_by_type (f, &al, "2", "v3d", 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3D);
    EXRCORE_TEST (al.num_attributes == 29);
    EXRCORE_TEST_RVAL (exr_attr_list_compute_size (f, &al, &sz));
    EXRCORE_TEST_RVAL (exr_attr_list_find_by_name (f, &al, "x", &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2I);
    EXRCORE_TEST_RVAL (exr_attr_list_find_by_name (f, &al, "a", &out));
    EXRCORE_TEST (out->type == EXR_ATTR_OPAQUE);

    exr_attr_list_destroy (f, &al);
    // double check double delete
    exr_attr_list_destroy (f, &al);

    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "b", EXR_ATTR_BOX2I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2I);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "b", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST (out == NULL);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "b", EXR_ATTR_BOX2I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2I);
    EXRCORE_TEST (al.num_attributes == 1);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_list_add (
            f,
            &al,
            "reallongreallongreallonglongname",
            EXR_ATTR_BOX2I,
            0,
            NULL,
            &out));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add (f, &al, "yyy", EXR_ATTR_OPAQUE, 0, NULL, &out));

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_list_add (f, &al, "c", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST (al.num_attributes == 1);

    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "c", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "d", EXR_ATTR_CHLIST, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_CHLIST);
    EXRCORE_TEST_RVAL (exr_attr_list_add (
        f, &al, "e", EXR_ATTR_CHROMATICITIES, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_CHROMATICITIES);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "f", EXR_ATTR_COMPRESSION, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_COMPRESSION);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "g", EXR_ATTR_DOUBLE, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_DOUBLE);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "h", EXR_ATTR_ENVMAP, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_ENVMAP);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "i", EXR_ATTR_FLOAT, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_FLOAT);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "j", EXR_ATTR_FLOAT_VECTOR, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_FLOAT_VECTOR);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "k", EXR_ATTR_INT, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_INT);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "l", EXR_ATTR_KEYCODE, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_KEYCODE);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "m", EXR_ATTR_LINEORDER, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_LINEORDER);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "n", EXR_ATTR_M33F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M33F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "o", EXR_ATTR_M33D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M33D);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "p", EXR_ATTR_M44F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M44F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "q", EXR_ATTR_M44D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M44D);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "r", EXR_ATTR_PREVIEW, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_PREVIEW);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "s", EXR_ATTR_RATIONAL, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_RATIONAL);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "t", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_STRING);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "u", EXR_ATTR_STRING_VECTOR, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_STRING_VECTOR);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "v", EXR_ATTR_TILEDESC, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_TILEDESC);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "w", EXR_ATTR_TIMECODE, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_TIMECODE);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "x", EXR_ATTR_V2I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2I);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "y", EXR_ATTR_V2F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "z", EXR_ATTR_V2D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2D);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "0", EXR_ATTR_V3I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3I);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "1", EXR_ATTR_V3F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3F);
    EXRCORE_TEST_RVAL (
        exr_attr_list_add (f, &al, "2", EXR_ATTR_V3D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3D);
    EXRCORE_TEST (al.num_attributes == 28);

    exr_attr_list_destroy (f, &al);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_attr_list_add_static_name (
            NULL, &al, "b", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "b", EXR_ATTR_BOX2I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2I);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "b", EXR_ATTR_BOX2I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2I);
    EXRCORE_TEST (al.num_attributes == 1);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "b", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST (out == NULL);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_list_add_static_name (
            f,
            &al,
            "reallongreallongreallonglongname",
            EXR_ATTR_BOX2I,
            0,
            NULL,
            &out));
    EXRCORE_TEST (al.num_attributes == 1);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_attr_list_add_static_name (
            f, &al, "c", EXR_ATTR_OPAQUE, 0, NULL, &out));
    EXRCORE_TEST (al.num_attributes == 1);

    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY,
        exr_attr_list_add_static_name (
            f, &al, "c", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST (al.num_attributes == 1);

    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "c", EXR_ATTR_BOX2F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_BOX2F);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "d", EXR_ATTR_CHLIST, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_CHLIST);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "e", EXR_ATTR_CHROMATICITIES, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_CHROMATICITIES);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "f", EXR_ATTR_COMPRESSION, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_COMPRESSION);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "g", EXR_ATTR_DOUBLE, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_DOUBLE);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "h", EXR_ATTR_ENVMAP, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_ENVMAP);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "i", EXR_ATTR_FLOAT, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_FLOAT);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "j", EXR_ATTR_FLOAT_VECTOR, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_FLOAT_VECTOR);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "k", EXR_ATTR_INT, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_INT);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "l", EXR_ATTR_KEYCODE, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_KEYCODE);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "m", EXR_ATTR_LINEORDER, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_LINEORDER);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "n", EXR_ATTR_M33F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M33F);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "o", EXR_ATTR_M33D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M33D);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "p", EXR_ATTR_M44F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M44F);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "q", EXR_ATTR_M44D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_M44D);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "r", EXR_ATTR_PREVIEW, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_PREVIEW);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "s", EXR_ATTR_RATIONAL, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_RATIONAL);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "t", EXR_ATTR_STRING, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_STRING);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "u", EXR_ATTR_STRING_VECTOR, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_STRING_VECTOR);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "v", EXR_ATTR_TILEDESC, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_TILEDESC);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "w", EXR_ATTR_TIMECODE, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_TIMECODE);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "x", EXR_ATTR_V2I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2I);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "y", EXR_ATTR_V2F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2F);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "z", EXR_ATTR_V2D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V2D);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "0", EXR_ATTR_V3I, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3I);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "1", EXR_ATTR_V3F, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3F);
    EXRCORE_TEST_RVAL (exr_attr_list_add_static_name (
        f, &al, "2", EXR_ATTR_V3D, 0, NULL, &out));
    EXRCORE_TEST (out->type == EXR_ATTR_V3D);
    EXRCORE_TEST (al.num_attributes == 28);

    exr_attr_list_destroy (f, &al);
}

void
testAttrLists (const std::string& tempdir)
{
    // we no longer allow a null context as we need the memory allocator
    //testAttrListHelper (NULL);
    exr_context_t f = createDummyFile ("<attr_lists>");
    testAttrListHelper (f);
    exr_finish (&f);
}

void
testXDR (const std::string& tempdir)
{
    uint64_t v64 = 0x123456789ABCDEF0;
    uint32_t v32 = 0x12345678;
    uint16_t v16 = 0x1234;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
    uint64_t ov64 = 0xF0DEBC9A78563412;
    uint32_t ov32 = 0x78563412;
    uint16_t ov16 = 0x3412;
#endif
    uint8_t  v8buf[]  = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    uint16_t v16buf[] = {0xAA00, 0xBB11, 0xCC22, 0xDD33, 0xEE44};
    uint32_t v32buf[] = {0xAA00BB11, 0xCC22DD33};
    uint64_t v64buf[] = {0xAA00BB11CC22DD33, 0xEE44FF5500661177};
    float    v32f     = 42.f;
    EXRCORE_TEST (one_to_native64 (one_from_native64 (v64)) == v64);
    EXRCORE_TEST (one_to_native32 (one_from_native32 (v32)) == v32);
    EXRCORE_TEST (one_to_native16 (one_from_native16 (v16)) == v16);
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
    EXRCORE_TEST (one_to_native64 (v64) == ov64);
    EXRCORE_TEST (one_to_native32 (v32) == ov32);
    EXRCORE_TEST (one_to_native16 (v16) == ov16);
#endif
    priv_from_native (v8buf, 5, sizeof (uint8_t));
    priv_to_native (v8buf, 5, sizeof (uint8_t));
    EXRCORE_TEST (v8buf[2] == 0xCC);
    priv_from_native (v16buf, 5, sizeof (uint16_t));
    priv_to_native (v16buf, 5, sizeof (uint16_t));
    EXRCORE_TEST (v16buf[2] == 0xCC22);
    priv_from_native (v32buf, 2, sizeof (uint32_t));
    priv_to_native (v32buf, 2, sizeof (uint32_t));
    EXRCORE_TEST (v32buf[1] == 0xCC22DD33);
    priv_from_native (v64buf, 2, sizeof (uint64_t));
    priv_to_native (v64buf, 2, sizeof (uint64_t));
    EXRCORE_TEST (v64buf[0] == 0xAA00BB11CC22DD33);

    EXRCORE_TEST (one_to_native_float (one_from_native_float (v32f)) == v32f);
}

#if defined(__GNUC__) && __GNUC__ > 7
#    pragma GCC diagnostic pop
#endif
