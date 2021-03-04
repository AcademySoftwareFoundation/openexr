//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_TILE_OFFSETS_H
#define INCLUDED_IMF_TILE_OFFSETS_H

//-----------------------------------------------------------------------------
//
//	class TileOffsets
//
//-----------------------------------------------------------------------------

#include "ImfTileDescription.h"
#include <vector>
#include "ImfNamespace.h"
#include "ImfForward.h"
#include "ImfExport.h"

#include <cstdint>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class TileOffsets
{
  public:

    IMF_EXPORT
    TileOffsets (LevelMode mode = ONE_LEVEL,
		 int numXLevels = 0,
		 int numYLevels = 0,
		 const int *numXTiles = 0,
		 const int *numYTiles = 0);    

    // --------
    // File I/O
    // --------

    IMF_EXPORT
    void		readFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is,  bool &complete,bool isMultiPart,bool isDeep);
    IMF_EXPORT
    void        readFrom (std::vector<uint64_t> chunkOffsets,bool &complete);
    IMF_EXPORT
    uint64_t		writeTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os) const;


    //-----------------------------------------------------------
    // Test if the tileOffsets array is empty (all entries are 0)
    //-----------------------------------------------------------

    IMF_EXPORT
    bool		isEmpty () const;
    
    
    
    //-----------------------------------------------------------
    // populate 'list' with tiles coordinates in the order they appear
    // in the offset table (assumes full table!
    // each array myst be at leat totalTiles long
    //-----------------------------------------------------------
    IMF_EXPORT
    void getTileOrder(int dx_table[], int dy_table[], int lx_table[], int ly_table[]) const;
    
    
    //-----------------------
    // Access to the elements
    //-----------------------

    IMF_EXPORT
    uint64_t &		operator () (int dx, int dy, int lx, int ly);
    IMF_EXPORT
    uint64_t &		operator () (int dx, int dy, int l);
    IMF_EXPORT
    const uint64_t &	operator () (int dx, int dy, int lx, int ly) const;
    IMF_EXPORT
    const uint64_t &	operator () (int dx, int dy, int l) const;
    IMF_EXPORT
    bool        isValidTile (int dx, int dy, int lx, int ly) const;
    IMF_EXPORT
    const std::vector<std::vector<std::vector <uint64_t> > >& getOffsets() const;
    
  private:

    void		findTiles (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, bool isMultiPartFile,
                                   bool isDeep,
        		           bool skipOnly);
    void		reconstructFromFile (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is,bool isMultiPartFile,bool isDeep);
    bool		readTile (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is);
    bool		anyOffsetsAreInvalid () const;

    LevelMode		_mode;
    int			_numXLevels;
    int			_numYLevels;

    std::vector<std::vector<std::vector <uint64_t> > > _offsets;
    
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT





#endif
