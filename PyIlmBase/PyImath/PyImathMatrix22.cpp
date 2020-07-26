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

#include "PyIlmBaseConfigInternal.h"

#define BOOST_PYTHON_MAX_ARITY 17

#include "PyImathMatrix.h"
#include "PyImathExport.h"
#include "PyImathDecorators.h"
#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/format.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/raw_function.hpp>
#include "PyImath.h"
#include "PyImathVec.h"
#include "PyImathMathExc.h"
#include <ImathVec.h>
#include <ImathMatrixAlgo.h>
#include <Iex.h>

namespace PyImath {

template<> const char *PyImath::M22fArray::name() { return "M22fArray"; }
template<> const char *PyImath::M22dArray::name() { return "M22dArray"; }

using namespace boost::python;
using namespace IMATH_NAMESPACE;

template <class T, int len>
struct MatrixRow {
    explicit MatrixRow(T *data) : _data(data) {}
    T & operator [] (int i) { return _data[i]; }
    T *_data;

    static const char *name;
    static void register_class()
    {
        typedef PyImath::StaticFixedArray<MatrixRow,T,len> MatrixRow_helper;
        class_<MatrixRow> matrixRow_class(name,no_init);
        matrixRow_class
            .def("__len__", MatrixRow_helper::len)
            .def("__getitem__", MatrixRow_helper::getitem,return_value_policy<copy_non_const_reference>())
            .def("__setitem__", MatrixRow_helper::setitem)
            ;
    }
};

template <> const char *MatrixRow<float,2>::name = "M22fRow";
template <> const char *MatrixRow<double,2>::name = "M22dRow";


template <class Container, class Data, int len>
struct IndexAccessMatrixRow {
    typedef MatrixRow<Data,len> result_type;
    static MatrixRow<Data,len> apply(Container &c, int i) { return MatrixRow<Data,len>(c[i]); }
};

template <class T> struct Matrix22Name { static const char *value; };
template<> const char *Matrix22Name<float>::value  = "M22f";
template<> const char *Matrix22Name<double>::value = "M22d";

template <class T>
static std::string Matrix22_str(const Matrix22<T> &v)
{
    std::stringstream stream;
    stream << Matrix22Name<T>::value << "(";
    for (int row = 0; row < 2; row++)
    {
        stream << "(";
	for (int col = 0; col < 2; col++)
	{
	    stream << v[row][col];
            stream << (col != 1 ? ", " : "");
	}
        stream << ")" << (row != 1 ? ", " : "");
    }
    stream << ")";
    return stream.str();
}

// Non-specialized repr is same as str
template <class T>
static std::string Matrix22_repr(const Matrix22<T> &v)
{
    return Matrix22_str(v);
}

// Specialization for float to full precision
template <>
std::string Matrix22_repr(const Matrix22<float> &v)
{
    return (boost::format("%s((%.9g, %.9g), (%.9g, %.9g))")
                        % Matrix22Name<float>::value
                        % v[0][0] % v[0][1]
                        % v[1][0] % v[1][1]).str();
}

// Specialization for double to full precision
template <>
std::string Matrix22_repr(const Matrix22<double> &v)
{
    return (boost::format("%s((%.17g, %.17g), (%.17g, %.17g))")
                        % Matrix22Name<double>::value
                        % v[0][0] % v[0][1]
                        % v[1][0] % v[1][1]).str();
}

template <class T>
static const Matrix22<T> &
invert22 (Matrix22<T> &m, bool singExc = true)
{
    MATH_EXC_ON;
    return m.invert(singExc);
}

template <class T>
static Matrix22<T>
inverse22 (Matrix22<T> &m, bool singExc = true)
{
    MATH_EXC_ON;
    return m.inverse(singExc);
}

template <class T, class U>
static const Matrix22<T> &
iadd22(Matrix22<T> &m, const Matrix22<U> &m2)
{
    MATH_EXC_ON;
    Matrix22<T> m3;
    m3.setValue (m2);
    return m += m3;
}

template <class T>
static const Matrix22<T> &
iadd22T(Matrix22<T> &mat, T a)
{
    MATH_EXC_ON;
    return mat += a;
}

template <class T>
static Matrix22<T>
add22(Matrix22<T> &m, const Matrix22<T> &m2)
{
    MATH_EXC_ON;
    return m + m2;
}

template <class T, class U>
static const Matrix22<T> &
isub22(Matrix22<T> &m, const Matrix22<U> &m2)
{
    MATH_EXC_ON;
    Matrix22<T> m3;
    m3.setValue (m2);
    return m -= m3;
}

template <class T>
static const Matrix22<T> &
isub22T(Matrix22<T> &mat, T a)
{
    MATH_EXC_ON;
    return mat -= a;
}

template <class T>
static Matrix22<T>
sub22(Matrix22<T> &m, const Matrix22<T> &m2)
{
    MATH_EXC_ON;
    return m - m2;
}

template <class T>
static const Matrix22<T> &
negate22 (Matrix22<T> &m)
{
    MATH_EXC_ON;
    return m.negate();
}

template <class T>
static Matrix22<T>
neg22 (Matrix22<T> &m)
{
    MATH_EXC_ON;
    return -m;
}

template <class T>
static const Matrix22<T> &
imul22T(Matrix22<T> &m, const T &t)
{
    MATH_EXC_ON;
    return m *= t;
}

template <class T>
static Matrix22<T>
mul22T(Matrix22<T> &m, const T &t)
{
    MATH_EXC_ON;
    return m * t;
}

template <class T>
static Matrix22<T>
rmul22T(Matrix22<T> &m, const T &t)
{
    MATH_EXC_ON;
    return t * m;
}

template <class T>
static const Matrix22<T> &
idiv22T(Matrix22<T> &m, const T &t)
{
    MATH_EXC_ON;
    return m /= t;
}

template <class T>
static Matrix22<T>
div22T(Matrix22<T> &m, const T &t)
{
    MATH_EXC_ON;
    return m / t;
}

template <class T>
void
outerProduct22(Matrix22<T> &mat, const Vec2<T> &a, const Vec2<T> &b)
{
    MATH_EXC_ON;
    mat = IMATH_NAMESPACE::outerProduct(a,b);
}

template <class TV,class TM>
static void
multDirMatrix22(Matrix22<TM> &mat, const Vec2<TV> &src, Vec2<TV> &dst)
{
    MATH_EXC_ON;
    mat.multDirMatrix(src, dst);    
}

template <class TV,class TM>
static Vec2<TV>
multDirMatrix22_return_value(Matrix22<TM> &mat, const Vec2<TV> &src)
{
    MATH_EXC_ON;
    Vec2<TV> dst;
    mat.multDirMatrix(src, dst);    
    return dst;
}

template <class TV,class TM>
static FixedArray<Vec2<TV> >
multDirMatrix22_array(Matrix22<TM> &mat, const FixedArray<Vec2<TV> >&src)
{
    MATH_EXC_ON;
    size_t len = src.len();
    FixedArray<Vec2<TV> > dst(len);
    for (size_t i=0; i<len; ++i) mat.multDirMatrix(src[i], dst[i]);    
    return dst;
}

template <class T>
static const Matrix22<T> &
rotate22(Matrix22<T> &mat, const T &r)
{
    MATH_EXC_ON;
    return mat.rotate(r);    
}

template <class T>
static void
extractEuler(Matrix22<T> &mat, Vec2<T> &dstObj)
{
    MATH_EXC_ON;
    T dst;
    IMATH_NAMESPACE::extractEuler(mat, dst);
    dstObj.setValue(dst, T (0));
}

template <class T>
static const Matrix22<T> &
scaleSc22(Matrix22<T> &mat, const T &s)
{
    MATH_EXC_ON;
    Vec2<T> sVec(s, s);
    return mat.scale(sVec);
}

template <class T>
static const Matrix22<T> &
scaleV22(Matrix22<T> &mat, const Vec2<T> &s)
{
    MATH_EXC_ON;
    return mat.scale(s);
}

template <class T>
static const Matrix22<T> &
scale22Tuple(Matrix22<T> &mat, const tuple &t)
{
    MATH_EXC_ON;
    if(t.attr("__len__")() == 2)
    {
        Vec2<T> s;
        s.x = extract<T>(t[0]);
        s.y = extract<T>(t[1]);
        
        return mat.scale(s);
    }
    else
        THROW(IEX_NAMESPACE::LogicExc, "m.scale needs tuple of length 2");
}

template <class T>
static const Matrix22<T> &
setRotation22(Matrix22<T> &mat, const T &r)
{
    MATH_EXC_ON;
    return mat.setRotation(r);    
}

template <class T>
static const Matrix22<T> &
setScaleSc22(Matrix22<T> &mat, const T &s)
{
    MATH_EXC_ON;
    Vec2<T> sVec(s, s);
    return mat.setScale(sVec);
}

template <class T>
static const Matrix22<T> &
setScaleV22(Matrix22<T> &mat, const Vec2<T> &s)
{
    MATH_EXC_ON;
    return mat.setScale(s);
}

template <class T>
static const Matrix22<T> &
setScale22Tuple(Matrix22<T> &mat, const tuple &t)
{
    MATH_EXC_ON;
    if(t.attr("__len__")() == 2)
    {
        Vec2<T> s;
        s.x = extract<T>(t[0]);
        s.y = extract<T>(t[1]);
        
        return mat.setScale(s);
    }
    else
        THROW(IEX_NAMESPACE::LogicExc, "m.setScale needs tuple of length 2");
}

template <class T>
static void
setValue22(Matrix22<T> &mat, const Matrix22<T> &value)
{
    MATH_EXC_ON;
    mat.setValue(value);
}

template <class T>
static Matrix22<T>
subtractTL22(Matrix22<T> &mat, T a)
{
    MATH_EXC_ON;
    Matrix22<T> m(mat.x);
    for(int i = 0; i < 2; ++i)
        for(int j = 0; j < 2; ++j)
            m.x[i][j] -= a;
    
    return m;
}

template <class T>
static Matrix22<T>
subtractTR22(Matrix22<T> &mat, T a)
{
    MATH_EXC_ON;
    Matrix22<T> m(mat.x);
    for(int i = 0; i < 2; ++i)
        for(int j = 0; j < 2; ++j)
            m.x[i][j] = a - m.x[i][j];
    
    return m;
}


template <class T>
static Matrix22<T>
add22T(Matrix22<T> &mat, T a)
{
    MATH_EXC_ON;
    Matrix22<T> m(mat.x);
    for(int i = 0; i < 2; ++i)
        for(int j = 0; j < 2; ++j)
            m.x[i][j] += a;
    
    return m;
}

template <class S, class T>
static Matrix22<T>
mul22(Matrix22<T> &mat1, Matrix22<S> &mat2)
{
    MATH_EXC_ON;
    Matrix22<T> mat2T;
    mat2T.setValue (mat2);
    return mat1 * mat2T;
}

template <class S, class T>
static Matrix22<T>
rmul22(Matrix22<T> &mat2, Matrix22<S> &mat1)
{
    MATH_EXC_ON;
    Matrix22<T> mat1T;
    mat1T.setValue (mat1);
    return mat1T * mat2;
}

template <class S, class T>
static const Matrix22<T> &
imul22(Matrix22<T> &mat1, Matrix22<S> &mat2)
{
    MATH_EXC_ON;
    Matrix22<T> mat2T;
    mat2T.setValue (mat2);
    return mat1 *= mat2T;
}

template <class T>
static bool
lessThan22(Matrix22<T> &mat1, const Matrix22<T> &mat2)
{
    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 2; ++j){
            if(mat1[i][j] > mat2[i][j]){
                return false;
            }
        }
    }
    
    return (mat1 != mat2);            
}

