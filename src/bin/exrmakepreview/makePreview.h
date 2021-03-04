//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_MAKE_PREVIEW_H
#define INCLUDED_MAKE_PREVIEW_H

//----------------------------------------------------------------------------
//
//	Add a preview image to an OpenEXR file.
//
//----------------------------------------------------------------------------


void	makePreview (const char inFileName[],
	             const char outFileName[],
		     int previewWidth,
		     float exposure,
		     bool verbose);


#endif
