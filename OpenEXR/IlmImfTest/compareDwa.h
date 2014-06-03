// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2009-2014 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#ifndef COMPARE_DWA_H_INCLUDED
#define COMPARE_DWA_H_INCLUDED

#include "ImfRgba.h"
#include "ImfArray.h"
#include "ImfNamespace.h"

void compareDwa(int width, 
                int height,
                const OPENEXR_IMF_NAMESPACE::Array2D<Imf::Rgba> &src,
                const OPENEXR_IMF_NAMESPACE::Array2D<Imf::Rgba> &test,
                OPENEXR_IMF_NAMESPACE::RgbaChannels channels);

#endif



// TM and (c) 2009-2014 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
