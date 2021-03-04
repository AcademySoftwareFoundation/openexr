//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFPARTTYPE_H_
#define IMFPARTTYPE_H_

#include <string>
#include "ImfNamespace.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


const std::string SCANLINEIMAGE = "scanlineimage";
const std::string TILEDIMAGE    = "tiledimage";
const std::string DEEPSCANLINE  = "deepscanline";
const std::string DEEPTILE      = "deeptile";

IMF_EXPORT bool isImage(const std::string& name);

IMF_EXPORT bool isTiled(const std::string& name);

IMF_EXPORT bool isDeepData(const std::string& name);

IMF_EXPORT bool isSupportedType(const std::string& name);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif /* IMFPARTTYPE_H_ */
