#ifndef _PyImathNoise_h_
#define _PyImathNoise_h_

//
//      Copyright (c) 1998-2006 Lucas Digital Ltd.  All rights reserved.
//      Used under authorization.  This material contains the
//      confidential and proprietary information of Lucas Digital
//      Ltd. and may not be copied in whole or in part without the
//      express written permission of Lucas Digital Ltd.  This
//      copyright notice does not imply publication.
//

// Separate header file created to deal with the noise functions when tuples are
// input instead of vectors

#include <PyImathMathExc.h>
#include <PyImathExport.h>
#include <Python.h>
#include <boost/python.hpp>
#include <ImathNoise.h>
#include <Iex.h>

namespace PyImath{

using namespace boost::python;

template <class T>
T noiseTuple(const tuple &t)
{    
    MATH_EXC_ON;
    if(t.attr("__len__")() == 2)
    {
        Imath::Vec2<T> point;
        point.x = extract<T>(t[0]);
        point.y = extract<T>(t[1]);
        
        return Imath::noise(point);
    }
    if(t.attr("__len__")() == 3)
    {
        Imath::Vec3<T> point;
        point.x = extract<T>(t[0]);
        point.y = extract<T>(t[1]);
        point.z = extract<T>(t[2]);
        
        return Imath::noise(point);        
    }
    else
        THROW(Iex::LogicExc, "noise expects tuple of length 2 or 3");
}
    
template <class T>
T noiseCenTuple(const tuple &t)
{    
    MATH_EXC_ON;
    if(t.attr("__len__")() == 2)
    {
        Imath::Vec2<T> point;
        point.x = extract<T>(t[0]);
        point.y = extract<T>(t[1]);
        
        return Imath::noiseCen(point);
    }
    if(t.attr("__len__")() == 3)
    {
        Imath::Vec3<T> point;
        point.x = extract<T>(t[0]);
        point.y = extract<T>(t[1]);
        point.z = extract<T>(t[1]);
        
        return Imath::noiseCen(point);
    }
    else
        THROW(Iex::LogicExc, "noiseCen expects tuple of length 2 or3");
}

template <class T>
Imath::Vec3<T> noiseCen3dTuple(const tuple &t)
{    
    MATH_EXC_ON;
    if(t.attr("__len__")() == 3)
    {
        Imath::Vec3<T> point;
        point.x = extract<T>(t[0]);
        point.y = extract<T>(t[1]);
        point.z = extract<T>(t[2]);
        
        return Imath::noiseCen3d(point);
    }
    else
        THROW(Iex::LogicExc, "noiseCen3d expects tuple of length 3");
}

template <class T>
T noiseCenGradTuple1(const tuple &t0, tuple &t1)
{    
    MATH_EXC_ON;
    if(t0.attr("__len__")() == 3 && t1.attr("__len__")() == 3)
    {
        Imath::Vec3<T> point0;
        Imath::Vec3<T> point1;
        point0.x = extract<T>(t0[0]);
        point0.y = extract<T>(t0[1]);
        point0.z = extract<T>(t0[2]);
        
        point1.x = extract<T>(t1[0]);
        point1.y = extract<T>(t1[1]);
        point1.z = extract<T>(t1[2]);
        
        return Imath::noiseCenGrad(point0, point1);
    }
    else if(t0.attr("__len__")() == 2 && t1.attr("__len__")() == 2)
    {
        Imath::Vec2<T> point0;
        Imath::Vec2<T> point1;
        point0.x = extract<T>(t0[0]);
        point0.y = extract<T>(t0[1]);
        
        point1.x = extract<T>(t1[0]);
        point1.y = extract<T>(t1[1]);
        
        return Imath::noiseCenGrad(point0, point1);        
    }
    else
        THROW(Iex::LogicExc, "noiseGrad expects tuple of length 3");
}

template <class T>
tuple noiseCenGradTuple2(const tuple &t0)
{    
    MATH_EXC_ON;
    tuple t;
    
    if(t0.attr("__len__")() == 3)
    {
        Imath::Vec3<T> point0;
        Imath::Vec3<T> point1;
        point0.x = extract<T>(t0[0]);
        point0.y = extract<T>(t0[1]);
        point0.z = extract<T>(t0[2]);
        
        T result = Imath::noiseCenGrad(point0, point1);
        t = make_tuple(result, point1);
    }
    else if(t0.attr("__len__")() == 2)
    {
        Imath::Vec2<T> point0;
        Imath::Vec2<T> point1;
        point0.x = extract<T>(t0[0]);
        point0.y = extract<T>(t0[1]);
        
        T result = Imath::noiseCenGrad(point0, point1);
        t = make_tuple(result, point1);
    }
    else
        THROW(Iex::LogicExc, "noiseGrad expects tuple of length 3");
    
    return t;
}

template <class T>
tuple noiseCenGradTuple3(T a)
{    
    MATH_EXC_ON;
    tuple t;
    T grad;
    T result = Imath::noiseCenGrad(a, grad);
    t = make_tuple(result, grad);
    return t;
}

template <class T>
tuple noiseGradTuple1(const tuple &t0)
{   
    MATH_EXC_ON;
    tuple t;
    
    if(t0.attr("__len__")() == 3)
    {
        Imath::Vec3<T> point0;
        Imath::Vec3<T> grad;
        point0.x = extract<T>(t0[0]);
        point0.y = extract<T>(t0[1]);
        point0.z = extract<T>(t0[2]);
        
        T result = Imath::noiseGrad(point0, grad);
        t = make_tuple(result, grad);
    }
    else if(t0.attr("__len__")() == 2)
    {
        Imath::Vec2<T> point0;
        Imath::Vec2<T> grad;
        point0.x = extract<T>(t0[0]);
        point0.y = extract<T>(t0[1]);
        
        T result = Imath::noiseGrad(point0, grad);
        t = make_tuple(result, grad);
    }
    else
        THROW(Iex::LogicExc, "noiseGrad expects tuple of length 3");
    
    return t;
}

template <class T>
tuple noiseGradTuple2(T a)
{    
    MATH_EXC_ON;
    tuple t;
    T grad;
    T result = Imath::noiseGrad(a, grad);
    t = make_tuple(result, grad);
    return t;
}

PYIMATH_EXPORT float 			noise1(float f);
PYIMATH_EXPORT double			noise2(double d);
PYIMATH_EXPORT float			noise3(const Imath::Vec2<float> &v);
PYIMATH_EXPORT double			noise4(const Imath::Vec2<double> &v);
PYIMATH_EXPORT float			noise5(const Imath::Vec3<float> &v);
PYIMATH_EXPORT double			noise6(const Imath::Vec3<double> &v);
PYIMATH_EXPORT float			noise7(const tuple &t);
PYIMATH_EXPORT double			noise8(const tuple &t);

PYIMATH_EXPORT float			noiseCen1(float f);
PYIMATH_EXPORT double			noiseCen2(double d);
PYIMATH_EXPORT float			noiseCen3(const Imath::Vec2<float> &v);
PYIMATH_EXPORT double			noiseCen4(const Imath::Vec2<double> &v);
PYIMATH_EXPORT float			noiseCen5(const tuple &t);
PYIMATH_EXPORT double			noiseCen6(const tuple &t);
PYIMATH_EXPORT float			noiseCen7(const Imath::Vec3<float> &v); 
PYIMATH_EXPORT double			noiseCen8(const Imath::Vec3<double> &v); 

PYIMATH_EXPORT Imath::Vec3<float>	noiseCen3d1(const Imath::Vec3<float> &v); 
PYIMATH_EXPORT Imath::Vec3<double>	noiseCen3d2(const Imath::Vec3<double> &v); 
PYIMATH_EXPORT Imath::Vec3<float>	noiseCen3d3(const tuple &t); 
PYIMATH_EXPORT Imath::Vec3<double>	noiseCen3d4(const tuple &t); 

PYIMATH_EXPORT float			noiseCenGrad1(float f, float &g); 
PYIMATH_EXPORT double			noiseCenGrad2(double d, double &g); 
PYIMATH_EXPORT float			noiseCenGrad3(const Imath::Vec2<float> &v, Imath::Vec2<float> &g); 
PYIMATH_EXPORT double			noiseCenGrad4(const Imath::Vec2<double> &v, Imath::Vec2<double> &g); 
PYIMATH_EXPORT float			noiseCenGrad5(const Imath::Vec3<float> &v, Imath::Vec3<float> &g); 
PYIMATH_EXPORT double			noiseCenGrad6(const Imath::Vec3<double> &v, Imath::Vec3<double> &g); 
PYIMATH_EXPORT float			noiseCenGrad7(const tuple &t, tuple &g); 
PYIMATH_EXPORT double			noiseCenGrad8(const tuple &t, tuple &g); 
PYIMATH_EXPORT tuple			noiseCenGrad9(const tuple &t); 
PYIMATH_EXPORT tuple			noiseCenGrad10(const tuple &t); 
PYIMATH_EXPORT tuple			noiseCenGrad11(float f); 
PYIMATH_EXPORT tuple			noiseCenGrad12(double d); 

PYIMATH_EXPORT float			noiseGrad1(float f, float &g); 
PYIMATH_EXPORT double			noiseGrad2(double d, double &g); 
PYIMATH_EXPORT float			noiseGrad3(const Imath::Vec2<float> &v, Imath::Vec2<float> &g); 
PYIMATH_EXPORT double			noiseGrad4(const Imath::Vec2<double> &v, Imath::Vec2<double> &g); 
PYIMATH_EXPORT float			noiseGrad5(const Imath::Vec3<float> &v, Imath::Vec3<float> &g); 
PYIMATH_EXPORT double			noiseGrad6(const Imath::Vec3<double> &v, Imath::Vec3<double> &g); 
PYIMATH_EXPORT tuple			noiseGrad7(const tuple &t); 
PYIMATH_EXPORT tuple			noiseGrad8(const tuple &t); 
PYIMATH_EXPORT tuple			noiseGrad9(float f); 
PYIMATH_EXPORT tuple			noiseGrad10(double d); 

} // PyImath

#endif


