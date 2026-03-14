//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_INPUT_FILE_H
#define INCLUDED_IMF_INPUT_FILE_H

//-----------------------------------------------------------------------------
//
//	class InputFile -- a scanline-based interface that can be used
//	to read both scanline-based and tiled OpenEXR image files.
//
//-----------------------------------------------------------------------------

#include "ImfForward.h"

#include "ImfThreading.h"

#include "ImfContext.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class TiledInputFile;

/// \brief Provides generic access to read an image from an EXR file
///
/// There are a suite of classes for accessing image data, depending
/// on the level of complexity your application demands. This one is
/// perhaps a good starting point. There is a simpler one in \sa
/// RgbaInputFile, but that has very noted limitations of only being
/// for a 16-bit half, RGBA image. This class gives access to
/// arbitrary channels and data type outputs. It also will convert a
/// tiled image into a "normal" image, and simplify access if you only
/// want the first part, first image of a file.
///
/// If you will be accessing tiles (say for a renderer), working with
/// multi-part images, or reading deep data there are other classes
/// which provide API for handling that complexity more efficiently:
///
/// MultiPartInputFile
///   can be constructed but not directly accessible
///   - InputPart (the part-based class corresponding to this class)
///   - TiledInputPart
///   - DeepTiledInputPart
///   - DeepScanlineInputPart
///   - ScanlineInputPart [[[ NEW, but for consistency ]]]
/// TiledInputFile
/// DeepScanLineInputFile
/// DeepTiledInputFile
/// ScanLineInputFile
///
/// Of these, InputFile provide somewhat of a barrier to knowing what
/// the file actually contains, such that it allows you to read a file
/// as if it is scanlines, even if it is actually tiled under the
/// covers. Similar, a deep file is automatically composited for the
/// user. If a multi-part file is opened, the first part will be
/// provided.
///
/// For most code, it is suggested to use MultiPartInputFile and the
/// API provided by the relevant part classes, but if only a simple
/// API is needed, InputFile will certainly hide much of the
/// complexity.
class IMF_EXPORT_TYPE InputFile
{
public:
    //-------------------------------------------------------------
    // A constructor that attaches the new InputFile object to a
    // file that has already been opened.  Destroying the InputFile
    // object will not close the file.
    //
    // numThreads determines the number of threads that will be
    // used to read the file (see ImfThreading.h).
    //-------------------------------------------------------------

    IMF_EXPORT
    InputFile (
        OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is,
        int numThreads = globalThreadCount ());

    //-----------------------------------------------------------
    // A constructor that opens the file with the specified name.
    // Destroying the InputFile object will close the file.
    //
    // numThreads determines the number of threads that will be
    // used to read the file (see ImfThreading.h).
    //-----------------------------------------------------------
    IMF_EXPORT
    InputFile (const char filename[], int numThreads = globalThreadCount ());

    //-----------------------------------------------------------
    // A constructor that opens the file with the specified name
    // and context initialization routines
    // Destroying the InputFile object will close the file.
    //-----------------------------------------------------------
    IMF_EXPORT
    InputFile (
        const char*               filename,
        const ContextInitializer& ctxtinit,
        int                       numThreads = globalThreadCount ());

    //------------------------
    // Access to the file name
    //------------------------

    IMF_EXPORT
    const char* fileName () const;

    //--------------------------
    // Access to the file header
    //--------------------------

    IMF_EXPORT
    const Header& header () const;

    //----------------------------------
    // Access to the file format version
    //----------------------------------

    IMF_EXPORT
    int version () const;

    //-----------------------------------------------------------
    // Set the current frame buffer -- copies the FrameBuffer
    // object into the InputFile object.
    //
    // The current frame buffer is the destination for the pixel
    // data read from the file.  The current frame buffer must be
    // set at least once before readPixels() is called.
    // The current frame buffer can be changed after each call
    // to readPixels().
    //-----------------------------------------------------------

    IMF_EXPORT
    void setFrameBuffer (const FrameBuffer& frameBuffer);

    //-----------------------------------
    // Access to the current frame buffer
    //-----------------------------------

    IMF_EXPORT
    const FrameBuffer& frameBuffer () const;

    //---------------------------------------------------------------
    // Check if the file is complete:
    //
    // isComplete() returns true if all pixels in the data window are
    // present in the input file, or false if any pixels are missing.
    // (Another program may still be busy writing the file, or file
    // writing may have been aborted prematurely.)
    //---------------------------------------------------------------

