//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_ZIP_H
#define INCLUDED_IMF_ZIP_H

#include "ImfNamespace.h"
#include "ImfExport.h"

#include <cstddef>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class Zip
{
    public:
        IMF_EXPORT
        explicit Zip(size_t rawMaxSize);
        IMF_EXPORT
        Zip(size_t maxScanlineSize, size_t numScanLines);
        IMF_EXPORT
        ~Zip();

        Zip (const Zip& other) = delete;
        Zip& operator = (const Zip& other) = delete;
        Zip (Zip&& other) = delete;
        Zip& operator = (Zip&& other) = delete;

        IMF_EXPORT
        size_t maxRawSize();
        IMF_EXPORT
        size_t maxCompressedSize();

        //
        // Compress the raw data into the provided buffer.
        // Returns the amount of compressed data.
        //
        IMF_EXPORT
        int compress(const char *raw, int rawSize, char *compressed);

        // 
        // Uncompress the compressed data into the provided
        // buffer. Returns the amount of raw data actually decoded.
        //
        IMF_EXPORT
        int uncompress(const char *compressed, int compressedSize,
                                                 char *raw);

    private:
        size_t _maxRawSize;
        char  *_tmpBuffer;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
