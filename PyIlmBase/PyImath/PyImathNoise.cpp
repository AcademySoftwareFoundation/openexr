//
//      Copyright (c) 1998-2006 Lucas Digital Ltd.  All rights reserved.
//      Used under authorization.  This material contains the
//      confidential and proprietary information of Lucas Digital
//      Ltd. and may not be copied in whole or in part without the
//      express written permission of Lucas Digital Ltd.  This
//      copyright notice does not imply publication.
//

#include <PyImathNoise.h>

namespace PyImath{

using namespace boost::python;

float noise1(float f) 
{
    MATH_EXC_ON;
    return Imath::noise(f);
}

double noise2(double d) 
{
    MATH_EXC_ON;
    return Imath::noise(d);
}

float noise3(const Imath::Vec2<float> &v) 
{
    MATH_EXC_ON;
    return Imath::noise(v);
}

double noise4(const Imath::Vec2<double> &v) 
{
    MATH_EXC_ON;
    return Imath::noise(v);
}

float noise5(const Imath::Vec3<float> &v) 
{
    MATH_EXC_ON;
    return Imath::noise(v);
}

double noise6(const Imath::Vec3<double> &v) 
{
    MATH_EXC_ON;
    return Imath::noise(v);
}

float noise7(const tuple &t) 
{
    return noiseTuple<float>(t);
}

double noise8(const tuple &t) 
{
    return noiseTuple<double>(t);
}
    
float noiseCen1(float f) 
{
    MATH_EXC_ON;
    return Imath::noiseCen(f);
}

double noiseCen2(double d) 
{
    MATH_EXC_ON;
    return Imath::noiseCen(d);
}

float noiseCen3(const Imath::Vec2<float> &v) 
{
    MATH_EXC_ON;
    return Imath::noiseCen(v);
}

double noiseCen4(const Imath::Vec2<double> &v) 
{
    MATH_EXC_ON;
    return Imath::noiseCen(v);
}

float noiseCen5(const tuple &t) 
{
    return noiseCenTuple<float>(t);
}

double noiseCen6(const tuple &t) 
{
    return noiseCenTuple<double>(t);
}

float noiseCen7(const Imath::Vec3<float> &v) 
{
    MATH_EXC_ON;
    return Imath::noiseCen(v);
}

double noiseCen8(const Imath::Vec3<double> &v) 
{
    MATH_EXC_ON;
    return Imath::noiseCen(v);
}
    
Imath::Vec3<float> noiseCen3d1(const Imath::Vec3<float> &v) 
{
    MATH_EXC_ON;
    return Imath::noiseCen3d(v);
}

Imath::Vec3<double> noiseCen3d2(const Imath::Vec3<double> &v) 
{
    MATH_EXC_ON;
    return Imath::noiseCen3d(v);
}

Imath::Vec3<float> noiseCen3d3(const tuple &t) 
{
    MATH_EXC_ON;
    return noiseCen3dTuple<float>(t);
}

Imath::Vec3<double> noiseCen3d4(const tuple &t) 
{
    MATH_EXC_ON;
    return noiseCen3dTuple<double>(t);
}
    
float noiseCenGrad1(float f, float &g) 
{
    MATH_EXC_ON;
    return Imath::noiseCenGrad(f, g);
}

double noiseCenGrad2(double d, double &g) 
{
    MATH_EXC_ON;
    return Imath::noiseCenGrad(d, g);
}

float noiseCenGrad3(const Imath::Vec2<float> &v, Imath::Vec2<float> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseCenGrad(v, g);
}

double noiseCenGrad4(const Imath::Vec2<double> &v, Imath::Vec2<double> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseCenGrad(v, g);
}

float noiseCenGrad5(const Imath::Vec3<float> &v, Imath::Vec3<float> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseCenGrad(v, g);
}

double noiseCenGrad6(const Imath::Vec3<double> &v, Imath::Vec3<double> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseCenGrad(v, g);
}

float noiseCenGrad7(const tuple &t, tuple &g) 
{
    return noiseCenGradTuple1<float>(t, g);
}

double noiseCenGrad8(const tuple &t, tuple &g) 
{
    return noiseCenGradTuple1<double>(t, g);
}

tuple noiseCenGrad9(const tuple &t) 
{
    return noiseCenGradTuple2<float>(t);
}

tuple noiseCenGrad10(const tuple &t) 
{
    return noiseCenGradTuple2<double>(t);
}

tuple noiseCenGrad11(float f) 
{
    return noiseCenGradTuple3<float>(f);
}

tuple noiseCenGrad12(double d) 
{
    return noiseCenGradTuple3<double>(d);
}

    
float noiseGrad1(float f, float &g) 
{
    MATH_EXC_ON;
    return Imath::noiseGrad(f, g);
}

double noiseGrad2(double d, double &g) 
{
    MATH_EXC_ON;
    return Imath::noiseGrad(d, g);
}

float noiseGrad3(const Imath::Vec2<float> &v, Imath::Vec2<float> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseGrad(v, g);
}

double noiseGrad4(const Imath::Vec2<double> &v, Imath::Vec2<double> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseGrad(v, g);
}

float noiseGrad5(const Imath::Vec3<float> &v, Imath::Vec3<float> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseGrad(v, g);
}

double noiseGrad6(const Imath::Vec3<double> &v, Imath::Vec3<double> &g) 
{
    MATH_EXC_ON;
    return Imath::noiseGrad(v, g);
}

tuple noiseGrad7(const tuple &t) 
{
    return noiseGradTuple1<float>(t);
}

tuple noiseGrad8(const tuple &t) 
{
    return noiseGradTuple1<double>(t);
}

tuple noiseGrad9(float f) 
{
    return noiseGradTuple2<float>(f);
}

tuple noiseGrad10(double d) 
{
    return noiseGradTuple2<double>(d);
}


} // PyImath

