///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2010, Industrial Light & Magic, a division of Lucas
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
#include <typeinfo>
#include <vector>

using namespace std;
using namespace Imath;

namespace {

//
// Test case generation utility - create a vector of Imath::Vec{2,3,4}
// with all permutations of integers 1..T::dimensions().
//
// Algorithm from www.bearcave.com/random_hacks/permute.html
//
template <class T>
static void
addItem(const std::vector<int> &value, std::vector<T> &perms)
{
    T p;
    for (unsigned int i = 0; i < value.size(); i++)
    {
        p[i] = value[i];
    }
    perms.push_back(p);
}

template <class T>
static void
visit(int &level, int n, int k, std::vector<int> &value, std::vector<T> &perms)
{
    level = level + 1;
    value[k] = level;

    if (level == n)
        addItem(value, perms);
    else
        for (int i = 0; i < n; i++)
            if (value[i] == 0)
                visit(level, n, i, value, perms);

    level = level - 1;
    value[k] = 0;
}


template <class T>
static void
permutations(std::vector<T> &perms)
{
    std::vector<int> value(T::dimensions());
    int level = -1;
    int n     = T::dimensions();

    visit(level, n, 0, value, perms);
}

template <class T>
static void
testConstructors(const char *type)
{
    cout << "    constructors for type " << type << endl;

    //
    // Empty
    //
    {
        Imath::Box<T> b;
        assert(b.min == T(T::baseTypeMax()) &&
               b.max == T(T::baseTypeMin()));
    }

    //
    // Single point
    //
    {
        T p;
        for (unsigned int i = 0; i < T::dimensions(); i++)
            p[i] = i;

        Imath::Box<T> b(p);
        assert(b.min == p && b.max == p);
    }

    //
    // Min and max
    //
    {
        T p0;
        T p1;
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
            p0[i] = i;
            p1[i] = 10 * T::dimensions() - i - 1;
        }

        Imath::Box<T> b(p0, p1);
        assert(b.min == p0 && b.max == p1);
    }
}

template <class T>
void
testMakeEmpty(const char *type)
{
    cout << "    makeEmpty() for type " << type << endl;

    //
    // Empty box
    //
    {
        Imath::Box<T> b;
        b.makeEmpty();
        assert(b.min == T(T::baseTypeMax()) &&
               b.max == T(T::baseTypeMin()));
    }

    //
    // Non-empty, has volume
    //
    {
        Imath::Box<T> b(T(-1), T(1));
        b.makeEmpty();
        assert(b.min == T(T::baseTypeMax()) &&
               b.max == T(T::baseTypeMin()));
    }

    //
    // Non-empty, no volume
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        T min(0);
        T max(0);
        max[T::dimensions() - 1] = 1;

        Imath::Box<T> b(min, max);
        b.makeEmpty();
        assert(b.min == T(T::baseTypeMax()) &&
               b.max == T(T::baseTypeMin()));
    }
}

template <class T>
void
testMakeInfinite(const char *type)
{
    cout << "    makeInfinite() for type " << type << endl;

    //
    // Infinite box
    //
    {
        Imath::Box<T> b;
        b.makeInfinite();
        assert(b.min == T(T::baseTypeMin()) &&
               b.max == T(T::baseTypeMax()));
    }

    //
    // Non-empty, has volume
    //
    {
        Imath::Box<T> b(T(-1), T(1));
        b.makeInfinite();
        assert(b.min == T(T::baseTypeMin()) &&
               b.max == T(T::baseTypeMax()));
    }

    //
    // Non-empty, no volume
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        T min(0);
        T max(0);
        max[T::dimensions() - 1] = 1;

        Imath::Box<T> b(min, max);
        b.makeInfinite();
        assert(b.min == T(T::baseTypeMin()) &&
               b.max == T(T::baseTypeMax()));
    }
}

