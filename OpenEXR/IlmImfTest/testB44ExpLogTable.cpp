//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.
//

#include <ImfNamespace.h>
#include <half.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <assert.h>

using namespace std;


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

extern const unsigned short expTable[];
extern const unsigned short logTable[];

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT

void
testB44ExpLogTable (const string&)
{
    const int iMax = (1 << 16);

    for (int i = 0; i < iMax; i++)
    {
	half h;
	h.setBits (i);

	if (!h.isFinite())
	    h = 0;
	else if (h >= 8 * log (HALF_MAX))
	    h = HALF_MAX;
	else
	    h = exp (h / 8);

        assert (OPENEXR_IMF_INTERNAL_NAMESPACE::expTable[i] == h.bits());
    }

    for (int i = 0; i < iMax; i++)
    {
	half h;
	h.setBits (i);

	if (!h.isFinite() || h < 0)
	    h = 0;
	else
	    h = 8 * log (h);

        assert (OPENEXR_IMF_INTERNAL_NAMESPACE::logTable[i] == h.bits());
    }
}
