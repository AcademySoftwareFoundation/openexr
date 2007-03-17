///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007, Industrial Light & Magic, a division of Lucas
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


#include <testBoxAlgo.h>
#include "ImathBoxAlgo.h"
#include "ImathRandom.h"
#include <iostream>
#include <algorithm>
#include <assert.h>


using namespace std;
using namespace Imath;

namespace {


bool
approximatelyEqual (const V3f &p1, const V3f &p2, float e)
{
    float m = 0;

    for (int i = 0; i < 3; ++i)
    {
	m = max (m, abs (p1[i]));
	m = max (m, abs (p2[i]));
    }

    for (int i = 0; i < 3; ++i)
	if (!equalWithAbsError (p1[i], p2[i], m * e))
	    return false;
    
    return true;
}


void
testRayBoxIntersection (const Box3f &box)
{
    Rand48 random (2007);

    float e = 50 * limits<float>::epsilon();

    if (box.isEmpty())
    {
	cout << "    empty box, no rays intersect" << endl;

	for (int i = 0; i < 100000; ++i)
	{
	    V3f p1 (random.nextf (box.max.x, box.min.x),
		    random.nextf (box.max.y, box.min.y),
		    random.nextf (box.max.z, box.min.z));

	    V3f p2 (p1 + hollowSphereRand<V3f> (random));

	    V3f ip;
	    assert (!intersects (box, Line3f (p1, p2), ip));
	}

	return;
    }

    cout << "    box = (" << box.min << " " << box.max << ")" << endl;

    if (box.max == box.min)
    {
	cout << "    single-point box, ray intersects" << endl;

	for (int i = 0; i < 100000; ++i)
	{
	    V3f p1 (random.nextf (box.max.x, box.min.x),
		    random.nextf (box.max.y, box.min.y),
		    random.nextf (box.max.z, box.min.z));

	    V3f ip;
	    assert (intersects (box, Line3f (p1, box.min), ip));
	    assert (ip == box.min);
	}

	cout << "    single-point box, ray does not intersect" << endl;

	for (int i = 0; i < 100000; ++i)
	{
	    //
	    // The ray starts at a distance of r2 from the of the
	    // box, and it passes the box at a minimum distance of r1.
	    //

	    const float r1 = 0.00001;
	    const float r2 = 1.0;

	    V3f p1 = box.min + r2 * hollowSphereRand<V3f> (random);
	    V3f p2;
	    float r3;

	    do
	    {
		do 
		{
		    p2 = box.min + r2 * hollowSphereRand<V3f> (random);
		}
		while (approximatelyEqual (p1, p2, e));

		V3f d1 = (p2 - p1).normalized();
		V3f d2 = (box.min - p1);
		r3 = (d2 - d1 * (d1 ^ d2)).length();
	    }
	    while (r3 < r1);

	    Line3f ray (p1, p2);
	    V3f ip;

	    assert (!intersects (box, ray, ip));
	}

	return;
    }

    cout << "    ray starts inside box" << endl;

    for (int i = 0; i < 1000; ++i)
    {
	V3f p1 (random.nextf (box.min.x, box.max.x),
	        random.nextf (box.min.y, box.max.y),
		random.nextf (box.min.z, box.max.z));

	V3f p2 (p1 + hollowSphereRand<V3f> (random));

	V3f ip;
	bool b = intersects (box, Line3f (p1, p2), ip);

	assert (b && ip == p1);
    }

    cout << "    ray starts outside box, intersects" << endl;

    Box3f bigBox (box.min - (box.max - box.min),
                  box.max + (box.max - box.min));

    for (int i = 0; i < 100000; ++i)
    {
	//
	// Find starting point outside the box, end point inside the box
	//

	V3f p1;

	do
	{
	    p1 = V3f (random.nextf (bigBox.min.x, bigBox.max.x),
		      random.nextf (bigBox.min.y, bigBox.max.y),
		      random.nextf (bigBox.min.z, bigBox.max.z));
	}
	while (box.intersects (p1));

	V3f p2;
	
	do
	{
	    p2 = V3f (random.nextf (box.min.x, box.max.x),
		      random.nextf (box.min.y, box.max.y),
		      random.nextf (box.min.z, box.max.z));
	}
	while (approximatelyEqual (p1, p2, e));

	Line3f ray (p1, p2);

	V3f ip;
	bool b = intersects (box, ray, ip);

	//
	// Ray and box must intersect, intersection point
	// must be on the surface of the box.
	//

	assert (b);

	assert (ip.x == box.min.x || ip.x == box.max.x ||
		ip.y == box.min.y || ip.y == box.max.y ||
		ip.z == box.min.z || ip.z == box.max.z);

	//
	// Intersection point must be consistent with the origin
	// and direction of the ray
	//

	if (ip.x == box.min.x)
	    assert (ray.pos.x <= box.min.x && ray.dir.x >= 0);

	if (ip.x == box.max.x)
	    assert (ray.pos.x >= box.max.x && ray.dir.x <= 0);

	if (ip.y == box.min.y)
	    assert (ray.pos.y <= box.min.y && ray.dir.y >= 0);

	if (ip.y == box.max.y)
	    assert (ray.pos.y >= box.max.y && ray.dir.y <= 0);

	if (ip.z == box.min.z)
	    assert (ray.pos.z <= box.min.z && ray.dir.z >= 0);

	if (ip.z == box.max.z)
	    assert (ray.pos.z >= box.max.z && ray.dir.z <= 0);

	//
	// Intersection point must be approximately on the ray
	// How far it can be off depends on how far p1 and ip
	// are from the origin.
	//

	V3f p3 = p1 + ray.dir * (ray.dir ^ (ip - p1));
	float m = 0;

	for (int j = 0; j < 3; ++j)
	{
	    m = max (abs (p1[j]), m);
	    m = max (abs (ip[j]), m);
	}

	float err = 30 * m * limits<float>::epsilon();
	assert (p3.equalWithAbsError (ip, err));

	//
	// Try same starting point, opposite direction
	// 

	ray.dir *= -1;
	V3f ip2;

	assert (!intersects (box, ray, ip2));
    }

    cout << "    ray starts outside box, does not intersect" << endl;

    V3f center = (box.min + box.max) * 0.5f;
    float r1 = (box.max - box.min).length() * 0.51f;
    float r2 = 2 * r1;

    for (int i = 0; i < 100000; ++i)
    {
	//
	// The ray starts at a distance of r2 from the center
	// of the box, and it passes the center at a minimum
	// distance of r1.  (r1 and r2 are both greater than
	// the distance between the center and the corners
	// of the box.)
	//

	V3f p1 = center + r2 * hollowSphereRand<V3f> (random);
	V3f p2;
	float r3;

	do
	{
	    do 
	    {
		p2 = center + r2 * hollowSphereRand<V3f> (random);
	    }
	    while (approximatelyEqual (p1, p2, e));

	    V3f d1 = (p2 - p1).normalized();
	    V3f d2 = (center - p1);
	    r3 = (d2 - d1 * (d1 ^ d2)).length();
	}
	while (r3 < r1);

	Line3f ray (p1, p2);
	V3f ip;

	assert (!intersects (box, ray, ip));
    }
}


void
rayBoxIntersection1 ()
{
    cout << "  ray-box intersection, random rays" << endl;

    Box3f boxes[] =
    {
	// Boxes with a positive volume

	Box3f (V3f (-1, -1, -1), V3f (1, 1, 1)),
	Box3f (V3f (10, 20, 30), V3f (1010, 21, 31)),
	Box3f (V3f (10, 20, 30), V3f (11, 1020, 31)),
	Box3f (V3f (10, 20, 30), V3f (11, 21, 1030)),
	Box3f (V3f (-1e10f, -2e10f, -3e10f), V3f (5e15f, 6e15f, 7e15f)),

	// Non-empty, zero-volume boxes

	Box3f (V3f (1, 1, 1), V3f (2, 1, 1)),
	Box3f (V3f (1, 1, 1), V3f (1, 2, 1)),
	Box3f (V3f (1, 1, 1), V3f (1, 1, 2)),
	Box3f (V3f (1, 1, 1), V3f (1, 2, 3)),
	Box3f (V3f (1, 1, 1), V3f (2, 3, 1)),
	Box3f (V3f (1, 1, 1), V3f (2, 1, 3)),
	Box3f (V3f (-1, -2, 1), V3f (-1, -2, 1)),
	Box3f (V3f (1, 1, 1), V3f (1, 1, 1)),
	Box3f (V3f (0, 0, 0), V3f (0, 0, 0)),

	// empty box

	Box3f ()
    };

    for (int i = 0; i < sizeof (boxes) / sizeof (boxes[0]); ++i)
	testRayBoxIntersection (boxes[i]);
}


void
testPerturbedRayBox
    (const Box3f &box,
     const Line3f &ray,
     bool result)
{
    cout << "    dir ~ " << ray.dir << ", result = " << result << endl;

    Rand48 random (19);
    const float e = 1e-25f;

    for (int i = 0; i < 10000; ++i)
    {
	Line3f r (ray);
	r.dir += e * solidSphereRand<V3f> (random);

	V3f ip;
	assert (result == intersects (box, ray, ip));
    }
}


void
rayBoxIntersection2 ()
{

    cout << "  ray-box intersection, nearly axis-parallel rays" << endl;

    Box3f box (V3f (-1e15f, -1e15f, -1e15f), V3f (1e15f, 1e15f, 1e15f));
    Line3f ray;
    V3f ip;
    bool b;

    ray = Line3f (V3f (-2e15f, 0, 0), V3f (2e15f, 0, 0));
    b = intersects (box, ray, ip);
    assert (b && ip == V3f (-1e15f, 0, 0));
    testPerturbedRayBox (box, ray, true);

    ray = Line3f (V3f (2e15f, 0, 0), V3f (-2e15f, 0, 0));
    b = intersects (box, ray, ip);
    assert (b && ip == V3f (1e15f, 0, 0));
    testPerturbedRayBox (box, ray, true);

    ray = Line3f (V3f (-2e15f, 2e15f, 0), V3f (2e15f, 2e15f, 0));
    b = intersects (box, ray, ip);
    assert (!b);
    testPerturbedRayBox (box, ray, false);

    ray = Line3f (V3f (2e15f, 2e15f, 0), V3f (-2e15f, 2e15f, 0));
    b = intersects (box, ray, ip);
    assert (!b);
    testPerturbedRayBox (box, ray, false);

    ray = Line3f (V3f (0, -2e15f, 0), V3f (0, 2e15f, 0));
    b = intersects (box, ray, ip);
    assert (b && ip == V3f (0, -1e15f, 0));
    testPerturbedRayBox (box, ray, true);

    ray = Line3f (V3f (0, 2e15f, 0), V3f (0, -2e15f, 0));
    b = intersects (box, ray, ip);
    assert (b && ip == V3f (0, 1e15f, 0));
    testPerturbedRayBox (box, ray, true);

    ray = Line3f (V3f (0, -2e15f, 2e15f), V3f (0, 2e15f, 2e15f));
    b = intersects (box, ray, ip);
    assert (!b);
    testPerturbedRayBox (box, ray, false);

    ray = Line3f (V3f (0, 2e15f, 2e15f), V3f (0, -2e15f, 2e15f));
    b = intersects (box, ray, ip);
    assert (!b);
    testPerturbedRayBox (box, ray, false);

    ray = Line3f (V3f (0, 0, -2e15f), V3f (0, 0, 2e15f));
    b = intersects (box, ray, ip);
    assert (b && ip == V3f (0, 0, -1e15f));
    testPerturbedRayBox (box, ray, true);

    ray = Line3f (V3f (0, 0, 2e15f), V3f (0, 0, -2e15f));
    b = intersects (box, ray, ip);
    assert (b && ip == V3f (0, 0, 1e15f));
    testPerturbedRayBox (box, ray, true);

    ray = Line3f (V3f (2e15f, 0, -2e15f), V3f (2e15f, 0, 2e15f));
    b = intersects (box, ray, ip);
    assert (!b);
    testPerturbedRayBox (box, ray, false);

    ray = Line3f (V3f (2e15f, 0, 2e15f), V3f (2e15f, 0, -2e15f));
    b = intersects (box, ray, ip);
    assert (!b);
    testPerturbedRayBox (box, ray, false);
}


Box3f
transformSimple (const Box3f &b, const M44f &M)
{
    Box3f b1;

    for (int i = 0; i < 8; ++i)
    {
	V3f p;

	for (int j = 0; j < 3; ++j)
	    p[j] = ((i >> j) & 1)? b.max[j]: b.min[j];

	b1.extendBy (p * M);
    }

    return b1;
}


void
boxMatrixTransform ()
{
    cout << "  transform box by matrix" << endl;

    const float e = 5 * limits<float>::epsilon();

    Box3f b1 (V3f (4, 5, 6), V3f (7, 8, 9));

    M44f M;
    M.setEulerAngles (V3f (1, 2, 3));
    M.translate (V3f (20, -15, 2));

    Box3f b2 = transform (b1, M);
    Box3f b3 = transformSimple (b1, M);

    assert (approximatelyEqual (b2.min, b3.min, e));
    assert (approximatelyEqual (b2.max, b3.max, e));

    M[0][3] = 1;
    M[1][3] = 2;
    M[2][3] = 3;
    M[3][3] = 4;

    Box3f b4 = transform (b1, M);
    Box3f b5 = transformSimple (b1, M);

    assert (approximatelyEqual (b4.min, b5.min, e));
    assert (approximatelyEqual (b4.max, b5.max, e));
}


} // namespace


void
testBoxAlgo ()
{
    cout << "Testing box algorithms" << endl;

    rayBoxIntersection1();
    rayBoxIntersection2();
    boxMatrixTransform();

    cout << "ok\n" << endl;
}
