//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Pixar Animation Studios and Contributors of the OpenEXR Project
//

//-----------------------------------------------------------------------------
//
//	class Pxr24Compressor
//
//	This compressor is based on source code that was contributed to
//	OpenEXR by Pixar Animation Studios.  The compression method was
//	developed by Loren Carpenter.
//
//	The compressor preprocesses the pixel data to reduce entropy,
//	and then calls zlib.
//
//	Compression of HALF and UINT channels is lossless, but compressing
//	FLOAT channels is lossy: 32-bit floating-point numbers are converted
//	to 24 bits by rounding the significand to 15 bits.
//
//	When the compressor is invoked, the caller has already arranged
//	the pixel data so that the values for each channel appear in a
//	contiguous block of memory.  The compressor converts the pixel
//	values to unsigned integers: For UINT, this is a no-op.  HALF
//	values are simply re-interpreted as 16-bit integers.  FLOAT
//	values are converted to 24 bits, and the resulting bit patterns
//	are interpreted as integers.  The compressor then replaces each
//	value with the difference between the value and its left neighbor.
//	This turns flat fields in the image into zeroes, and ramps into
//	strings of similar values.  Next, each difference is split into
//	2, 3 or 4 bytes, and the bytes are transposed so that all the
//	most significant bytes end up in a contiguous block, followed
//	by the second most significant bytes, and so on.  The resulting
//	string of bytes is compressed with zlib.
//
//-----------------------------------------------------------------------------

#include "ImfPxr24Compressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

Pxr24Compressor::Pxr24Compressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (hdr, EXR_COMPRESSION_PXR24, maxScanLineSize, numScanLines)
{
}

Pxr24Compressor::~Pxr24Compressor ()
{
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
