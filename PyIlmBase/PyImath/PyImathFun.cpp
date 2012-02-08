///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998-2011, Industrial Light & Magic, a division of Lucas
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

#include <PyImathFun.h>
#include <PyImathDecorators.h>
#include <PyImathExport.h>
#include <PyImathAutovectorize.h>
#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/format.hpp>
#include <ImathVec.h>
#include <ImathMatrixAlgo.h>
#include <ImathFun.h>

namespace PyImath {

using namespace boost::python;
using namespace PyImath;

namespace {

template <class T>
struct rotationXYZWithUpDir_op
{
    static Imath::Vec3<T>
    apply(const Imath::Vec3<T> &from, const Imath::Vec3<T> &to, 
          const Imath::Vec3<T> &up)
    {
        Imath::Vec3<T> retval;
        Imath::extractEulerXYZ(Imath::rotationMatrixWithUpDir(from,to,up),retval);
        return retval;
    }
};

template <class T>
struct abs_op
{
    static T
    apply(T value)
    {
        return Imath::abs<T>(value);
    }
};

template <class T>
struct sign_op
{
    static T
    apply(T value)
    {
        return Imath::sign<T>(value);
    }
};

template <class T>
struct lerp_op
{
    static T
    apply(T a, T b, T t)
    {
        return Imath::lerp<T>(a,b,t);
    }
};

template <class T>
struct ulerp_op
{
    static T
    apply(T a, T b, T t)
    {
        return Imath::ulerp<T>(a,b,t);
    }
};

template <class T>
struct lerpfactor_op
{
    static T
    apply(T a, T b, T t)
    {
        return Imath::lerpfactor<T>(a,b,t);
    }
};

template <class T>
struct clamp_op
{
    static T
    apply(T value, T low, T high)
    {
        return Imath::clamp<T>(value,low,high);
    }
};

template <class T>
struct cmp_op
{
    static T
    apply(T value)
    {
        return Imath::cmp<T>(value);
    }
};

template <class T>
struct cmpt_op
{
    static T
    apply(T value)
    {
        return Imath::cmpt<T>(value);
    }
};

template <class T>
struct iszero_op
{
    static T
    apply(T value)
    {
        return Imath::iszero<T>(value);
    }
};

template <class T>
struct equal_op
{
    static T
    apply(T value)
    {
        return Imath::equal<T>(value);
    }
};

template <class T>
struct floor_op
{
    static int
    apply(T value)
    {
        return Imath::floor<T>(value);
    }
};

template <class T>
struct ceil_op
{
    static int
    apply(T value)
    {
        return Imath::ceil<T>(value);
    }
};

template <class T>
struct trunc_op
{
    static int
    apply(T value)
    {
        return Imath::trunc<T>(value);
    }
};

struct divs_op
{
    static int
    apply(int x, int y)
    {
        return Imath::divs(x,y);
    }
};

struct mods_op
{
    static int
    apply(int x, int y)
    {
        return Imath::mods(x,y);
    }
};

struct divp_op
{
    static int
    apply(int x, int y)
    {
        return Imath::divp(x,y);
    }
};

struct modp_op
{
    static int
    apply(int x, int y)
    {
        return Imath::modp(x,y);
    }
};

} // namespace

void register_functions()
{
    //
    // Utility Functions
    //

    // nb: MSVC gets confused about which arg we want (it thinks it
    // might be boost::arg), so telling it which one explicitly here.
    typedef boost::python::arg arg;

    PyImath::generate_bindings<abs_op<int>,boost::mpl::true_>(
        "abs",
        "return the absolute value of 'value'",
        (arg("value")));
    PyImath::generate_bindings<abs_op<float>,boost::mpl::true_>(
        "abs",
        "return the absolute value of 'value'",
        (arg("value")));
    PyImath::generate_bindings<abs_op<double>,boost::mpl::true_>(
        "abs",
        "return the absolute value of 'value'",
        (arg("value")));
    
    PyImath::generate_bindings<sign_op<int>,boost::mpl::true_>(
        "sign",
        "return 1 or -1 based on the sign of 'value'",
        (arg("value")));
    PyImath::generate_bindings<sign_op<float>,boost::mpl::true_>(
        "sign",
        "return 1 or -1 based on the sign of 'value'",
        (arg("value")));
    PyImath::generate_bindings<sign_op<double>,boost::mpl::true_>(
        "sign",
        "return 1 or -1 based on the sign of 'value'",
        (arg("value")));
    
    PyImath::generate_bindings<lerp_op<float>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "lerp",
        "return the linear interpolation of 'a' to 'b' using parameter 't'",
        (arg("a"),arg("b"),arg("t")));
    PyImath::generate_bindings<lerp_op<double>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "lerp",
        "return the linear interpolation of 'a' to 'b' using parameter 't'",
        (arg("a"),arg("b"),arg("t")));
    
    PyImath::generate_bindings<lerpfactor_op<float>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "lerpfactor",
        "return how far m is between a and b, that is return t such that\n"
        "if:\n"
        "    t = lerpfactor(m, a, b);\n"
        "then:\n"
        "    m = lerp(a, b, t);\n"
        "\n"
        "If a==b, return 0.\n",
        (arg("m"),arg("a"),arg("b")));
    PyImath::generate_bindings<lerpfactor_op<double>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "lerpfactor",
        "return how far m is between a and b, that is return t such that\n"
        "    if:\n"
        "        t = lerpfactor(m, a, b);\n"
        "    then:\n"
        "        m = lerp(a, b, t);\n"
        "    if a==b, return 0.\n",
        (arg("m"),arg("a"),arg("b")));
    
    PyImath::generate_bindings<clamp_op<int>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "clamp",
        "return the value clamped to the range [low,high]",
        (arg("value"),arg("low"),arg("high")));
    PyImath::generate_bindings<clamp_op<float>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "clamp",
        "return the value clamped to the range [low,high]",
        (arg("value"),arg("low"),arg("high")));
    PyImath::generate_bindings<clamp_op<double>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "clamp",
        "return the value clamped to the range [low,high]",
        (arg("value"),arg("low"),arg("high")));

    def("cmp", Imath::cmp<float>);
    def("cmp", Imath::cmp<double>);   

    def("cmpt", Imath::cmpt<float>);
    def("cmpt", Imath::cmpt<double>);

    def("iszero", Imath::iszero<float>);
    def("iszero", Imath::iszero<double>);

    def("equal", Imath::equal<float, float, float>);
    def("equal", Imath::equal<double, double, double>);    

    PyImath::generate_bindings<floor_op<float>,boost::mpl::true_>(
        "floor",
        "return the closest integer less than or equal to 'value'",
        (arg("value")));
    PyImath::generate_bindings<floor_op<double>,boost::mpl::true_>(
        "floor",
        "return the closest integer less than or equal to 'value'",
        (arg("value")));

    PyImath::generate_bindings<ceil_op<float>,boost::mpl::true_>(
        "ceil",
        "return the closest integer greater than or equal to 'value'",
        (arg("value")));
    PyImath::generate_bindings<ceil_op<double>,boost::mpl::true_>(
        "ceil",
        "return the closest integer greater than or equal to 'value'",
        (arg("value")));

    PyImath::generate_bindings<trunc_op<float>,boost::mpl::true_>(
        "trunc",
        "return the closest integer with magnitude less than or equal to 'value'",
        (arg("value")));
    PyImath::generate_bindings<trunc_op<double>,boost::mpl::true_>(
        "trunc",
        "return the closest integer with magnitude less than or equal to 'value'",
        (arg("value")));

    PyImath::generate_bindings<divs_op,boost::mpl::true_,boost::mpl::true_>(
        "divs",
        "return x/y where the remainder has the same sign as x:\n"
        "    divs(x,y) == (abs(x) / abs(y)) * (sign(x) * sign(y))\n",
        (arg("x"),arg("y")));
    PyImath::generate_bindings<mods_op,boost::mpl::true_,boost::mpl::true_>(
        "mods",
        "return x%y where the remainder has the same sign as x:\n"
        "    mods(x,y) == x - y * divs(x,y)\n",
        (arg("x"),arg("y")));

    PyImath::generate_bindings<divp_op,boost::mpl::true_,boost::mpl::true_>(
        "divp",
        "return x/y where the remainder is always positive:\n"
        "    divp(x,y) == floor (double(x) / double (y))\n",
        (arg("x"),arg("y")));
    PyImath::generate_bindings<modp_op,boost::mpl::true_,boost::mpl::true_>(
        "modp",
        "return x%y where the remainder is always positive:\n"
        "    modp(x,y) == x - y * divp(x,y)\n",
        (arg("x"),arg("y")));

    //
    // Vectorized utility functions
    // 
    PyImath::generate_bindings<rotationXYZWithUpDir_op<float>,boost::mpl::true_,boost::mpl::true_,boost::mpl::true_>(
        "rotationXYZWithUpDir",
        "return the XYZ rotation vector that rotates 'fromDir' to 'toDir'"
        "using the up vector 'upDir'",
        args("fromDir","toDir","upDir"));
}

} // namespace PyImath


