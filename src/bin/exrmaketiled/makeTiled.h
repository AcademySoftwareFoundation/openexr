//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_MAKE_TILED_H
#define INCLUDED_MAKE_TILED_H

//----------------------------------------------------------------------------
//
//	Produce a tiled version of an OpenEXR image.
//
//----------------------------------------------------------------------------

#include <ImfPartType.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfMultiPartInputFile.h>
#include <ImfTileDescription.h>
#include <ImfCompression.h>
#include <OpenEXRConfig.h>

#include <string>
#include <set>

#include "namespaceAlias.h"

enum Extrapolation
{
    BLACK,
    CLAMP,
    PERIODIC,
    MIRROR
};


void    makeTiled (const char inFileName[],
                   const char outFileName[],
                   int partnum,
                   IMF::LevelMode mode,
                   IMF::LevelRoundingMode roundingMode,
                   IMF::Compression compression,
                   int tileSizeX,
                   int tileSizeY,
                   const std::set<std::string> &doNotFilter,
                   Extrapolation extX,
                   Extrapolation extY,
                   bool verbose);


#endif
