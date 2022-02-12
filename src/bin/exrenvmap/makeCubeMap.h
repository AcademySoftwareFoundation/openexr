//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_MAKE_CUBE_MAP_H
#define INCLUDED_MAKE_CUBE_MAP_H

//-----------------------------------------------------------------------------
//
//	function makeCubeMap() -- makes cube-face environment maps
//
//-----------------------------------------------------------------------------

#include "namespaceAlias.h"
#include "readInputImage.h"

#include <ImfCompression.h>
#include <ImfTileDescription.h>

void makeCubeMap (
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

#endif
