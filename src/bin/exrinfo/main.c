/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr.h>
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
