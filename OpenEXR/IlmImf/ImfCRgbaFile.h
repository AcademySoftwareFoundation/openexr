/*

Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
Digital Ltd. LLC

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
*       Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
*       Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
*       Neither the name of Industrial Light & Magic nor the names of
its contributors may be used to endorse or promote products derived
from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef INCLUDED_IMF_C_RGBA_FILE_H
#define INCLUDED_IMF_C_RGBA_FILE_H


#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define IMF_DLL to create a Win32 dll
 */
#ifdef IMF_DLL
#  define IMF_EXPORT __declspec(dllexport)
#else
#  define IMF_EXPORT
#endif

/*
** Interpreting unsigned shorts as 16-bit floating point numbers
*/

typedef unsigned short ImfHalf;

IMF_EXPORT void		ImfFloatToHalf (float f,
					ImfHalf *h);

IMF_EXPORT void		ImfFloatToHalfArray (int n,
					     const float f[/*n*/],
					     ImfHalf h[/*n*/]);

IMF_EXPORT float	ImfHalfToFloat (ImfHalf h);

IMF_EXPORT void		ImfHalfToFloatArray (int n,
					     const ImfHalf h[/*n*/],
					     float f[/*n*/]);

/*
** RGBA pixel; memory layout must be the same as struct Imf::Rgba.
*/

IMF_EXPORT struct ImfRgba
{
    ImfHalf	r;
    ImfHalf	g;
    ImfHalf	b;
    ImfHalf	a;
};

typedef struct ImfRgba ImfRgba;

/*
** Magic number; this must be the same as Imf::MAGIC
*/

#define IMF_MAGIC               20000630

/*
** Version number; this must be the same as Imf::VERSION
*/

#define IMF_VERSION_NUMBER      2

/*
** Line order; values must the the same as in Imf::LineOrder.
*/

#define IMF_INCREASING_Y	0
#define IMF_DECREASING_Y	1


/*
** Compression types; values must be the same as in Imf::Compression.
*/

#define IMF_NO_COMPRESSION	0
#define IMF_RLE_COMPRESSION	1
#define IMF_ZIPS_COMPRESSION	2
#define IMF_ZIP_COMPRESSION	3
#define IMF_PIZ_COMPRESSION	4


/*
** Channels; values must be the same as in Imf::RgbaChannels.
*/

#define IMF_WRITE_R		0x1
#define IMF_WRITE_G		0x2
#define IMF_WRITE_B		0x4
#define IMF_WRITE_A		0x8
#define IMF_WRITE_RGB		0x7
#define IMF_WRITE_RGBA		0xf


/*
** RGBA file header
*/

IMF_EXPORT struct ImfHeader;
typedef struct ImfHeader ImfHeader;

IMF_EXPORT ImfHeader *	ImfNewHeader (void);

IMF_EXPORT void		ImfDeleteHeader (ImfHeader *hdr);

IMF_EXPORT ImfHeader *	ImfCopyHeader (const ImfHeader *hdr);

IMF_EXPORT void		ImfHeaderSetDisplayWindow (ImfHeader *hdr,
						   int xMin, int yMin,
						   int xMax, int yMax);

IMF_EXPORT void		ImfHeaderDisplayWindow (const ImfHeader *hdr,
					int *xMin, int *yMin,
					int *xMax, int *yMax);

IMF_EXPORT void		ImfHeaderSetDataWindow (ImfHeader *hdr,
					int xMin, int yMin,
					int xMax, int yMax);

IMF_EXPORT void		ImfHeaderDataWindow (const ImfHeader *hdr,
				     int *xMin, int *yMin,
				     int *xMax, int *yMax);

IMF_EXPORT void		ImfHeaderSetPixelAspectRatio (ImfHeader *hdr,
					      float pixelAspectRatio);

IMF_EXPORT float		ImfHeaderPixelAspectRatio (const ImfHeader *hdr);

IMF_EXPORT void		ImfHeaderSetScreenWindowCenter (ImfHeader *hdr,
						float x, float y);

IMF_EXPORT void		ImfHeaderScreenWindowCenter (const ImfHeader *hdr,
					     float *x, float *y);

IMF_EXPORT void		ImfHeaderSetScreenWindowWidth (ImfHeader *hdr,
					       float width);

IMF_EXPORT float		ImfHeaderScreenWindowWidth (const ImfHeader *hdr);

IMF_EXPORT void		ImfHeaderSetLineOrder (ImfHeader *hdr,
				       int lineOrder);

IMF_EXPORT int		ImfHeaderLineOrder (const ImfHeader *hdr);
			    
IMF_EXPORT void		ImfHeaderSetCompression (ImfHeader *hdr,
					 int compression);

IMF_EXPORT int		ImfHeaderCompression (const ImfHeader *hdr);

IMF_EXPORT int		ImfHeaderSetIntAttribute (ImfHeader *hdr,
					  const char name[],
					  int value);

IMF_EXPORT int		ImfHeaderIntAttribute (const ImfHeader *hdr,
				       const char name[],
				       int *value);

IMF_EXPORT int		ImfHeaderSetFloatAttribute (ImfHeader *hdr,
					    const char name[],
					    float value);

IMF_EXPORT int		ImfHeaderSetDoubleAttribute (ImfHeader *hdr,
					     const char name[],
					     double value);

IMF_EXPORT int		ImfHeaderFloatAttribute (const ImfHeader *hdr,
				         const char name[],
				         float *value);

