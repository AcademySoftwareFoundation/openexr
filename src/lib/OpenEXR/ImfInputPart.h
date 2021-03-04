//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFINPUTPART_H_
#define IMFINPUTPART_H_

#include "ImfInputFile.h"
#include "ImfOutputPart.h"
#include "ImfForward.h"
#include "ImfNamespace.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

//-------------------------------------------------------------------
// class InputPart:
//
// Same interface as InputFile. Please refer to InputFile.
//-------------------------------------------------------------------

class InputPart
{
    public:
        IMF_EXPORT
        InputPart(MultiPartInputFile& multiPartFile, int partNumber);

        IMF_EXPORT
        const char *        fileName () const;
        IMF_EXPORT
        const Header &      header () const;
        IMF_EXPORT
        int                 version () const;
        IMF_EXPORT
        void                setFrameBuffer (const FrameBuffer &frameBuffer);
        IMF_EXPORT
        const FrameBuffer & frameBuffer () const;
        IMF_EXPORT
        bool                isComplete () const;
        IMF_EXPORT
        bool                isOptimizationEnabled () const;
        IMF_EXPORT
        void                readPixels (int scanLine1, int scanLine2);
        IMF_EXPORT
        void                readPixels (int scanLine);
        IMF_EXPORT
        void                rawPixelData (int firstScanLine,
                                          const char *&pixelData,
                                          int &pixelDataSize);

 
        IMF_EXPORT
        void                rawPixelDataToBuffer (int scanLine,
                                                  char *pixelData,
                                                  int &pixelDataSize) const;


        IMF_EXPORT
        void                rawTileData (int &dx, int &dy,
                                         int &lx, int &ly,
                                         const char *&pixelData,
                                         int &pixelDataSize);

    private:
        InputFile* file;
    // for internal use - give OutputFile and TiledOutputFile access to file for copyPixels
    friend void OutputFile::copyPixels(InputPart&);
    friend void TiledOutputFile::copyPixels(InputPart&);
    
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif /* IMFINPUTPART_H_ */
