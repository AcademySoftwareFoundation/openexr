//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFDEEPTILEDINPUTPART_H_
#define IMFDEEPTILEDINPUTPART_H_

#include "ImfForward.h"

#include "ImfTileDescription.h"

#include <ImathBox.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class IMF_EXPORT_TYPE DeepTiledInputPart
{
  public:

    IMF_EXPORT
    DeepTiledInputPart(MultiPartInputFile& multiPartFile, int partNumber);

    //------------------------
    // Access to the file name
    //------------------------

    IMF_EXPORT
    const char *        fileName () const;


    //--------------------------
    // Access to the file header
    //--------------------------

    IMF_EXPORT
    const Header &      header () const;


    //----------------------------------
    // Access to the file format version
    //----------------------------------

    IMF_EXPORT
    int                 version () const;


    //-----------------------------------------------------------
    // Set the current frame buffer -- copies the FrameBuffer
    // object into the TiledInputFile object.
    //
    // The current frame buffer is the destination for the pixel
    // data read from the file.  The current frame buffer must be
    // set at least once before readTile() is called.
    // The current frame buffer can be changed after each call
    // to readTile().
    //-----------------------------------------------------------

    IMF_EXPORT
    void                setFrameBuffer (const DeepFrameBuffer &frameBuffer);


    //-----------------------------------
    // Access to the current frame buffer
    //-----------------------------------

    IMF_EXPORT
    const DeepFrameBuffer & frameBuffer () const;


    //------------------------------------------------------------
    // Check if the file is complete:
    //
    // isComplete() returns true if all pixels in the data window
    // (in all levels) are present in the input file, or false if
    // any pixels are missing.  (Another program may still be busy
    // writing the file, or file writing may have been aborted
    // prematurely.)
    //------------------------------------------------------------

    IMF_EXPORT
    bool                isComplete () const;


    //--------------------------------------------------
    // Utility functions:
    //--------------------------------------------------

    //---------------------------------------------------------
    // Multiresolution mode and tile size:
    // The following functions return the xSize, ySize and mode
    // fields of the file header's TileDescriptionAttribute.
    //---------------------------------------------------------

    IMF_EXPORT
    unsigned int        tileXSize () const;
    IMF_EXPORT
    unsigned int        tileYSize () const;
    IMF_EXPORT
    LevelMode           levelMode () const;
    IMF_EXPORT
    LevelRoundingMode   levelRoundingMode () const;


    //--------------------------------------------------------------------
    // Number of levels:
    //
    // numXLevels() returns the file's number of levels in x direction.
    //
    //  if levelMode() == ONE_LEVEL:
    //      return value is: 1
    //
    //  if levelMode() == MIPMAP_LEVELS:
    //      return value is: rfunc (log (max (w, h)) / log (2)) + 1
    //
    //  if levelMode() == RIPMAP_LEVELS:
    //      return value is: rfunc (log (w) / log (2)) + 1
    //
    //  where
    //      w is the width of the image's data window,  max.x - min.x + 1,
    //      y is the height of the image's data window, max.y - min.y + 1,
    //      and rfunc(x) is either floor(x), or ceil(x), depending on
    //      whether levelRoundingMode() returns ROUND_DOWN or ROUND_UP.
    //
    // numYLevels() returns the file's number of levels in y direction.
    //
    //  if levelMode() == ONE_LEVEL or levelMode() == MIPMAP_LEVELS:
    //      return value is the same as for numXLevels()
    //
    //  if levelMode() == RIPMAP_LEVELS:
    //      return value is: rfunc (log (h) / log (2)) + 1
    //
    //
    // numLevels() is a convenience function for use with
    // MIPMAP_LEVELS files.
    //
    //  if levelMode() == ONE_LEVEL or levelMode() == MIPMAP_LEVELS:
    //      return value is the same as for numXLevels()
    //
    //  if levelMode() == RIPMAP_LEVELS:
    //      an IEX_NAMESPACE::LogicExc exception is thrown
    //
    // isValidLevel(lx, ly) returns true if the file contains
    // a level with level number (lx, ly), false if not.
    //
    //--------------------------------------------------------------------

    IMF_EXPORT
    int                 numLevels () const;
    IMF_EXPORT
    int                 numXLevels () const;
    IMF_EXPORT
    int                 numYLevels () const;
    IMF_EXPORT
    bool                isValidLevel (int lx, int ly) const;


    //----------------------------------------------------------
    // Dimensions of a level:
    //
    // levelWidth(lx) returns the width of a level with level
    // number (lx, *), where * is any number.
    //
    //  return value is:
    //      max (1, rfunc (w / pow (2, lx)))
    //
    //
    // levelHeight(ly) returns the height of a level with level
    // number (*, ly), where * is any number.
    //
    //  return value is:
    //      max (1, rfunc (h / pow (2, ly)))
    //
    //----------------------------------------------------------

    IMF_EXPORT
    int                 levelWidth  (int lx) const;
    IMF_EXPORT
    int                 levelHeight (int ly) const;


    //--------------------------------------------------------------
    // Number of tiles:
    //
    // numXTiles(lx) returns the number of tiles in x direction
    // that cover a level with level number (lx, *), where * is
    // any number.
    //
    //  return value is:
    //      (levelWidth(lx) + tileXSize() - 1) / tileXSize()
    //
    //
    // numYTiles(ly) returns the number of tiles in y direction
    // that cover a level with level number (*, ly), where * is
    // any number.
    //
    //  return value is:
    //      (levelHeight(ly) + tileXSize() - 1) / tileXSize()
    //
    //--------------------------------------------------------------

    IMF_EXPORT
    int                 numXTiles (int lx = 0) const;
    IMF_EXPORT
    int                 numYTiles (int ly = 0) const;


    //---------------------------------------------------------------
    // Level pixel ranges:
    //
    // dataWindowForLevel(lx, ly) returns a 2-dimensional region of
    // valid pixel coordinates for a level with level number (lx, ly)
    //
    //  return value is a Box2i with min value:
    //      (dataWindow.min.x, dataWindow.min.y)
    //
    //  and max value:
    //      (dataWindow.min.x + levelWidth(lx) - 1,
    //       dataWindow.min.y + levelHeight(ly) - 1)
    //
    // dataWindowForLevel(level) is a convenience function used
    // for ONE_LEVEL and MIPMAP_LEVELS files.  It returns
    // dataWindowForLevel(level, level).
    //
    //---------------------------------------------------------------

    IMF_EXPORT
    IMATH_NAMESPACE::Box2i        dataWindowForLevel (int l = 0) const;
    IMF_EXPORT
    IMATH_NAMESPACE::Box2i        dataWindowForLevel (int lx, int ly) const;


    //-------------------------------------------------------------------
    // Tile pixel ranges:
    //
    // dataWindowForTile(dx, dy, lx, ly) returns a 2-dimensional
    // region of valid pixel coordinates for a tile with tile coordinates
    // (dx,dy) and level number (lx, ly).
    //
    //  return value is a Box2i with min value:
    //      (dataWindow.min.x + dx * tileXSize(),
    //       dataWindow.min.y + dy * tileYSize())
    //
    //  and max value:
    //      (dataWindow.min.x + (dx + 1) * tileXSize() - 1,
    //       dataWindow.min.y + (dy + 1) * tileYSize() - 1)
    //
    // dataWindowForTile(dx, dy, level) is a convenience function
    // used for ONE_LEVEL and MIPMAP_LEVELS files.  It returns
    // dataWindowForTile(dx, dy, level, level).
    //
    //-------------------------------------------------------------------

    IMF_EXPORT
    IMATH_NAMESPACE::Box2i        dataWindowForTile (int dx, int dy, int l = 0) const;

    IMF_EXPORT
    IMATH_NAMESPACE::Box2i        dataWindowForTile (int dx, int dy,
                                         int lx, int ly) const;

    //------------------------------------------------------------
    // Read pixel data:
    //
    // readTile(dx, dy, lx, ly) reads the tile with tile
    // coordinates (dx, dy), and level number (lx, ly),
    // and stores it in the current frame buffer.
    //
    //   dx must lie in the interval [0, numXTiles(lx)-1]
    //   dy must lie in the interval [0, numYTiles(ly)-1]
    //
    //   lx must lie in the interval [0, numXLevels()-1]
    //   ly must lie in the inverval [0, numYLevels()-1]
    //
    // readTile(dx, dy, level) is a convenience function used
    // for ONE_LEVEL and MIPMAP_LEVELS files.  It calls
    // readTile(dx, dy, level, level).
    //
    // The two readTiles(dx1, dx2, dy1, dy2, ...) functions allow
    // reading multiple tiles at once.  If multi-threading is used
    // the multiple tiles are read concurrently.
    //
    // Pixels that are outside the pixel coordinate range for the
    // tile's level, are never accessed by readTile().
    //
    // Attempting to access a tile that is not present in the file
    // throws an InputExc exception.
    //
    //------------------------------------------------------------

    IMF_EXPORT
    void                readTile  (int dx, int dy, int l = 0);
    IMF_EXPORT
    void                readTile  (int dx, int dy, int lx, int ly);

    IMF_EXPORT
    void                readTiles (int dx1, int dx2, int dy1, int dy2,
                                 int lx, int ly);

    IMF_EXPORT
    void                readTiles (int dx1, int dx2, int dy1, int dy2,
                                 int l = 0);


    //--------------------------------------------------
    // Read a tile of raw pixel data from the file,
    // without uncompressing it (this function is
    // used to implement TiledOutputFile::copyPixels()).
    //--------------------------------------------------

    IMF_EXPORT
    void                rawTileData (int &dx, int &dy,
                                   int &lx, int &ly,
                                   char *data,
                                   uint64_t &dataSize
                                   ) const;

    //------------------------------------------------------------------
    // Read pixel sample counts into a slice in the frame buffer.
    //
    // readPixelSampleCount(dx, dy, lx, ly) reads the sample counts
    // for tile (dx, dy) in level (lx, ly).
    //
    // readPixelSampleCount(dx, dy, l) calls
    // readPixelSampleCount(dx, dy, lx = l, ly = l)
    //
    // dx must lie in the interval [0, numXTiles(lx)-1]
    // dy must lie in the interval [0, numYTiles(ly)-1]
    //
    // lx must lie in the interval [0, numXLevels()-1]
    // ly must lie in the inverval [0, numYLevels()-1]
    //
    // readPixelSampleCounts(dx1, dx2, dy1, dy2, lx, ly) reads all
    // the sample counts for tiles within range
    // [(min(dx1, dx2), min(dy1, dy2))...(max(dx1, dx2), max(dy1, dy2)],
    // and on level (lx, ly)
    //
    // readPixelSampleCounts(dx1, dx2, dy1, dy2, l) calls
    // readPixelSampleCounts(dx1, dx2, dy1, dy2, lx = l, ly = l).
    //------------------------------------------------------------------

    IMF_EXPORT
    void                readPixelSampleCount  (int dx, int dy, int l = 0);
    IMF_EXPORT
    void                readPixelSampleCount  (int dx, int dy, int lx, int ly);

    IMF_EXPORT
    void                readPixelSampleCounts (int dx1, int dx2,
                                            int dy1, int dy2,
                                            int lx, int ly);

    IMF_EXPORT
    void                readPixelSampleCounts (int dx1, int dx2,
                                            int dy1, int dy2,
                                            int l = 0);

  private:
    DeepTiledInputFile* file;

    friend class DeepTiledOutputFile;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif /* IMFDEEPTILEDINPUTPART_H_ */