template <class T>
static bool
lessThanEqual22(Matrix22<T> &mat1, const Matrix22<T> &mat2)
{
    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 2; ++j){
            if(mat1[i][j] > mat2[i][j]){
                return false;
            }
        }
    }
    
    return true;            
}

template <class T>
static bool
greaterThan22(Matrix22<T> &mat1, const Matrix22<T> &mat2)
{
    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 2; ++j){
            if(mat1[i][j] < mat2[i][j]){
                std::cout << mat1[i][j] << " " << mat2[i][j] << std::endl;
                return false;
            }
        }
    }
    
    return (mat1 != mat2);            
}

template <class T>
static bool
greaterThanEqual22(Matrix22<T> &mat1, const Matrix22<T> &mat2)
{
    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 2; ++j){
            if(mat1[i][j] < mat2[i][j]){
                return false;
            }
        }
    }
    
    return true;            
}

BOOST_PYTHON_FUNCTION_OVERLOADS(invert22_overloads, invert22, 1, 2);
BOOST_PYTHON_FUNCTION_OVERLOADS(inverse22_overloads, inverse22, 1, 2);
BOOST_PYTHON_FUNCTION_OVERLOADS(outerProduct22_overloads, outerProduct22, 3, 3);

template <class T>
static Matrix22<T> * Matrix2_tuple_constructor(const tuple &t0, const tuple &t1)
{
  if(t0.attr("__len__")() == 2 && t1.attr("__len__")() == 2)
  {
      return new Matrix22<T>(extract<T>(t0[0]),  extract<T>(t0[1]),
                             extract<T>(t1[0]),  extract<T>(t1[1]));
  }
  else
      THROW(IEX_NAMESPACE::LogicExc, "Matrix22 takes 2 tuples of length 2");
}