template <class T>
void
testExtendByPoint(const char *type)
{
    cout << "    extendBy() point for type " << type << endl;

    Imath::Rand32 rand(0);

    const unsigned int iters = 10;

    //
    // Extend empty box with a single point.
    //
    for (unsigned int i = 0; i < iters; i++)
    {
        T p;
        for (unsigned int j = 0; j < T::dimensions(); j++)
            p[j] = rand.nextf(-12345, 12345);
                              
        Imath::Box<T> b;
        b.extendBy(p);
        assert(b.min == p && b.max == p);
    }

    //
    // Extend empty box with a number of random points. Note that
    // this also covers extending a non-empty box.
    //
    for (unsigned int i = 0; i < iters; i++)
    {
        Imath::Box<T> b;

        T min;
        T max;

        for (unsigned int j = 0; j < i; j++)
        {
            T p;
            for (unsigned int k = 0; k < T::dimensions(); k++)
                p[k] = rand.nextf(-12345, 12345);

            if (j == 0)
            {
                min = p;
                max = p;
            }
            for (unsigned int k = 0; k < T::dimensions(); k++)
            {
                min[k] = std::min(min[k], p[k]);
                max[k] = std::max(max[k], p[k]);
            }

            b.extendBy(p);

            assert(b.min == min && b.max == max);
        }
    }
}

template <class T>
void
testExtendByBox(const char *type)
{
    cout << "    extendBy() box for type " << type << endl;

    //
    // Extend empty box with an empty box;
    //
    {
        Imath::Box<T> b;
        b.extendBy(Imath::Box<T>());
        assert(b.min == T(T::baseTypeMax()) &&
               b.max == T(T::baseTypeMin()));
    }

    //
    // Extend empty box with a non-empty box and vice versa.
    //
    {
        std::vector<T> perms;
        permutations(perms);

        for (unsigned int i = 0; i < perms.size(); i++)
        {
            for (unsigned int j = 0; j < perms.size(); j++)
            {
                T p0 = -perms[i];
                T p1 =  perms[j];

                Imath::Box<T> b0;
                b0.extendBy(Imath::Box<T>(p0, p1));
                assert(b0.min == p0 && b0.max == p1);

                Imath::Box<T> b1(p0, p1);
                b1.extendBy(Imath::Box<T>());
                assert(b1.min == p0 && b1.max == p1);
            }
        }
    }
        
    //
    // Extend non-empty box with non-empty box. Starts with empty, then builds.
    //
    Imath::Rand32 rand(0);
    const unsigned int iters = 10;
    {
        Imath::Box<T> b;

        T min, max;

        for (unsigned int i = 1; i < iters; i++)
        {
            T p0;
            T p1;
            for (unsigned int k = 0; k < T::dimensions(); k++)
            {
                p0[k] = rand.nextf(   0,  999);
                p1[k] = rand.nextf(1000, 1999);
            }

            min = b.min;
            max = b.max;
            for (unsigned int k = 0; k < T::dimensions(); k++)
            {
                min[k] = std::min(min[k], p0[k]);
                max[k] = std::max(max[k], p1[k]);
            }
            b.extendBy(Imath::Box<T>(p0, p1));

            assert(b.min == min && b.max == max);
        }
    }
}

template <class T>
void
testComparators(const char *type)
{
    cout << "    comparators for type " << type << endl;

    Imath::Rand32 rand(0);

    //
    // Compare empty.
    //
    {
        Imath::Box<T> b0;
        Imath::Box<T> b1;

        assert(b0 == b1);
        assert(!(b0 != b1));
    }

    //
    // Compare empty to non-empty.
    //
    {
        std::vector<T> perms;
        permutations(perms);

        for (unsigned int i = 0; i < perms.size(); i++)
        {
            for (unsigned int j = 0; j < perms.size(); j++)
            {
                T p0 = -perms[i];
                T p1 =  perms[j];

                Imath::Box<T> b0;
                Imath::Box<T> b1(p0, p1);
                assert(!(b0 == b1));
                assert(b0 != b1);
            }
        }
    }

    //
    // Compare two non-empty
    //
    {
        std::vector<T> perms;
        permutations(perms);

        for (unsigned int i = 0; i < perms.size(); i++)
        {
            for (unsigned int j = 0; j < perms.size(); j++)
            {
                T p0 = -perms[i];
                T p1 =  perms[j];

                T p2 = -perms[j];
                T p3 =  perms[i];

                Imath::Box<T> b0(p0, p1);
                Imath::Box<T> b1(p2, p3);
                Imath::Box<T> b2(p0, p1);

                if (i == j)
                {
                    assert(b0 == b1);
                    assert(!(b0 != b1));
                }
                else
                {
                    assert(b0 != b1);
                    assert(!(b0 == b1));
                }
                assert(b0 == b2);
                assert(!(b0 != b2));
            }
        }
    }
}