IMF_EXPORT int		ImfHeaderDoubleAttribute (const ImfHeader *hdr,
				          const char name[],
				          double *value);

IMF_EXPORT int		ImfHeaderSetStringAttribute (ImfHeader *hdr,
					     const char name[],
					     const char value[]);

IMF_EXPORT int		ImfHeaderStringAttribute (const ImfHeader *hdr,
				          const char name[],
					  const char **value);

IMF_EXPORT int		ImfHeaderSetBox2iAttribute (ImfHeader *hdr,
					    const char name[],
					    int xMin, int yMin,
					    int xMax, int yMax);

IMF_EXPORT int		ImfHeaderBox2iAttribute (const ImfHeader *hdr,
					 const char name[],
					 int *xMin, int *yMin,
					 int *xMax, int *yMax);

IMF_EXPORT int		ImfHeaderSetBox2fAttribute (ImfHeader *hdr,
					    const char name[],
					    float xMin, float yMin,
					    float xMax, float yMax);

IMF_EXPORT int		ImfHeaderBox2fAttribute (const ImfHeader *hdr,
					 const char name[],
					 float *xMin, float *yMin,
					 float *xMax, float *yMax);

IMF_EXPORT int		ImfHeaderSetV2iAttribute (ImfHeader *hdr,
				          const char name[],
				          int x, int y);

IMF_EXPORT int		ImfHeaderV2iAttribute (const ImfHeader *hdr,
				       const char name[],
				       int *x, int *y);

IMF_EXPORT int		ImfHeaderSetV2fAttribute (ImfHeader *hdr,
				          const char name[],
				          float x, float y);

IMF_EXPORT int		ImfHeaderV2fAttribute (const ImfHeader *hdr,
				       const char name[],
				       float *x, float *y);

IMF_EXPORT int		ImfHeaderSetV3iAttribute (ImfHeader *hdr,
				          const char name[],
				          int x, int y, int z);

IMF_EXPORT int		ImfHeaderV3iAttribute (const ImfHeader *hdr,
				       const char name[],
				       int *x, int *y, int *z);

IMF_EXPORT int		ImfHeaderSetV3fAttribute (ImfHeader *hdr,
				          const char name[],
				          float x, float y, float z);

IMF_EXPORT int		ImfHeaderV3fAttribute (const ImfHeader *hdr,
				       const char name[],
				       float *x, float *y, float *z);

IMF_EXPORT int		ImfHeaderSetM33fAttribute (ImfHeader *hdr,
					   const char name[],
					   const float m[3][3]);

IMF_EXPORT int		ImfHeaderM33fAttribute (const ImfHeader *hdr,
					const char name[],
					float m[3][3]);

IMF_EXPORT int		ImfHeaderSetM44fAttribute (ImfHeader *hdr,
					   const char name[],
					   const float m[4][4]);

IMF_EXPORT int		ImfHeaderM44fAttribute (const ImfHeader *hdr,
					const char name[],
					float m[4][4]);

/*
** RGBA output file
*/

IMF_EXPORT struct ImfOutputFile;
typedef struct ImfOutputFile ImfOutputFile;

IMF_EXPORT ImfOutputFile *		ImfOpenOutputFile (const char name[],
					   const ImfHeader *hdr,
					   int channels);

IMF_EXPORT int			ImfCloseOutputFile (ImfOutputFile *out);

IMF_EXPORT int			ImfOutputSetFrameBuffer (ImfOutputFile *out,
						 const ImfRgba *base,
						 size_t xStride,
						 size_t yStride);

IMF_EXPORT int			ImfOutputWritePixels (ImfOutputFile *out,
					      int numScanLines);

IMF_EXPORT int			ImfOutputCurrentScanLine (const ImfOutputFile *out);

IMF_EXPORT const ImfHeader *	ImfOutputHeader (const ImfOutputFile *out);

IMF_EXPORT int			ImfOutputChannels (const ImfOutputFile *out);

/*
** RGBA input file
*/

IMF_EXPORT struct ImfInputFile;
typedef struct ImfInputFile ImfInputFile;

IMF_EXPORT ImfInputFile *		ImfOpenInputFile (const char name[]);

IMF_EXPORT int			ImfCloseInputFile (ImfInputFile *in);

IMF_EXPORT int			ImfInputSetFrameBuffer (ImfInputFile *in,
						ImfRgba *base,
						size_t xStride,
						size_t yStride);

IMF_EXPORT int			ImfInputReadPixels (ImfInputFile *in,
					    int scanLine1,
					    int scanLine2);

IMF_EXPORT const ImfHeader *	ImfInputHeader (const ImfInputFile *in);

IMF_EXPORT int			ImfInputChannels (const ImfInputFile *in);

IMF_EXPORT const char *            ImfInputFileName (const ImfInputFile *in);

/*
** Lookup tables
*/

IMF_EXPORT struct ImfLut;
typedef struct ImfLut ImfLut;

IMF_EXPORT ImfLut *		ImfNewRound12logLut (int channels);

IMF_EXPORT ImfLut *		ImfNewRoundNBitLut (unsigned int n, int channels);

IMF_EXPORT void			ImfDeleteLut (ImfLut *lut);

IMF_EXPORT void			ImfApplyLut (ImfLut *lut,
				     ImfRgba *data,
				     int nData,
				     int stride);
/*
** Most recent error message
*/

IMF_EXPORT const char *		ImfErrorMessage (void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