template <class T, class S>
static Matrix22<T> *Matrix2_matrix_constructor(const Matrix22<S> &mat)
{
    Matrix22<T> *m = new Matrix22<T>;
    
    for(int i = 0; i < 2; ++i)
        for(int j = 0; j < 2; ++j)
            m->x[i][j] = T (mat.x[i][j]);
    
    return m;
}

template <class T>
class_<Matrix22<T> >
register_Matrix22()
{
    typedef PyImath::StaticFixedArray<Matrix22<T>,T,2,IndexAccessMatrixRow<Matrix22<T>,T,2> > Matrix22_helper;

    MatrixRow<T,2>::register_class();
    class_<Matrix22<T> > matrix22_class(Matrix22Name<T>::value, Matrix22Name<T>::value,init<Matrix22<T> >("copy construction"));
    matrix22_class
        .def(init<>("initialize to identity"))
        .def(init<T>("initialize all entries to a single value"))
        .def(init<T,T,T,T>("make from components"))
        .def("__init__", make_constructor(Matrix2_tuple_constructor<T>))
        .def("__init__", make_constructor(Matrix2_matrix_constructor<T,float>))
        .def("__init__", make_constructor(Matrix2_matrix_constructor<T,double>))
        
	//.def_readwrite("x00", &Matrix22<T>::x[0][0])
	//.def_readwrite("x01", &Matrix22<T>::x[0][1])
	//.def_readwrite("x02", &Matrix22<T>::x[0][2])
	//.def_readwrite("x10", &Matrix22<T>::x[1][0])
	//.def_readwrite("x11", &Matrix22<T>::x[1][1])
	//.def_readwrite("x12", &Matrix22<T>::x[1][2])
	//.def_readwrite("x20", &Matrix22<T>::x[2][0])
	//.def_readwrite("x21", &Matrix22<T>::x[2][1])
	//.def_readwrite("x22", &Matrix22<T>::x[2][2])
        .def("baseTypeEpsilon", &Matrix22<T>::baseTypeEpsilon,"baseTypeEpsilon() epsilon value of the base type of the vector")
        .staticmethod("baseTypeEpsilon")
        .def("baseTypeMax", &Matrix22<T>::baseTypeMax,"baseTypeMax() max value of the base type of the vector")
        .staticmethod("baseTypeMax")
        .def("baseTypeMin", &Matrix22<T>::baseTypeMin,"baseTypeMin() min value of the base type of the vector")
        .staticmethod("baseTypeMin")
        .def("baseTypeSmallest", &Matrix22<T>::baseTypeSmallest,"baseTypeSmallest() smallest value of the base type of the vector")
        .staticmethod("baseTypeSmallest")
        .def("equalWithAbsError", &Matrix22<T>::equalWithAbsError,"m1.equalWithAbsError(m2,e) true if the elements "
             "of v1 and v2 are the same with an absolute error of no more than e, "
             "i.e., abs(m1[i] - m2[i]) <= e")
        .def("equalWithRelError", &Matrix22<T>::equalWithRelError,"m1.equalWithAbsError(m2,e) true if the elements "
             "of m1 and m2 are the same with an absolute error of no more than e, "
             "i.e., abs(m1[i] - m2[i]) <= e * abs(m1[i])")
        // need a different version for matrix data access
        .def("__len__", Matrix22_helper::len)
        .def("__getitem__", Matrix22_helper::getitem)
	//.def("__setitem__", Matrix22_helper::setitem)
        .def("makeIdentity",&Matrix22<T>::makeIdentity,"makeIdentity() make this matrix the identity matrix")
        .def("transpose",&Matrix22<T>::transpose,return_internal_reference<>(),"transpose() transpose this matrix")
        .def("transposed",&Matrix22<T>::transposed,"transposed() return a transposed copy of this matrix")
        .def("invert",&invert22<T>,invert22_overloads("invert() invert this matrix")[return_internal_reference<>()])
        .def("inverse",&inverse22<T>,inverse22_overloads("inverse() return an inverted copy of this matrix"))
        .def("determinant",&Matrix22<T>::determinant,"determinant() return the determinant of this matrix")
        .def(self == self) // NOSONAR - suppress SonarCloud bug report.
        .def(self != self) // NOSONAR - suppress SonarCloud bug report.
        .def("__iadd__", &iadd22<T, float>,return_internal_reference<>())
        .def("__iadd__", &iadd22<T, double>,return_internal_reference<>())
        .def("__iadd__", &iadd22T<T>,return_internal_reference<>())
        .def("__add__", &add22<T>)
        .def("__isub__", &isub22<T, float>,return_internal_reference<>())
        .def("__isub__", &isub22<T, double>,return_internal_reference<>())
        .def("__isub__", &isub22T<T>,return_internal_reference<>())
        .def("__sub__", &sub22<T>)
        .def("negate",&negate22<T>,return_internal_reference<>(),"negate() negate all entries in this matrix")
        .def("__neg__", &neg22<T>)
        .def("__imul__", &imul22T<T>,return_internal_reference<>())
        .def("__mul__", &mul22T<T>)
        .def("__rmul__", &rmul22T<T>)
        .def("__idiv__", &idiv22T<T>,return_internal_reference<>())
        .def("__itruediv__", &idiv22T<T>,return_internal_reference<>())
        .def("__div__", &div22T<T>)
        .def("__truediv__", &div22T<T>)
        .def("__add__", &add22T<T>)
        .def("__radd__", &add22T<T>)
        .def("__sub__", &subtractTL22<T>)
        .def("__rsub__", &subtractTR22<T>)
        .def("__mul__", &mul22<float, T>)
        .def("__mul__", &mul22<double, T>)
        .def("__rmul__", &rmul22<float, T>)
        .def("__rmul__", &rmul22<double, T>)
        .def("__imul__", &imul22<float, T>,return_internal_reference<>())
        .def("__imul__", &imul22<double, T>,return_internal_reference<>())
        .def("__lt__", &lessThan22<T>)
        .def("__le__", &lessThanEqual22<T>)
        .def("__gt__", &greaterThan22<T>)
        .def("__ge__", &greaterThanEqual22<T>)
	//.def(self_ns::str(self))
        .def("__str__",&Matrix22_str<T>)
        .def("__repr__",&Matrix22_repr<T>)

         .def("extractEuler", &extractEuler<T>, 				
              "M.extractEuler(r) -- extracts the "
			  "rotation component of M into r. "
              "Assumes that M contains no shear or "
              "non-uniform scaling; results are "
              "meaningless if it does.")
              
         .def("multDirMatrix", &multDirMatrix22<double,T>, "mult matrix")
         .def("multDirMatrix", &multDirMatrix22_return_value<double,T>, "mult matrix")
         .def("multDirMatrix", &multDirMatrix22_array<double,T>, "mult matrix")
         .def("multDirMatrix", &multDirMatrix22<float,T>, "mult matrix")
         .def("multDirMatrix", &multDirMatrix22_return_value<float,T>, "mult matrix")
         .def("multDirMatrix", &multDirMatrix22_array<float,T>, "mult matrix")

         .def("rotate", &rotate22<T>, return_internal_reference<>(),"rotate matrix")

         .def("scale", &scaleSc22<T>, return_internal_reference<>(),"scale matrix")
         .def("scale", &scaleV22<T>, return_internal_reference<>(),"scale matrix")
         .def("scale", &scale22Tuple<T>, return_internal_reference<>(),"scale matrix")

         .def("setRotation", &setRotation22<T>, return_internal_reference<>(),"setRotation()")
         .def("setScale", &setScaleSc22<T>, return_internal_reference<>(),"setScale()")
         .def("setScale", &setScaleV22<T>, return_internal_reference<>(),"setScale()")
         .def("setScale", &setScale22Tuple<T>, return_internal_reference<>(),"setScale()")

         .def("setValue", &setValue22<T>, "setValue()")
         ;

    decoratecopy(matrix22_class);

    return matrix22_class;
/*
    const Matrix22 &	operator = (const Matrix22 &v);
    const Matrix22 &	operator = (T a);
    T *			getValue ();
    const T *		getValue () const;
    template <class S> void getValue (Matrix22<S> &v) const;
    template <class S> Matrix22 & setValue (const Matrix22<S> &v);
    template <class S> Matrix22 & setTheMatrix (const Matrix22<S> &v);
    template <class S> void multVecMatrix(const Vec2<S> &src, Vec2<S> &dst) const;
    template <class S> void multDirMatrix(const Vec2<S> &src, Vec2<S> &dst) const;
    template <class S> const Matrix22 &	setRotation (S r);
    template <class S> const Matrix22 &	rotate (S r);
    const Matrix22 &	setScale (T s);
    template <class S> const Matrix22 &	setScale (const Vec2<S> &s);
    template <class S> const Matrix22 &	scale (const Vec2<S> &s);
    template <class S> const Matrix22 &	setTranslation (const Vec2<S> &t);
    Vec2<T>		translation () const;
    template <class S> const Matrix22 &	translate (const Vec2<S> &t);
    template <class S> const Matrix22 &	setShear (const S &h);
    template <class S> const Matrix22 &	setShear (const Vec2<S> &h);
    template <class S> const Matrix22 &	shear (const S &xy);
    template <class S> const Matrix22 &	shear (const Vec2<S> &h);
*/
}