template <class T>
void
testIntersects(const char *type)
{
    cout << "    intersects() for type " << type << endl;

    Imath::Rand32 rand(0);

    //
    // Intersect point with empty box.
    //
    {
        Imath::Box<T> b;
        T             p(1);

        assert(!b.intersects(p));
    }

    //
    // Intersect point with non-empty, has-volume box.
    //
    {
        Imath::Box<T> b(T(-1), T(1));
        T             p0(0);
        T             p1(5);
        T             p2(-5);

        assert(b.intersects(p0));
        assert(!b.intersects(p1));
        assert(!b.intersects(p2));
    }

    //
    // Intersect point with non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 1;

        T p0(0);
        T p1(5);
        Imath::Box<T> b(min, max);

        assert(b.intersects(p0));
        assert(!b.intersects(p1));
    }

    //
    // Intersect empty box with empty box.
    //
    {
        Imath::Box<T> b0;
        Imath::Box<T> b1;

        assert(!b0.intersects(b1));
        assert(!b1.intersects(b0));
    }

    //
    // Intersect empty box with non-empty has-volume boxes.
    //
    {
        Imath::Box<T> b0;
        Imath::Box<T> b1(T(-1), T(1));
        Imath::Box<T> b2(T( 1), T(2));

        assert(!b0.intersects(b1));
        assert(!b0.intersects(b2));

        assert(!b1.intersects(b0));
        assert(!b2.intersects(b0));
    }

    //
    // Intersect empty box with non-empty no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 1;

        Imath::Box<T> b0;
        Imath::Box<T> b1(min, max);

        assert(!b0.intersects(b1));
        assert(!b1.intersects(b0));
    }

    //
    // Intersect non-empty has-volume box with non-empty has-volume box.
    //
    {
        Imath::Box<T> b1(T(-1), T(1));
        Imath::Box<T> b2(T(-1), T(1));
        Imath::Box<T> b3(T( 1), T(2));
        Imath::Box<T> b4(T( 2), T(3));

        assert(b1.intersects(b1));
        assert(b1.intersects(b3));
        assert(!b1.intersects(b4));

        assert(b3.intersects(b1));
        assert(!b4.intersects(b1));
    }

    //
    // Intersect non-empty has-volume box with non-empty no-volume box.
    //
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        Imath::Box<T> b0(T(-1), T(1));

        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 1;

        Imath::Box<T> b1(min, max);
        Imath::Box<T> b2(min + T(2), max + T(2));
        
        assert(b0.intersects(b1));
        assert(b1.intersects(b0));

        assert(!b0.intersects(b2));
        assert(!b2.intersects(b1));
    }

    //
    // Intersect non-empty no-volume box with non-empty no-volume box.
    //
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 1;

        Imath::Box<T> b0(min, max);
        Imath::Box<T> b1(min,  max + T(2));
        Imath::Box<T> b2(min + T(2),  max + T(2));
        
        assert(b0.intersects(b1));
        assert(b1.intersects(b0));

        assert(!b0.intersects(b2));
        assert(!b2.intersects(b0));
    }
}