    IMF_EXPORT
    bool isComplete () const;

    //---------------------------------------------------------------
    // Check if SSE optimization is enabled
    //
    // Call after setFrameBuffer() to query whether optimized file decoding
    // is available - decode times will be faster if returns true
    //
    // Optimization depends on:
    //   the file type (only scanline data is supported),
    //   the framebuffer channels (RGB/RGBA mono or stereo)
    //   the framebuffer channel types (all channels half-float format only)
    //   the file channels (RGB/RGBA mono or stereo)
    //   the file channel types (all channel half-float format only)
    //   whether SSE2 instruction support was detected at compile time
    //
    // Calling isOptimizationEnabled before setFrameBuffer will throw an exception
    //
    //---------------------------------------------------------------

    OPENEXR_DEPRECATED ("No longer meaningful")
    IMF_EXPORT
    bool isOptimizationEnabled () const;

    //---------------------------------------------------------------
    // Read pixel data:
    //
    // readPixels(s1,s2) reads all scan lines with y coordinates
    // in the interval [min (s1, s2), max (s1, s2)] from the file,
    // and stores them in the current frame buffer.
    //
    // Both s1 and s2 must be within the interval
    // [header().dataWindow().min.y, header().dataWindow().max.y]
    //
    // The scan lines can be read from the file in random order, and
    // individual scan lines may be skipped or read multiple times.
    // For maximum efficiency, the scan lines should be read in the
    // order in which they were written to the file.
    //
    // readPixels(s) calls readPixels(s,s).
    //
    //---------------------------------------------------------------

    IMF_EXPORT
    void readPixels (int scanLine1, int scanLine2);

    IMF_EXPORT
    void readPixels (int scanLine);

    //----------------------------------------------
    // Combines the setFrameBuffer and readPixels into a singular
    // call. This does more than that in that it can, with the right
    // conditions, not require a lock on the file, such that multiple
    // (external to OpenEXR) threads can read at the same time on
    // different framebuffers
    //
    // NB: if the underlying file is deep or tiled, that requires
    // translation, so will not do the pass through, but will behave
    // in a threadsafe manner (where the only way that was possible
    // before was to have a larger framebuffer, set the framebuffer
    // once, then call readPixels by the external threads, although
    // that occurred with a mutex and so the reads were serialized.
    // There are reasons why that might still be serialized, such as a
    // non-threadable stream.
    //----------------------------------------------

    IMF_EXPORT
    void readPixels (
        const FrameBuffer& frameBuffer, int scanLine1, int scanLine2);

    //----------------------------------------------
    // Read a block of raw pixel data from the file,
    // without uncompressing it (this function is
    // used to implement OutputFile::copyPixels()).
    //----------------------------------------------

    IMF_EXPORT
    void rawPixelData (
        int firstScanLine, const char*& pixelData, int& pixelDataSize);

    //----------------------------------------------
    // Read a scanline's worth of raw pixel data
    // from the file, without uncompressing it, and
    // store in an external buffer, pixelData.
    // pixelData should be pre-allocated with space
    // for pixelDataSize chars.
    //
    // This function can be used to separate the
    // reading of a raw scan line from the
    // decompression of that scan line, for
    // example to allow multiple scan lines to be
    // decompressed in parallel by an application's
    // own threads, where it is not convenient to
    // use the threading within the library.
    //----------------------------------------------

    IMF_EXPORT
    void rawPixelDataToBuffer (
        int scanLine, char* pixelData, int& pixelDataSize) const;

    //--------------------------------------------------
    // Read a tile of raw pixel data from the file,
    // without uncompressing it (this function is
    // used to implement TiledOutputFile::copyPixels()).
    //--------------------------------------------------

    IMF_EXPORT
    void rawTileData (
        int&         dx,
        int&         dy,
        int&         lx,
        int&         ly,
        const char*& pixelData,
        int&         pixelDataSize);

private:
    IMF_HIDDEN void initialize (void);

    // TODO: Remove these once MultiPartInputFile is converted
    IMF_HIDDEN InputFile (InputPartData* part);
    friend class MultiPartInputFile;

    // TODO: Remove these once TiledOutputFile is converted
    IMF_HIDDEN TiledInputFile& asTiledInput (void) const;
    friend class TiledOutputFile;

    Context _ctxt;
    struct Data;
    std::shared_ptr<Data> _data;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
