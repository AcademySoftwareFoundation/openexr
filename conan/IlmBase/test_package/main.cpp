///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#include "ImathMatrix.h"
#include "ImathMatrixAlgo.h"
#include "ImathVec.h"
#include "ImathLimits.h"
#include "ImathMath.h"
#include "ImathInt64.h"
#include "ImathRandom.h"
#include <iostream>
#include <assert.h>
#include <string.h>

using namespace std;
using IMATH_INTERNAL_NAMESPACE::Int64;

int
main (int argc, char *argv[])
{
	cout << "M33d constructors and equality operators" << endl;

	IMATH_INTERNAL_NAMESPACE::M33d m2;
	m2[0][0] = 99.0f;
	m2[1][2] = 101.0f;

	IMATH_INTERNAL_NAMESPACE::M33d test(m2);
	assert(test == m2);

	IMATH_INTERNAL_NAMESPACE::M33d test2;
	assert(test != test2);

	IMATH_INTERNAL_NAMESPACE::M33d test3;
	test3.makeIdentity();
	assert(test2 == test3);

    cout << "ok\n" << endl;

    return 0;
}