template <class T>
void
testSize(const char *type)
{
    cout << "    size() for type " << type << endl;

    //
    // Size of empty box.
    //
    {
        Imath::Box<T> b;
        assert(b.size() == T(0));
    }

    //
    // Size of non-empty, has-volume box.
    // Boxes are:
    //    2D: [(-1, -1),         (1, 1)       ]
    //    3D: [(-1, -1, -1),     (1, 1, 1)    ]
    //    4D: [(-1, -1, -1, -1), (1, 1, 1, 1) ] 
    //
    // and
    //
    //    2D: [(-1, -2),         (1, 2)       ]
    //    3D: [(-1, -2, -3),     (1, 2, 3)    ]
    //    4D: [(-1, -2, -3, -4), (1, 2, 3, 4) ]
    //
    {
        Imath::Box<T> b0(T(-1), T(1));
        assert(b0.size() == T(2));

        T p;
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
            p[i] = i;
        }
        Imath::Box<T> b1(-p, p);
        assert(b1.size() == p * T(2));
    }

    //
    // Size of non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 1)      ]
    //    3D: [(0, 0, 0),    (0, 0, 1)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 1)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 1;

        Imath::Box<T> b(min, max);

        assert(b.size() == max);
    }
}


template <class T>
void
testCenter(const char *type)
{
    cout << "    center() for type " << type << endl;

    //
    // Center of empty box.
    //
    {
        Imath::Box<T> b;
        assert(b.center() == T(0));
    }

    //
    // Center of non-empty, has-volume box.
    // Boxes are:
    //    2D: [(-1, -1),         (1, 1)       ]
    //    3D: [(-1, -1, -1),     (1, 1, 1)    ]
    //    4D: [(-1, -1, -1, -1), (1, 1, 1, 1) ] 
    //
    // and
    //
    //    2D: [(-2, -4),         ( 8,  2)       ]
    //    3D: [(-2, -4, -6),     (12,  8, 2)    ]
    //    4D: [(-2, -4, -6, -8), (16, 12, 8, 4) ]
    //
    {
        Imath::Box<T> b0(T(-1), T(1));
        assert(b0.center() == T(0));

        T p0;
        T p1;
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
            p0[i] = -pow(2.0, (int)(i + 1));
            p1[i] =  pow(2.0, (int)(T::dimensions() - i));
        }
        Imath::Box<T> b1(p0, p1);
        assert(b1.center() == (p1 + p0) / 2);
    }

    //
    // Center of non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 2)      ]
    //    3D: [(0, 0, 0),    (0, 0, 2)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 2)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 2;

        Imath::Box<T> b(min, max);

        assert(b.center() == max /2);
    }
}


template <class T>
void
testIsEmpty(const char *type)
{
    cout << "    isEmpty() for type " << type << endl;

    //
    // Empty box.
    //
    {
        Imath::Box<T> b;
        assert(b.isEmpty());
    }

    //
    // Non-empty, has-volume box.
    //    2D: [(-2, -4),         ( 8,  2)       ]
    //    3D: [(-2, -4, -6),     (12,  8, 2)    ]
    //    4D: [(-2, -4, -6, -8), (16, 12, 8, 4) ]
    //
    {
        Imath::Box<T> b0(T(-1), T(1));
        assert(!b0.isEmpty());

        T p0;
        T p1;
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
           p0[i] = -pow(2.0, (int)(i + 1));
           p1[i] =  pow(2.0, (int)(T::dimensions() - i));
        }
        Imath::Box<T> b1(p0, p1);
        assert(!b1.isEmpty());
    }

    //
    // Non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 2)      ]
    //    3D: [(0, 0, 0),    (0, 0, 2)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 2)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 2;

        Imath::Box<T> b(min, max);

        assert(!b.isEmpty());
    }
}


template <class T>
void
testIsInfinite(const char *type)
{
    cout << "    isInfinite() for type " << type << endl;

    //
    // Infinite box.
    //
    {
        Imath::Box<T> b;
        b.makeInfinite();
        assert(b.isInfinite());
    }

    //
    // Non-empty, has-volume box.
    //    2D: [(-2, -4),         ( 8,  2)       ]
    //    3D: [(-2, -4, -6),     (12,  8, 2)    ]
    //    4D: [(-2, -4, -6, -8), (16, 12, 8, 4) ]
    //
    {
        Imath::Box<T> b0(T(-1), T(1));
        assert(!b0.isInfinite());

        T p0;
        T p1;
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
            p0[i] = -typename T::BaseType(1 << (i + 1));
            p1[i] =  typename T::BaseType(1 << (T::dimensions() - i));
        }
        Imath::Box<T> b1(p0, p1);
        assert(!b1.isInfinite());
    }

    //
    // Non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 2)      ]
    //    3D: [(0, 0, 0),    (0, 0, 2)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 2)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 2;

        Imath::Box<T> b(min, max);

        assert(!b.isInfinite());
    }
}


