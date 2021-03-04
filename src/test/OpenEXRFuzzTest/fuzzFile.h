//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_FUZZ_FILE_H
#define INCLUDED_FUZZ_FILE_H

#include <ImathRandom.h>


void
fuzzFile (const char goodFile[],
          const char brokenFile[],
	  void (*readFile) (const char[]),
	  int nSlidingWindow,
	  int nFixedWindow,
	  IMATH_NAMESPACE::Rand48 &random);


#endif
