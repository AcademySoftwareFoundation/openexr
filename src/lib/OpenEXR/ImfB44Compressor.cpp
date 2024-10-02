//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class B44Compressor
//
//	This compressor is lossy for HALF channels; the compression rate
//	is fixed at 32/14 (approximately 2.28).  FLOAT and UINT channels
//	are not compressed; their data are preserved exactly.
//
//	Each HALF channel is split into blocks of 4 by 4 pixels.  An
//	uncompressed block occupies 32 bytes, which are re-interpreted
//	as sixteen 16-bit unsigned integers, t[0] ... t[15].  Compression
//	shrinks the block to 14 bytes.  The compressed 14-byte block
//	contains
//
//	 - t[0]
//
//	 - a 6-bit shift value
//
//	 - 15 densely packed 6-bit values, r[0] ... r[14], which are
//         computed by subtracting adjacent pixel values and right-
//	   shifting the differences according to the stored shift value.
//
//	   Differences between adjacent pixels are computed according
//	   to the following diagram:
//
//		 0 -------->  1 -------->  2 -------->  3
//               |     3            7           11
//               |
//               | 0
//               |
//               v
//		 4 -------->  5 -------->  6 -------->  7
//               |     4            8           12
//               |
//               | 1
//               |
//               v
//		 8 -------->  9 --------> 10 --------> 11
//               |     5            9           13
//               |
//               | 2
//               |
//               v
//		12 --------> 13 --------> 14 --------> 15
//                     6           10           14
//
//	    Here
//
//               5 ---------> 6
//                     8
//
//	    means that r[8] is the difference between t[5] and t[6].
//
//	 - optionally, a 4-by-4 pixel block where all pixels have the
//	   same value can be treated as a special case, where the
//	   compressed block contains only 3 instead of 14 bytes:
//	   t[0], followed by an "impossible" 6-bit shift value and
//	   two padding bits.
//
//	This compressor can handle positive and negative pixel values.
//	NaNs and infinities are replaced with zeroes before compression.
//
//-----------------------------------------------------------------------------

#include "ImfB44Compressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

B44Compressor::B44Compressor (
    const Header& hdr,
    size_t        maxScanLineSize,
    int           numScanLines,
    bool          optFlatFields)
    : Compressor (hdr,
                  optFlatFields ? EXR_COMPRESSION_B44A : EXR_COMPRESSION_B44,
                  maxScanLineSize,
                  numScanLines)
{
}

B44Compressor::~B44Compressor ()
{
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