template <class T>
void
testHasVolume(const char *type)
{
    cout << "    hasVolume() for type " << type << endl;

    //
    // Empty box.
    //
    {
        Imath::Box<T> b;
        assert(!b.hasVolume());
    }

    //
    // Infinite box.
    //
    {
        Imath::Box<T> b;
        b.makeInfinite();
        assert(b.hasVolume());
    }

    //
    // Non-empty, has-volume box.
    //    2D: [(-2, -4),         ( 8,  2)       ]
    //    3D: [(-2, -4, -6),     (12,  8, 2)    ]
    //    4D: [(-2, -4, -6, -8), (16, 12, 8, 4) ]
    //
    {
        Imath::Box<T> b0(T(-1), T(1));
        assert(b0.hasVolume());

        T p0;
        T p1;
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
            p0[i] = -pow(2.0, (int)(i + 1));
            p1[i] =  pow(2.0, (int)(T::dimensions() - i));
        }
        Imath::Box<T> b1(p0, p1);
        assert(b1.hasVolume());
    }

    //
    // Non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0),       (0, 2)      ]
    //    3D: [(0, 0, 0),    (0, 0, 2)   ]
    //    4D: [(0, 0, 0, 0), (0, 0, 0, 2)]
    //
    {
        T min(0);
        T max = min;
        max[T::dimensions() - 1] = 2;

        Imath::Box<T> b(min, max);

        assert(!b.hasVolume());
    }
}


template <class T>
void
testMajorAxis(const char *type)
{
    cout << "    majorAxis() for type " << type << endl;

    //
    // Empty box.
    //
    {
        Imath::Box<T> b;
        assert(b.majorAxis() == 0);
    }

    //
    // Non-empty, has-volume box.
    // Boxes are [ (0, 0, ...), (<all permutations of 1..T::dimensions()>) ]
    //
    {
        std::vector<T> perms;
        permutations(perms);

        for (unsigned int i = 0; i < perms.size(); i++)
        {
            Imath::Box<T> b(T(0), perms[i]);

            unsigned int major = 0;
            T size = perms[i] - T(0);
            for (unsigned int j = 1; j < T::dimensions(); j++)
                if (size[j] > size[major])
                    major = j;
                    
            assert(b.majorAxis() == major);
        }
    }

    //
    // Non-empty, no-volume box.
    // Boxes are:
    //    2D: [(0, 0), (1, 0) ]
    //    2D: [(0, 0), (0, 1) ]
    //
    //    3D: [(0, 0), (1, 0, 0) ]
    //    3D: [(0, 0), (0, 1, 0) ]
    //    3D: [(0, 0), (0, 0, 1) ]
    //
    //    and similarly for 4D
    //
    {
        for (unsigned int i = 0; i < T::dimensions(); i++)
        {
            for (unsigned int j = 0; j < T::dimensions(); j++)
            {
                T max(0);
                max[j] = 1;

                Imath::Box<T> b(T(0), max);
                assert(b.majorAxis() == j);
            }
        }
    }
}

} // anonymous namespace


