//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFGENERICOUTPUTFILE_H_
#define IMFGENERICOUTPUTFILE_H_

#include "ImfVersion.h"
#include "ImfIO.h"
#include "ImfXdr.h"
#include "ImfHeader.h"
#include "ImfNamespace.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class GenericOutputFile
{
    public:
        IMF_EXPORT
        virtual ~GenericOutputFile();

    protected:
        IMF_EXPORT
        GenericOutputFile();
        IMF_EXPORT
        void writeMagicNumberAndVersionField (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os, const Header& header);
        IMF_EXPORT
        void writeMagicNumberAndVersionField (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os, const Header * headers, int parts);
  
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif /* GENERICOUTPUTFILE_H_ */