template <class T>
static void
setM22ArrayItem(FixedArray<IMATH_NAMESPACE::Matrix22<T> > &ma,
                Py_ssize_t index,
                const IMATH_NAMESPACE::Matrix22<T> &m)
{
    ma[ma.canonical_index(index)] = m;
}

template <class T>
class_<FixedArray<IMATH_NAMESPACE::Matrix22<T> > >
register_M22Array()
{
    class_<FixedArray<IMATH_NAMESPACE::Matrix22<T> > > matrixArray_class = FixedArray<IMATH_NAMESPACE::Matrix22<T> >::register_("Fixed length array of IMATH_NAMESPACE::Matrix22");
    matrixArray_class
         .def("__setitem__", &setM22ArrayItem<T>)
        ;
    return matrixArray_class;
}

template PYIMATH_EXPORT class_<IMATH_NAMESPACE::Matrix22<float> > register_Matrix22<float>();
template PYIMATH_EXPORT class_<IMATH_NAMESPACE::Matrix22<double> > register_Matrix22<double>();

template PYIMATH_EXPORT class_<FixedArray<IMATH_NAMESPACE::Matrix22<float> > > register_M22Array<float>();
template PYIMATH_EXPORT class_<FixedArray<IMATH_NAMESPACE::Matrix22<double> > > register_M22Array<double>();


template<> PYIMATH_EXPORT IMATH_NAMESPACE::Matrix22<float> FixedArrayDefaultValue<IMATH_NAMESPACE::Matrix22<float> >::value() { return IMATH_NAMESPACE::Matrix22<float>(); }
template<> PYIMATH_EXPORT IMATH_NAMESPACE::Matrix22<double> FixedArrayDefaultValue<IMATH_NAMESPACE::Matrix22<double> >::value() { return IMATH_NAMESPACE::Matrix22<double>(); }
}