void
testBox()
{
    cout << "Testing box methods" << endl;

    //
    // Constructors
    //
    testConstructors<Imath::V2s>("V2s");
    testConstructors<Imath::V2i>("V2i");
    testConstructors<Imath::V2f>("V2f");
    testConstructors<Imath::V2d>("V2d");

    testConstructors<Imath::V3s>("V3s");
    testConstructors<Imath::V3i>("V3i");
    testConstructors<Imath::V3f>("V3f");
    testConstructors<Imath::V3d>("V3d");

    testConstructors<Imath::V4s>("V4s");
    testConstructors<Imath::V4i>("V4i");
    testConstructors<Imath::V4f>("V4f");
    testConstructors<Imath::V4d>("V4d");

    //
    // makeEmpty()
    //
    testMakeEmpty<Imath::V2s>("V2s");
    testMakeEmpty<Imath::V2i>("V2i");
    testMakeEmpty<Imath::V2f>("V2f");
    testMakeEmpty<Imath::V2d>("V2d");

    testMakeEmpty<Imath::V3s>("V3s");
    testMakeEmpty<Imath::V3i>("V3i");
    testMakeEmpty<Imath::V3f>("V3f");
    testMakeEmpty<Imath::V3d>("V3d");

    testMakeEmpty<Imath::V4s>("V4s");
    testMakeEmpty<Imath::V4i>("V4i");
    testMakeEmpty<Imath::V4f>("V4f");
    testMakeEmpty<Imath::V4d>("V4d");

    //
    // makeInfinite()
    //
    testMakeInfinite<Imath::V2s>("V2s");
    testMakeInfinite<Imath::V2i>("V2i");
    testMakeInfinite<Imath::V2f>("V2f");
    testMakeInfinite<Imath::V2d>("V2d");

    testMakeInfinite<Imath::V3s>("V3s");
    testMakeInfinite<Imath::V3i>("V3i");
    testMakeInfinite<Imath::V3f>("V3f");
    testMakeInfinite<Imath::V3d>("V3d");

    testMakeInfinite<Imath::V4s>("V4s");
    testMakeInfinite<Imath::V4i>("V4i");
    testMakeInfinite<Imath::V4f>("V4f");
    testMakeInfinite<Imath::V4d>("V4d");

    //
    // extendBy() (point)
    //
    testExtendByPoint<Imath::V2s>("V2s");
    testExtendByPoint<Imath::V2i>("V2i");
    testExtendByPoint<Imath::V2f>("V2f");
    testExtendByPoint<Imath::V2d>("V2d");

    testExtendByPoint<Imath::V3s>("V3s");
    testExtendByPoint<Imath::V3i>("V3i");
    testExtendByPoint<Imath::V3f>("V3f");
    testExtendByPoint<Imath::V3d>("V3d");

    testExtendByPoint<Imath::V4s>("V4s");
    testExtendByPoint<Imath::V4i>("V4i");
    testExtendByPoint<Imath::V4f>("V4f");
    testExtendByPoint<Imath::V4d>("V4d");

    //
    // extendBy() box
    //
    testExtendByBox<Imath::V2s>("V2s");
    testExtendByBox<Imath::V2i>("V2i");
    testExtendByBox<Imath::V2f>("V2f");
    testExtendByBox<Imath::V2d>("V2d");

    testExtendByBox<Imath::V3s>("V3s");
    testExtendByBox<Imath::V3i>("V3i");
    testExtendByBox<Imath::V3f>("V3f");
    testExtendByBox<Imath::V3d>("V3d");

    testExtendByBox<Imath::V4s>("V4s");
    testExtendByBox<Imath::V4i>("V4i");
    testExtendByBox<Imath::V4f>("V4f");
    testExtendByBox<Imath::V4d>("V4d");

    //
    // == and !==
    //
    testComparators<Imath::V2s>("V2s");
    testComparators<Imath::V2i>("V2i");
    testComparators<Imath::V2f>("V2f");
    testComparators<Imath::V2d>("V2d");

    testComparators<Imath::V3s>("V3s");
    testComparators<Imath::V3i>("V3i");
    testComparators<Imath::V3f>("V3f");
    testComparators<Imath::V3d>("V3d");

    testComparators<Imath::V4s>("V4s");
    testComparators<Imath::V4i>("V4i");
    testComparators<Imath::V4f>("V4f");
    testComparators<Imath::V4d>("V4d");

    //
    // size()
    //
    testSize<Imath::V2s>("V2s");
    testSize<Imath::V2i>("V2i");
    testSize<Imath::V2f>("V2f");
    testSize<Imath::V2d>("V2d");

    testSize<Imath::V3s>("V3s");
    testSize<Imath::V3i>("V3i");
    testSize<Imath::V3f>("V3f");
    testSize<Imath::V3d>("V3d");

    testSize<Imath::V4s>("V4s");
    testSize<Imath::V4i>("V4i");
    testSize<Imath::V4f>("V4f");
    testSize<Imath::V4d>("V4d");

    //
    // center()
    //
    testCenter<Imath::V2s>("V2s");
    testCenter<Imath::V2i>("V2i");
    testCenter<Imath::V2f>("V2f");
    testCenter<Imath::V2d>("V2d");

    testCenter<Imath::V3s>("V3s");
    testCenter<Imath::V3i>("V3i");
    testCenter<Imath::V3f>("V3f");
    testCenter<Imath::V3d>("V3d");

    testCenter<Imath::V4s>("V4s");
    testCenter<Imath::V4i>("V4i");
    testCenter<Imath::V4f>("V4f");
    testCenter<Imath::V4d>("V4d");

    //
    // isEmpty()
    //
    testIsEmpty<Imath::V2s>("V2s");
    testIsEmpty<Imath::V2i>("V2i");
    testIsEmpty<Imath::V2f>("V2f");
    testIsEmpty<Imath::V2d>("V2d");

    testIsEmpty<Imath::V3s>("V3s");
    testIsEmpty<Imath::V3i>("V3i");
    testIsEmpty<Imath::V3f>("V3f");
    testIsEmpty<Imath::V3d>("V3d");

    testIsEmpty<Imath::V4s>("V4s");
    testIsEmpty<Imath::V4i>("V4i");
    testIsEmpty<Imath::V4f>("V4f");
    testIsEmpty<Imath::V4d>("V4d");

    //
    // isInfinite()
    //
    testIsInfinite<Imath::V2s>("V2s");
    testIsInfinite<Imath::V2i>("V2i");
    testIsInfinite<Imath::V2f>("V2f");
    testIsInfinite<Imath::V2d>("V2d");

    testIsInfinite<Imath::V3s>("V3s");
    testIsInfinite<Imath::V3i>("V3i");
    testIsInfinite<Imath::V3f>("V3f");
    testIsInfinite<Imath::V3d>("V3d");

    testIsInfinite<Imath::V4s>("V4s");
    testIsInfinite<Imath::V4i>("V4i");
    testIsInfinite<Imath::V4f>("V4f");
    testIsInfinite<Imath::V4d>("V4d");

    //
    // hasVolume()
    //
    testHasVolume<Imath::V2s>("V2s");
    testHasVolume<Imath::V2i>("V2i");
    testHasVolume<Imath::V2f>("V2f");
    testHasVolume<Imath::V2d>("V2d");

    testHasVolume<Imath::V3s>("V3s");
    testHasVolume<Imath::V3i>("V3i");
    testHasVolume<Imath::V3f>("V3f");
    testHasVolume<Imath::V3d>("V3d");

    testHasVolume<Imath::V4s>("V4s");
    testHasVolume<Imath::V4i>("V4i");
    testHasVolume<Imath::V4f>("V4f");
    testHasVolume<Imath::V4d>("V4d");

    //
    // majorAxis()
    //
    testMajorAxis<Imath::V2s>("V2s");
    testMajorAxis<Imath::V2i>("V2i");
    testMajorAxis<Imath::V2f>("V2f");
    testMajorAxis<Imath::V2d>("V2d");

    testMajorAxis<Imath::V3s>("V3s");
    testMajorAxis<Imath::V3i>("V3i");
    testMajorAxis<Imath::V3f>("V3f");
    testMajorAxis<Imath::V3d>("V3d");

    testMajorAxis<Imath::V4s>("V4s");
    testMajorAxis<Imath::V4i>("V4i");
    testMajorAxis<Imath::V4f>("V4f");
    testMajorAxis<Imath::V4d>("V4d");

    cout << "ok\n" << endl;
}
