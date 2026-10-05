/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#    include <fcntl.h>
#    include <io.h>
#    include <windows.h>
#else
#    include <unistd.h>
#endif

#include <stdlib.h>

static void
usage (FILE* stream, const char* argv0, int verbose)
{
    fprintf (
        stream,
        "Usage: %s [-v|--verbose] [-a|--all-metadata] [-s|--strict] <filename> [<filename> ...]\n\n",
        argv0);

    if (verbose)
        fprintf (
            stream,
            "\n"
            "Read exr files and print values of header attributes\n"
            "\n"
            "Warnings about inconsistent color space metadata are printed to\n"
            "stderr. They do not affect the exit status, since they do not\n"
            "make a file malformed.\n"
            "\n"
            "Options:\n"
            "  -s, --strict        strict mode\n"
            "  -a, --all-metadata  print all metadata\n"
            "  -v, --verbose       verbose mode\n"
            "  -h, --help          print this message\n"
            "      --version       print version information\n"
            "\n"
            "Report bugs via https://github.com/AcademySoftwareFoundation/openexr/issues or email security@openexr.com\n"
            "");
}

static void
error_handler_cb (exr_const_context_t f, int code, const char* msg)
{
    const char* fn;
    if (EXR_ERR_SUCCESS != exr_get_file_name (f, &fn)) fn = "<error>";
    fprintf (
        stderr,
        "ERROR '%s' (%s): %s\n",
        fn,
        exr_get_error_code_as_string (code),
        msg);
}

/*
 * The longest attribute value exrinfo will quote in a warning. Values come
 * from the file, so they can be arbitrarily long; a message must not be
 * proportional to the size of the thing it complains about.
 */
#define EXRINFO_MAX_VALUE_PRINT 64

/* Enough for the value, the quotes and the truncation marker. */
#define EXRINFO_VALUE_BUF_SIZE (EXRINFO_MAX_VALUE_PRINT + 32)

static void
quote_value (const char* s, char* buf, size_t bufsz)
{
    if (strlen (s) <= EXRINFO_MAX_VALUE_PRINT)
        snprintf (buf, bufsz, "'%s'", s);
    else
        snprintf (
            buf, bufsz, "'%.*s...' (truncated)", EXRINFO_MAX_VALUE_PRINT, s);
}

/* Return a part's colorInteropID, or NULL if it has none. */
static const char*
color_interop_id (exr_const_context_t f, int part)
{
    const exr_attribute_t* attr = NULL;

    if (EXR_ERR_SUCCESS !=
        exr_get_attribute_by_name (f, part, "colorInteropID", &attr))
        return NULL;

    if (attr->type != EXR_ATTR_STRING) return NULL;

    return attr->string->str;
}

/*
 * Return the ID whose chromaticities match the part's chromaticities
 * attribute, or NULL if there is no such attribute or it matches no known
 * color space.
 */
static const char*
chromaticities_interop_id (exr_const_context_t f, int part)
{
    exr_attr_chromaticities_t chroma;
    const char*               id = NULL;

    if (EXR_ERR_SUCCESS !=
        exr_attr_get_chromaticities (f, part, "chromaticities", &chroma))
        return NULL;

    if (EXR_ERR_SUCCESS != exr_chromaticities_to_color_interop_id (
                               &chroma,
                               EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
                               &id))
        return NULL;

    return id;
}

/*
 * The library reports which rule was broken; for the two warnings that are
 * about a relationship between values, name the values as well.
 */
static void
print_color_metadata_warning (
    exr_const_context_t          f,
    const char*                  fn,
    int                          part,
    exr_color_metadata_warning_t warning)
{
    char idbuf[EXRINFO_VALUE_BUF_SIZE];
    char otherbuf[EXRINFO_VALUE_BUF_SIZE];

    const char* id = color_interop_id (f, part);

    if (id && warning == EXR_COLOR_METADATA_CHROMATICITIES_DIFFER)
    {
        const char* match = chromaticities_interop_id (f, part);

        if (match)
        {
            quote_value (id, idbuf, sizeof (idbuf));
            quote_value (match, otherbuf, sizeof (otherbuf));
            fprintf (
                stderr,
                "WARNING '%s' (part %d): colorInteropID is %s but the chromaticities are those of %s\n",
                fn,
                part,
                idbuf,
                otherbuf);
            return;
        }
    }
    else if (id && warning == EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0)
    {
        quote_value (id, idbuf, sizeof (idbuf));
        fprintf (
            stderr,
            "WARNING '%s' (part %d): acesImageContainerFlag is present but colorInteropID is %s, not 'lin_ap0_scene'\n",
            fn,
            part,
            idbuf);
        return;
    }
    else if (warning == EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0)
    {
        const char* match = chromaticities_interop_id (f, part);

        if (match)
        {
            quote_value (match, otherbuf, sizeof (otherbuf));
            fprintf (
                stderr,
                "WARNING '%s' (part %d): acesImageContainerFlag is present but the chromaticities are those of %s, not 'lin_ap0_scene'\n",
                fn,
                part,
                otherbuf);
            return;
        }
    }
    else if (id && warning == EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED)
    {
        const char* first = color_interop_id (f, 0);

        quote_value (id, idbuf, sizeof (idbuf));

        if (first)
        {
            quote_value (first, otherbuf, sizeof (otherbuf));
            fprintf (
                stderr,
                "WARNING '%s' (part %d): colorInteropID is %s but part 0's is %s\n",
                fn,
                part,
                idbuf,
                otherbuf);
        }
        else
        {
            fprintf (
                stderr,
                "WARNING '%s' (part %d): colorInteropID is %s but part 0 has none\n",
                fn,
                part,
                idbuf);
        }
        return;
    }

    fprintf (
        stderr,
        "WARNING '%s' (part %d): %s\n",
        fn,
        part,
        exr_get_color_metadata_warning_as_string (warning));
}

