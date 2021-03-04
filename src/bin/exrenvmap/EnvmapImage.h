//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_ENVMAP_IMAGE_H
#define INCLUDED_ENVMAP_IMAGE_H

//-----------------------------------------------------------------------------
//
//        class EnvmapImage
//
//-----------------------------------------------------------------------------

#include "namespaceAlias.h"

#include <ImfArray.h>
#include <ImfRgba.h>
#include <ImfEnvmap.h>
#include <ImathBox.h>



class EnvmapImage
{
  public:

      EnvmapImage ();
      EnvmapImage (IMF::Envmap type, const IMATH::Box2i &dataWindow);
      
      void                      resize (IMF::Envmap type,
                                        const IMATH::Box2i &dataWindow);

      void                      clear ();

      IMF::Envmap               type () const;
      const IMATH::Box2i &      dataWindow () const;

      IMF::Array2D<IMF::Rgba> & pixels ();
      const IMF::Array2D<IMF::Rgba> &
                                pixels () const;
      
      IMF::Rgba                 filteredLookup (IMATH::V3f direction,
                                                float radius,
                                                int numSamples) const;

  private:
      
      IMF::Rgba                 sample (const IMATH::V2f &pos) const;

      IMF::Envmap               _type;
      IMATH::Box2i              _dataWindow;
      IMF::Array2D<IMF::Rgba>   _pixels;
};


#endif
