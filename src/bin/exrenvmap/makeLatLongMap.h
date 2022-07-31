//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_MAKE_LATLONG_MAP_H
#define INCLUDED_MAKE_LATLONG_MAP_H

//-----------------------------------------------------------------------------
//
//	function makeLatLongMap() -- makes latitude-longitude environment maps
//
//-----------------------------------------------------------------------------

#include "namespaceAlias.h"
#include <ImfCompression.h>
#include <ImfTileDescription.h>
#include <readInputImage.h>

void makeLatLongMap (
    EnvmapImage&           image,
    IMF::Header&           header,
    IMF::RgbaChannels      channels,
    const char             outFileName[],
    int                    tileWidth,
    int                    tileHeight,
    IMF::LevelMode         levelMode,
    IMF::LevelRoundingMode roundingMode,
    IMF::Compression       compression,
    int                    mapWidth,
    float                  filterRadius,
    int                    numSamples,
    bool                   verbose);

#endif // INCLUDED_MAKE_LATLONG_MAP_H
