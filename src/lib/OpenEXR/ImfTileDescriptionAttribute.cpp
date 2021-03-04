//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class TileDescriptionAttribute
//
//-----------------------------------------------------------------------------

#include <ImfTileDescriptionAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
TileDescriptionAttribute::staticTypeName ()
{
    return "tiledesc";
}


template <>
void
TileDescriptionAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.xSize);
    Xdr::write <StreamIO> (os, _value.ySize);

    unsigned char tmp = _value.mode | (_value.roundingMode << 4);
    Xdr::write <StreamIO> (os, tmp);
}


template <>
void
TileDescriptionAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is,
					 int size,
					 int version)
{
    Xdr::read <StreamIO> (is, _value.xSize);
    Xdr::read <StreamIO> (is, _value.ySize);

    unsigned char tmp;
    Xdr::read <StreamIO> (is, tmp);

    //
    // four bits are allocated for 'mode' for future use (16 possible values)
    // but only values 0,1,2 are currently valid. '3' is a special valid enum value
    // that indicates bad values have been used
    //
    // roundingMode can only be 0 or 1, and 2 is a special enum value for 'bad enum'
    //
    unsigned char levelMode = tmp & 0x0f;
    if(levelMode > 3)
    {
        levelMode = 3;
    }

    _value.mode = LevelMode(levelMode);

    unsigned char levelRoundingMode = (tmp >> 4) & 0x0f;
    if(levelRoundingMode > 2)
    {
        levelRoundingMode = 2;
    }

    _value.roundingMode = LevelRoundingMode (levelRoundingMode);
    
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