/*
 * Report any part whose color space metadata is inconsistent. These do not
 * make the file malformed, so they do not count towards the exit status.
 */
static void
report_color_metadata (exr_const_context_t f)
{
    static const exr_color_metadata_warning_t all_warnings[] = {
        EXR_COLOR_METADATA_EMPTY_INTEROP_ID,
        EXR_COLOR_METADATA_CHROMATICITIES_DIFFER,
        EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES,
        EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE,
        EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL,
        EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED,
        EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE,
        EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0,
        EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES,
        EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0};
    static const size_t num_warnings =
        sizeof (all_warnings) / sizeof (all_warnings[0]);

    const char* fn;
    int         numparts = 0;

    if (EXR_ERR_SUCCESS != exr_get_file_name (f, &fn)) fn = "<error>";

    if (EXR_ERR_SUCCESS != exr_get_count (f, &numparts)) return;

    for (int p = 0; p < numparts; ++p)
    {
        uint32_t warnings = EXR_COLOR_METADATA_OK;

        if (EXR_ERR_SUCCESS != exr_check_color_metadata (f, p, &warnings))
            continue;

        for (size_t w = 0; w < num_warnings; ++w)
        {
            if (warnings & (uint32_t) all_warnings[w])
                print_color_metadata_warning (f, fn, p, all_warnings[w]);
        }
    }
}

static int64_t
stdin_reader (
    exr_const_context_t         file,
    void*                       userdata,
    void*                       buffer,
    uint64_t                    sz,
    uint64_t                    offset,
    exr_stream_error_func_ptr_t error_cb)
{
    static uint64_t lastoffset = 0;
    int64_t         nread      = 0;

    (void) userdata;

    if (offset != lastoffset)
    {
        error_cb (file, EXR_ERR_READ_IO, "Unable to seek in stdin stream");
        return -1;
    }
#ifdef _WIN32
    if (sz >= (size_t) (INT32_MAX))
    {
        error_cb (
            file, EXR_ERR_READ_IO, "Read request too large for win32 API");
        return -1;
    }
    nread = _read (_fileno (stdin), buffer, (unsigned) sz);
#else
    nread = read (STDIN_FILENO, buffer, sz);
#endif
    if (nread > 0) lastoffset = offset + (uint64_t) nread;
    return nread;
}

static int
process_stdin (int verbose, int allmeta, int strict)
{
    int                       failcount = 0;
    exr_result_t              rv;
    exr_context_t             e     = NULL;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &error_handler_cb;
    cinit.read_fn                   = &stdin_reader;

    if (!verbose) cinit.flags |= EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE;

    if (strict) cinit.flags |= EXR_CONTEXT_FLAG_STRICT_HEADER;

#ifdef _WIN32
    _setmode (_fileno (stdin), _O_BINARY);
#endif
    rv = exr_start_read (&e, "<stdin>", &cinit);
    if (rv == EXR_ERR_SUCCESS)
    {
        exr_print_context_info (e, verbose || allmeta);
        report_color_metadata (e);
        exr_finish (&e);
    }
    else
        ++failcount;
    return failcount;
}

static int
process_file (const char* filename, int verbose, int allmeta, int strict)
{
    int                       failcount = 0;
    exr_result_t              rv;
    exr_context_t             e     = NULL;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &error_handler_cb;

    if (!verbose) cinit.flags |= EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE;

    if (strict) cinit.flags |= EXR_CONTEXT_FLAG_STRICT_HEADER;

    rv = exr_start_read (&e, filename, &cinit);

    if (rv == EXR_ERR_SUCCESS)
    {
        exr_print_context_info (e, verbose || allmeta);
        report_color_metadata (e);
        exr_finish (&e);
    }
    else
        ++failcount;
    return failcount;
}

int
main (int argc, const char* argv[])
{
    int rv = 0, verbose = 0, allmeta = 0, strict = 0;

    for (int a = 1; a < argc; ++a)
    {
        if (!strcmp (argv[a], "-h") || !strcmp (argv[a], "-?") ||
            !strcmp (argv[a], "--help"))
        {
            usage (stdout, "exrinfo", 1);
            return 0;
        }
        else if (!strcmp (argv[a], "--version"))
        {
            printf (
                "exrinfo (OpenEXR) %s  https://openexr.com\n",
                OPENEXR_VERSION_STRING);
            printf ("Copyright (c) Contributors to the OpenEXR Project\n");
            printf ("License BSD-3-Clause\n");
            return 0;
        }
        else if (!strcmp (argv[a], "-v") || !strcmp (argv[a], "--verbose"))
        {
            verbose = 1;
        }
        else if (!strcmp (argv[a], "-a") || !strcmp (argv[a], "--all-metadata"))
        {
            allmeta = 1;
        }
        else if (!strcmp (argv[a], "-s") || !strcmp (argv[a], "--strict"))
        {
            strict = 1;
        }
        else if (!strcmp (argv[a], "-"))
        {
            rv += process_stdin (verbose, allmeta, strict);
        }
        else if (argv[a][0] == '-')
        {
            usage (stderr, argv[0], 0);
            return 1;
        }
        else { rv += process_file (argv[a], verbose, allmeta, strict); }
    }

    return rv;
}
