// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

// Windows specific addition to prevent the indirect import of the redefined min/max macros
#if defined _WIN32 || defined _WIN64
#    ifdef NOMINMAX
#        undef NOMINMAX
#    endif
#    define NOMINMAX
#endif

#include <openexr.h>

#include "test_value.h"
#include <vector>

void
testBufferCompression (const std::string& tempdir)
{
    size_t bc0   = exr_compress_max_buffer_size (0);
    size_t bc128 = exr_compress_max_buffer_size (128);
    if (bc0 < 9) EXRCORE_TEST_FAIL (bc0 < 9);
    if (bc128 < 128 + 9) EXRCORE_TEST_FAIL (bc0 < 9);
    std::cout << "Max Buffer Size (0): " << bc0 << std::endl;
    std::cout << "Max Buffer Size (128): " << bc128 << std::endl;

    std::vector<char> buf, cbuf;
    buf = {'O', 'p', 'e', 'n', 'E', 'X', 'R'};
    cbuf.resize (exr_compress_max_buffer_size (buf.size ()));
    size_t outsz;
    EXRCORE_TEST_RVAL (exr_compress_buffer (
        nullptr, 9, buf.data (), buf.size (), &cbuf[0], cbuf.size (), &outsz));
    std::cout << "compressed size: " << outsz << std::endl;
    EXRCORE_TEST_RVAL (exr_uncompress_buffer (
        nullptr, cbuf.data (), outsz, &buf[0], buf.size (), &outsz));
    std::cout << "uncompressed size: " << outsz << std::endl;
    if (buf[0] != 'O') EXRCORE_TEST_FAIL (buf[0] != 'O');
}
