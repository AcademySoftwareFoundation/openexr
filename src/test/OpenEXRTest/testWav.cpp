//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImathRandom.h"
#include <ImfArray.h>
#include <ImfWav.h>
#include <assert.h>
#include <exception>
#include <iostream>
#include <stdlib.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

namespace
{

void
fill1_14bit (
    Array2D<unsigned short>& a,
    Array2D<unsigned short>& b,
    int                      nx,
    int                      ny,
    IMATH_NAMESPACE::Rand48& rand48)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = rand48.nexti () & 0x3fff;
}

void
fill1_16bit (
    Array2D<unsigned short>& a,
    Array2D<unsigned short>& b,
    int                      nx,
    int                      ny,
    IMATH_NAMESPACE::Rand48& rand48)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = rand48.nexti () & 0xffff;
}

void
fill2 (
    Array2D<unsigned short>& a,
    Array2D<unsigned short>& b,
    int                      nx,
    int                      ny,
    unsigned short           v)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = v;
}

void
fill3_14bit (
    Array2D<unsigned short>& a, Array2D<unsigned short>& b, int nx, int ny)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = (x & 1) ? 0 : 0x3fff;
}

void
fill3_16bit (
    Array2D<unsigned short>& a, Array2D<unsigned short>& b, int nx, int ny)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = (x & 1) ? 0 : 0xffff;
}

void
fill4_14bit (
    Array2D<unsigned short>& a, Array2D<unsigned short>& b, int nx, int ny)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = (y & 1) ? 0 : 0x3fff;
}

void
fill4_16bit (
    Array2D<unsigned short>& a, Array2D<unsigned short>& b, int nx, int ny)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = (y & 1) ? 0 : 0xffff;
}

void
fill5_14bit (
    Array2D<unsigned short>& a, Array2D<unsigned short>& b, int nx, int ny)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = ((x + y) & 1) ? 0 : 0x3fff;
}

void
fill5_16bit (
    Array2D<unsigned short>& a, Array2D<unsigned short>& b, int nx, int ny)
{
    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            a[y][x] = b[y][x] = ((x + y) & 1) ? 0 : 0xffff;
}

unsigned short
maxValue (const Array2D<unsigned short>& a, int nx, int ny)
{
    unsigned short mx = 0;

    for (int y = 0; y < ny; ++y)
        for (int x = 0; x < nx; ++x)
            if (mx < a[y][x]) mx = a[y][x];

    // cout << "max value = " << mx << endl;

    return mx;
}

void
wavEncodeDecode (
    Array2D<unsigned short>&       a,
    const Array2D<unsigned short>& b,
    int                            nx,
    int                            ny)
{
    unsigned short mx = maxValue (a, nx, ny);

    //cout << "encoding " << flush;

    wav2Encode (&a[0][0], nx, 1, ny, nx, mx);

    //cout << "decoding " << flush;

    wav2Decode (&a[0][0], nx, 1, ny, nx, mx);

    //cout << "comparing" << endl;

    for (int y = 0; y < ny; ++y)
    {
        for (int x = 0; x < nx; ++x)
        {
            //cout << x << ' ' << y << ' ' << a[y][x] << ' ' << b[y][x] << endl;
            assert (a[y][x] == b[y][x]);
        }
    }
}

void
test (int nx, int ny)
{
    cout << nx << " x " << ny << endl;

    Array2D<unsigned short> a (ny, nx);
    Array2D<unsigned short> b (ny, nx);

    IMATH_NAMESPACE::Rand48 rand48 (0);

    fill1_14bit (a, b, nx, ny, rand48);
    wavEncodeDecode (a, b, nx, ny);

    fill1_16bit (a, b, nx, ny, rand48);
    wavEncodeDecode (a, b, nx, ny);

    fill2 (a, b, nx, ny, 0);
    wavEncodeDecode (a, b, nx, ny);

    fill2 (a, b, nx, ny, 1);
    wavEncodeDecode (a, b, nx, ny);

    fill2 (a, b, nx, ny, 0x3ffe);
    wavEncodeDecode (a, b, nx, ny);

    fill2 (a, b, nx, ny, 0x3fff);
    wavEncodeDecode (a, b, nx, ny);

    fill2 (a, b, nx, ny, 0xfffe);
    wavEncodeDecode (a, b, nx, ny);

    fill2 (a, b, nx, ny, 0xffff);
    wavEncodeDecode (a, b, nx, ny);

    fill3_14bit (a, b, nx, ny);
    wavEncodeDecode (a, b, nx, ny);

    fill3_16bit (a, b, nx, ny);
    wavEncodeDecode (a, b, nx, ny);

    fill4_14bit (a, b, nx, ny);
    wavEncodeDecode (a, b, nx, ny);

    fill4_16bit (a, b, nx, ny);
    wavEncodeDecode (a, b, nx, ny);

    fill5_14bit (a, b, nx, ny);
    wavEncodeDecode (a, b, nx, ny);

    fill5_16bit (a, b, nx, ny);
    wavEncodeDecode (a, b, nx, ny);
}

} // namespace

void
testWav (const std::string&)
{
    try
    {
        cout << "Testing Wavelet encoder" << endl;

        test (1, 1);
        test (2, 2);
        test (32, 32);
        test (1024, 16);
        test (16, 1024);
        test (997, 37);
        test (37, 997);
        test (1024, 1024);
        test (997, 997);

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
