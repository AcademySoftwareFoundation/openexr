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



#include <testMatrix.h>
#include "ImathMatrix.h"
#include "ImathVec.h"
#include "ImathLimits.h"
#include "ImathMath.h"
#include "ImathInt64.h"
#include <iostream>
#include <assert.h>


using namespace std;
using Imath::Int64;


//
// This file is not currently intended to exhaustively test
// the Imath Matrix33<T> and Matrix44<T> classes.  We leave
// that to PyImathTest.
//
// Instead, in this file we test only those aspects of the
// Imath Matrix33<T> and Matrix44<T> classes that must be 
// or are more convenient to test from C++.
//

void
testMatrix ()
{
    cout << "Testing functions in ImathMatrix.h" << endl;

    union {float f; int i;} nanf;
    nanf.i = 0x7f800001; //  NAN

    union {double d; Int64 i;} nand;
    nand.i = 0x7ff0000000000001ULL; //  NAN

    {
	cout << "Imath::M33f shear functions" << endl;

	Imath::M33f m1, m2;
	m1.setShear (2.0f);
	assert
	   (m1[0][0] == 1.0f  &&  m1[0][1] == 0.0f  &&  m1[0][2] == 0.0f  &&
	    m1[1][0] == 2.0f  &&  m1[1][1] == 1.0f  &&  m1[1][2] == 0.0f  &&
	    m1[2][0] == 0.0f  &&  m1[2][1] == 0.0f  &&  m1[2][2] == 1.0f);

	m2.setShear (Imath::V2f (3.0f, 4.0f));
	assert
	   (m2[0][0] == 1.0f  &&  m2[0][1] == 4.0f  &&  m2[0][2] == 0.0f  &&
	    m2[1][0] == 3.0f  &&  m2[1][1] == 1.0f  &&  m2[1][2] == 0.0f  &&
	    m2[2][0] == 0.0f  &&  m2[2][1] == 0.0f  &&  m2[2][2] == 1.0f);


	m1.shear (Imath::V2f (5.0f, 6.0f));
	assert
	   (m1[0][0] == 13.0f  &&  m1[0][1] == 6.0f  &&  m1[0][2] == 0.0f  &&
	    m1[1][0] ==  7.0f  &&  m1[1][1] == 1.0f  &&  m1[1][2] == 0.0f  &&
	    m1[2][0] ==  0.0f  &&  m1[2][1] == 0.0f  &&  m1[2][2] == 1.0f);

	m2.shear (7.0f);
	assert
	   (m2[0][0] ==  1.0f  &&  m2[0][1] ==  4.0f  &&  m2[0][2] == 0.0f  &&
	    m2[1][0] == 10.0f  &&  m2[1][1] == 29.0f  &&  m2[1][2] == 0.0f  &&
	    m2[2][0] ==  0.0f  &&  m2[2][1] ==  0.0f  &&  m2[2][2] == 1.0f);

	cout << "M33f constructors and equality operators" << endl;

	Imath::M33f test(m2);
	assert(test == m2);

	Imath::M33f test2;
	assert(test != test2);

	Imath::M33f test3;
	test3.makeIdentity();
	assert(test2 == test3);
    }

    {
	cout << "M33d constructors and equality operators" << endl;

	Imath::M33d m2;
	m2[0][0] = 99.0f;
	m2[1][2] = 101.0f;

	Imath::M33d test(m2);
	assert(test == m2);

	Imath::M33d test2;
	assert(test != test2);

	Imath::M33d test3;
	test3.makeIdentity();
	assert(test2 == test3);
    }

    {
	Imath::M44f m2;
	m2[0][0] = 99.0f;
	m2[1][2] = 101.0f;

	cout << "M44f constructors and equality operators" << endl;

	Imath::M44f test(m2);
	assert(test == m2);

	Imath::M44f test2;
	assert(test != test2);

	Imath::M44f test3;
	test3.makeIdentity();
	assert(test2 == test3);

	//
	// Test non-equality when a NAN is in the same
	// place in two identical matrices
	//

	test2[0][0] = nanf.f;
	test3 = test2;
	assert(test2 != test3);
    }

    {
	Imath::M44d m2;
	m2[0][0] = 99.0f;
	m2[1][2] = 101.0f;

	cout << "M44d constructors and equality operators" << endl;

	Imath::M44d test(m2);
	assert(test == m2);

	Imath::M44d test2;
	assert(test != test2);

	Imath::M44d test3;
	test3.makeIdentity();
	assert(test2 == test3);

	//
	// Test non-equality when a NAN is in the same
	// place in two identical matrices
	//

	test2[0][0] = nand.d;
	test3 = test2;
	assert(test2 != test3);
    }

    {
	cout << "Converting between M33 and M44" << endl;

	Imath::M44d m1;
	m1[0][0] = 99;
	Imath::M44f m2;
	m2.setValue(m1);
	assert(m2[0][0] == (float)m1[0][0]);
	m1[0][0] = 101;
	m1.setValue(m2);
	assert(m2[0][0] == (float)m1[0][0]);
    }

    cout << "ok\n" << endl;
}
