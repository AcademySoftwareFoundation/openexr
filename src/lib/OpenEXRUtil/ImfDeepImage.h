//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_DEEP_IMAGE_H
#define INCLUDED_IMF_DEEP_IMAGE_H

//----------------------------------------------------------------------------
//
//      class DeepImage
//
//      For an explanation of images, levels and channels,
//      see the comments in header file Image.h.
//
//----------------------------------------------------------------------------

#include "ImfDeepImageLevel.h"
#include "ImfImage.h"
#include "ImfUtilExport.h"

#include <ImfTileDescription.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class IMFUTIL_EXPORT DeepImage : public Image
{
  public:

    //
    // Constructors and destructor.
    // The default constructor constructs an image with an empty data
    // window level mode ONE_LEVEL and level rounding mode ROUND_DOWN.
    //

    DeepImage();

  	DeepImage(const IMATH_NAMESPACE::Box2i &dataWindow,
               LevelMode levelMode = ONE_LEVEL,
               LevelRoundingMode levelRoundingMode = ROUND_DOWN);

  	virtual ~DeepImage();


    //
    // Accessing image levels by level number
    //

    virtual DeepImageLevel &        level(int l = 0);
    virtual const DeepImageLevel &  level(int l = 0) const;

    virtual DeepImageLevel &        level(int lx, int ly);
    virtual const DeepImageLevel &  level(int lx, int ly) const;

  protected:

  	virtual DeepImageLevel *
        newLevel (int lx, int ly, const IMATH_NAMESPACE::Box2i &dataWindow);
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
