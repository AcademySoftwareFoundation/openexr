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



#ifndef INCLUDED_IMATHFRUSTUM_H
#define INCLUDED_IMATHFRUSTUM_H


#include "ImathVec.h"
#include "ImathPlane.h"
#include "ImathLine.h"
#include "ImathMatrix.h"
#include "ImathLimits.h"
#include "ImathFun.h"
#include "IexMathExc.h"

#if defined _WIN32 || defined _WIN64
    #ifdef near
        #define _redef_near
        #undef near
    #endif
    #ifdef far
        #define _redef_far
        #undef far
    #endif
#endif

namespace Imath {

//
//	template class Frustum<T>
//
//	The frustum is always located with the eye point at the
//	origin facing down -Z. This makes the Frustum class 
//	compatable with OpenGL (or anything that assumes a camera
//	looks down -Z, hence with a right-handed coordinate system) 
//	but not with RenderMan which assumes the camera looks down
//	+Z. Additional functions are provided for conversion from
//	and from various camera coordinate spaces.
//


template<class T>
class Frustum
{
  public:
    Frustum();
    Frustum(const Frustum &);
    Frustum(T near, T far, T left, T right, T top, T bottom, bool ortho=false);
    Frustum(T near, T far, T fovx, T fovy, T aspect);
    virtual ~Frustum();

    //--------------------
    // Assignment operator
    //--------------------

    const Frustum &operator	= (const Frustum &);

    //--------------------
    //  Operators:  ==, !=
    //--------------------
    
    bool                        operator == (const Frustum<T> &src) const;
    bool                        operator != (const Frustum<T> &src) const;

    //--------------------------------------------------------
    //  Set functions change the entire state of the Frustum
    //--------------------------------------------------------

    void		set(T near, T far, 
			    T left, T right, 
			    T top, T bottom, 
			    bool ortho=false);

    void		set(T near, T far, T fovx, T fovy, T aspect);

    //------------------------------------------------------
    //	These functions modify an already valid frustum state
    //------------------------------------------------------

    void		modifyNearAndFar(T near, T far);
    void		setOrthographic(bool);

    //--------------
    //  Access
    //--------------

    bool		orthographic() const	{ return _orthographic; }
    T			near() const		{ return _near;		}
    T           hither() const      { return _near;     }
    T			far() const		{ return _far;		}
    T           yon() const     { return _far;      }
    T			left() const		{ return _left;		}
    T			right() const		{ return _right;	}
    T			bottom() const		{ return _bottom;	}
    T			top() const		{ return _top;		}

    //-----------------------------------------------------------------------
    //  Sets the planes in p to be the six bounding planes of the frustum, in
    //  the following order: top, right, bottom, left, near, far.
    //  Note that the planes have normals that point out of the frustum.
    //  The version of this routine that takes a matrix applies that matrix
    //  to transform the frustum before setting the planes.
    //-----------------------------------------------------------------------

    void		planes(Plane3<T> p[6]);
    void		planes(Plane3<T> p[6], const Matrix44<T> &M);

    //----------------------
    //  Derived Quantities
    //----------------------

    T			fovx() const;
    T			fovy() const;
    T			aspect() const;
    Matrix44<T>		projectionMatrix() const;

    //-----------------------------------------------------------------------
    //  Takes a rectangle in the screen space (i.e., -1 <= left <= right <= 1 
    //  and -1 <= bottom <= top <= 1) of this Frustum, and returns a new
    //  Frustum whose near clipping-plane window is that rectangle in local
    //  space.  
    //-----------------------------------------------------------------------

    Frustum<T>		window(T left, T right, T top, T bottom) const;

    //----------------------------------------------------------
    // Projection is in screen space / Conversion from Z-Buffer
    //----------------------------------------------------------

    Line3<T>		projectScreenToRay( const Vec2<T> & ) const;
    Vec2<T>		projectPointToScreen( const Vec3<T> & ) const;

    T			ZToDepth(long zval, long min, long max) const;
    T			normalizedZToDepth(T zval) const;
    long		DepthToZ(T depth, long zmin, long zmax) const;

    T			worldRadius(const Vec3<T> &p, T radius) const;
    T			screenRadius(const Vec3<T> &p, T radius) const;


  protected:

    Vec2<T>		screenToLocal( const Vec2<T> & ) const;
    Vec2<T>		localToScreen( const Vec2<T> & ) const;

  protected:
    T			_near;
    T			_far;
    T			_left;
    T			_right;
    T			_top;
    T			_bottom;
    bool		_orthographic;
};


template<class T>
inline Frustum<T>::Frustum()
{
    set(T (0.1),
	T (1000.0),
	T (-1.0),
	T (1.0),
	T (1.0),
	T (-1.0),
	false);
}

template<class T>
inline Frustum<T>::Frustum(const Frustum &f)
{
    *this = f;
}

template<class T>
inline Frustum<T>::Frustum(T n, T f, T l, T r, T t, T b, bool o)
{
    set(n,f,l,r,t,b,o);
}

template<class T>
inline Frustum<T>::Frustum(T near, T far, T fovx, T fovy, T aspect)
{
    set(near,far,fovx,fovy,aspect);
}

template<class T>
Frustum<T>::~Frustum()
{
}

template<class T>
const Frustum<T> &
Frustum<T>::operator = (const Frustum &f)
{
    _near         = f._near;
    _far          = f._far;
    _left         = f._left;
    _right        = f._right;
    _top          = f._top;
    _bottom       = f._bottom;
    _orthographic = f._orthographic;

    return *this;
}

template <class T>
bool
Frustum<T>::operator == (const Frustum<T> &src) const
{
    return
        _near         == src._near   &&
        _far          == src._far    &&
        _left         == src._left   &&
        _right        == src._right  &&
        _top          == src._top    &&
        _bottom       == src._bottom &&
        _orthographic == src._orthographic;
}

template <class T>
inline bool
Frustum<T>::operator != (const Frustum<T> &src) const
{
    return !operator== (src);
}

template<class T>
void Frustum<T>::set(T n, T f, T l, T r, T t, T b, bool o)
{
    _near	    = n;
    _far	    = f;
    _left	    = l;
    _right	    = r;
    _bottom	    = b;
    _top	    = t;
    _orthographic   = o;
}

template<class T>
void Frustum<T>::modifyNearAndFar(T n, T f)
{
    if ( _orthographic )
    {
	_near = n;
    }
    else
    {
	Line3<T>  lowerLeft( Vec3<T>(0,0,0), Vec3<T>(_left,_bottom,-_near) );
	Line3<T> upperRight( Vec3<T>(0,0,0), Vec3<T>(_right,_top,-_near) );
	Plane3<T> nearPlane( Vec3<T>(0,0,-1), n );

	Vec3<T> ll,ur;
	nearPlane.intersect(lowerLeft,ll);
	nearPlane.intersect(upperRight,ur);

	_left	= ll.x;
	_right	= ur.x;
	_top	= ur.y;
	_bottom	= ll.y;
	_near	= n;
	_far	= f;
    }

    _far = f;
}

template<class T>
void Frustum<T>::setOrthographic(bool ortho)
{
    _orthographic   = ortho;
}

template<class T>
void Frustum<T>::set(T near, T far, T fovx, T fovy, T aspect)
{
    if (fovx != 0 && fovy != 0)
	throw Iex::ArgExc ("fovx and fovy cannot both be non-zero.");

    if (fovx != 0)
    {
	_right	    = near * Math<T>::tan (fovx / 2);
	_left	    = -_right;
	_top	    = ((_right - _left) / aspect) / 2;
	_bottom	    = -_top;
    }
    else
    {
	_top	    = near * Math<T>::tan (fovy / 2);
	_bottom	    = -_top;
	_right	    = (_top - _bottom) * aspect / 2;
	_left	    = -_right;
    }
    _near	    = near;
    _far	    = far;
    _orthographic   = false;
}

template<class T>
T Frustum<T>::fovx() const
{
    return Math<T>::atan2(_right,_near) - Math<T>::atan2(_left,_near);
}

template<class T>
T Frustum<T>::fovy() const
{
    return Math<T>::atan2(_top,_near) - Math<T>::atan2(_bottom,_near);
}

template<class T>
T Frustum<T>::aspect() const
{
    T rightMinusLeft = _right-_left;
    T topMinusBottom = _top-_bottom;

    if (abs(topMinusBottom) < 1 &&
	abs(rightMinusLeft) > limits<T>::max() * abs(topMinusBottom))
    {
	throw Iex::DivzeroExc ("Bad viewing frustum: "
			       "aspect ratio cannot be computed.");
    }

    return rightMinusLeft / topMinusBottom;
}

template<class T>
Matrix44<T> Frustum<T>::projectionMatrix() const
{
    T rightPlusLeft  = _right+_left;
    T rightMinusLeft = _right-_left;

    T topPlusBottom  = _top+_bottom;
    T topMinusBottom = _top-_bottom;

    T farPlusNear    = _far+_near;
    T farMinusNear   = _far-_near;

    if ((abs(rightMinusLeft) < 1 &&
	 abs(rightPlusLeft) > limits<T>::max() * abs(rightMinusLeft)) ||
	(abs(topMinusBottom) < 1 &&
	 abs(topPlusBottom) > limits<T>::max() * abs(topMinusBottom)) ||
	(abs(farMinusNear) < 1 &&
	 abs(farPlusNear) > limits<T>::max() * abs(farMinusNear)))
    {
	throw Iex::DivzeroExc ("Bad viewing frustum: "
			       "projection matrix cannot be computed.");
    }

    if ( _orthographic )
    {
	T tx = -rightPlusLeft / rightMinusLeft;
	T ty = -topPlusBottom / topMinusBottom;
	T tz = -farPlusNear   / farMinusNear;

	if ((abs(rightMinusLeft) < 1 &&
	     2 > limits<T>::max() * abs(rightMinusLeft)) ||
	    (abs(topMinusBottom) < 1 &&
	     2 > limits<T>::max() * abs(topMinusBottom)) ||
	    (abs(farMinusNear) < 1 &&
	     2 > limits<T>::max() * abs(farMinusNear)))
	{
	    throw Iex::DivzeroExc ("Bad viewing frustum: "
				   "projection matrix cannot be computed.");
	}

	T A  =  2 / rightMinusLeft;
	T B  =  2 / topMinusBottom;
	T C  = -2 / farMinusNear;

	return Matrix44<T>( A,  0,  0,  0,
			    0,  B,  0,  0,
			    0,  0,  C,  0,
			    tx, ty, tz, 1.f );
    }
    else
    {
	T A =  rightPlusLeft / rightMinusLeft;
	T B =  topPlusBottom / topMinusBottom;
	T C = -farPlusNear   / farMinusNear;

	T farTimesNear = -2 * _far * _near;
	if (abs(farMinusNear) < 1 &&
	    abs(farTimesNear) > limits<T>::max() * abs(farMinusNear))
	{
	    throw Iex::DivzeroExc ("Bad viewing frustum: "
				   "projection matrix cannot be computed.");
	}

	T D = farTimesNear / farMinusNear;

	T twoTimesNear = 2 * _near;

	if ((abs(rightMinusLeft) < 1 &&
	     abs(twoTimesNear) > limits<T>::max() * abs(rightMinusLeft)) ||
	    (abs(topMinusBottom) < 1 &&
	     abs(twoTimesNear) > limits<T>::max() * abs(topMinusBottom)))
	{
	    throw Iex::DivzeroExc ("Bad viewing frustum: "
				   "projection matrix cannot be computed.");
	}

	T E = twoTimesNear / rightMinusLeft;
	T F = twoTimesNear / topMinusBottom;

	return Matrix44<T>( E,  0,  0,  0,
			    0,  F,  0,  0,
			    A,  B,  C, -1,
			    0,  0,  D,  0 );
    }
}

template<class T>
Frustum<T> Frustum<T>::window(T l, T r, T t, T b) const
{
    // move it to 0->1 space

    Vec2<T> bl = screenToLocal( Vec2<T>(l,b) );
    Vec2<T> tr = screenToLocal( Vec2<T>(r,t) );

    return Frustum<T>(_near, _far, bl.x, tr.x, tr.y, bl.y, _orthographic);
}


template<class T>
Vec2<T> Frustum<T>::screenToLocal(const Vec2<T> &s) const
{
    return Vec2<T>( _left + (_right-_left) * (1.f+s.x) / 2.f,
		    _bottom + (_top-_bottom) * (1.f+s.y) / 2.f );
}

template<class T>
Vec2<T> Frustum<T>::localToScreen(const Vec2<T> &p) const
{
    T leftPlusRight  = _left - 2 * p.x + _right;
    T leftMinusRight = _left-_right;
    T bottomPlusTop  = _bottom - 2 * p.y + _top;
    T bottomMinusTop = _bottom-_top;

    if ((abs(leftMinusRight) < 1 &&
	 abs(leftPlusRight) > limits<T>::max() * abs(leftMinusRight)) ||
	(abs(bottomMinusTop) < 1 &&
	 abs(bottomPlusTop) > limits<T>::max() * abs(bottomMinusTop)))
    {
	throw Iex::DivzeroExc
	    ("Bad viewing frustum: "
	     "local-to-screen transformation cannot be computed");
    }

    return Vec2<T>( leftPlusRight / leftMinusRight,
		    bottomPlusTop / bottomMinusTop );
}

template<class T>
Line3<T> Frustum<T>::projectScreenToRay(const Vec2<T> &p) const
{
    Vec2<T> point = screenToLocal(p);
    if (orthographic())
	return Line3<T>( Vec3<T>(point.x,point.y, 0.0),
			 Vec3<T>(point.x,point.y,-_near));
    else
	return Line3<T>( Vec3<T>(0, 0, 0), Vec3<T>(point.x,point.y,-_near));
}

template<class T>
Vec2<T> Frustum<T>::projectPointToScreen(const Vec3<T> &point) const
{
    if (orthographic() || point.z == 0)
	return localToScreen( Vec2<T>( point.x, point.y ) );
    else
	return localToScreen( Vec2<T>( point.x * _near / -point.z, 
				       point.y * _near / -point.z ) );
}

template<class T>
T Frustum<T>::ZToDepth(long zval,long zmin,long zmax) const
{
    int zdiff = zmax - zmin;

    if (zdiff == 0)
    {
	throw Iex::DivzeroExc
	    ("Bad call to Frustum::ZToDepth: zmax == zmin");
    }

    if ( zval > zmax+1 ) zval -= zdiff;

    T fzval = (T(zval) - T(zmin)) / T(zdiff);
    return normalizedZToDepth(fzval);
}

template<class T>
T Frustum<T>::normalizedZToDepth(T zval) const
{
    T Zp = zval * 2.0 - 1;

    if ( _orthographic )
    {
        return   -(Zp*(_far-_near) + (_far+_near))/2;
    }
    else 
    {
	T farTimesNear = 2 * _far * _near;
	T farMinusNear = Zp * (_far - _near) - _far - _near;

	if (abs(farMinusNear) < 1 &&
	    abs(farTimesNear) > limits<T>::max() * abs(farMinusNear))
	{
	    throw Iex::DivzeroExc
		("Frustum::normalizedZToDepth cannot be computed.  The "
		 "near and far clipping planes of the viewing frustum "
		 "may be too close to each other");
	}

	return farTimesNear / farMinusNear;
    }
}

template<class T>
long Frustum<T>::DepthToZ(T depth,long zmin,long zmax) const
{
    long zdiff     = zmax - zmin;
    T farMinusNear = _far-_near;

    if ( _orthographic )
    {
	T farPlusNear = 2*depth + _far + _near;

	if (abs(farMinusNear) < 1 &&
	    abs(farPlusNear) > limits<T>::max() * abs(farMinusNear))
	{
	    throw Iex::DivzeroExc
		("Bad viewing frustum: near and far clipping planes "
		 "are too close to each other");
	}

	T Zp = -farPlusNear/farMinusNear;
	return long(0.5*(Zp+1)*zdiff) + zmin;
    }
    else 
    { 
	// Perspective

	T farTimesNear = 2*_far*_near;
	if (abs(depth) < 1 &&
	    abs(farTimesNear) > limits<T>::max() * abs(depth))
	{
	    throw Iex::DivzeroExc
		("Bad call to DepthToZ function: value of `depth' "
		 "is too small");
	}

	T farPlusNear = farTimesNear/depth + _far + _near;
	if (abs(farMinusNear) < 1 &&
	    abs(farPlusNear) > limits<T>::max() * abs(farMinusNear))
	{
	    throw Iex::DivzeroExc
		("Bad viewing frustum: near and far clipping planes "
		 "are too close to each other");
	}

	T Zp = farPlusNear/farMinusNear;
	return long(0.5*(Zp+1)*zdiff) + zmin;
    }
}

template<class T>
T Frustum<T>::screenRadius(const Vec3<T> &p, T radius) const
{
    // Derivation:
    // Consider X-Z plane.
    // X coord of projection of p = xp = p.x * (-_near / p.z)
    // Let q be p + (radius, 0, 0).
    // X coord of projection of q = xq = (p.x - radius)  * (-_near / p.z)
    // X coord of projection of segment from p to q = r = xp - xq
    //         = radius * (-_near / p.z)
    // A similar analysis holds in the Y-Z plane.
    // So r is the quantity we want to return.

    if (abs(p.z) > 1 || abs(-_near) < limits<T>::max() * abs(p.z))
    {
	return radius * (-_near / p.z);
    }
    else
    {
	throw Iex::DivzeroExc
	    ("Bad call to Frustum::screenRadius: the magnitude of `p' "
	     "is too small");
    }

    return radius * (-_near / p.z);
}

template<class T>
T Frustum<T>::worldRadius(const Vec3<T> &p, T radius) const
{
    if (abs(-_near) > 1 || abs(p.z) < limits<T>::max() * abs(-_near))
    {
	return radius * (p.z / -_near);
    }
    else
    {
	throw Iex::DivzeroExc
	    ("Bad viewing frustum: the near clipping plane is too "
	     "close to zero");
    }
}

template<class T>
void Frustum<T>::planes(Plane3<T> p[6])
{
    //
    //	Plane order: Top, Right, Bottom, Left, Near, Far.
    //  Normals point outwards.
    //

    if (! _orthographic)
    {
        Vec3<T> a( _left,  _bottom, -_near);
        Vec3<T> b( _left,  _top,    -_near);
        Vec3<T> c( _right, _top,    -_near);
        Vec3<T> d( _right, _bottom, -_near);
        Vec3<T> o(0,0,0);

        p[0].set( o, c, b );
        p[1].set( o, d, c );
        p[2].set( o, a, d );
        p[3].set( o, b, a );
    }
    else
    {
        p[0].set( Vec3<T>( 0, 1, 0), _top );
        p[1].set( Vec3<T>( 1, 0, 0), _right );
        p[2].set( Vec3<T>( 0,-1, 0),-_bottom );
        p[3].set( Vec3<T>(-1, 0, 0),-_left );
    }
    p[4].set( Vec3<T>(0, 0, 1), -_near );
    p[5].set( Vec3<T>(0, 0,-1), _far );
}


template<class T>
void Frustum<T>::planes(Plane3<T> p[6], const Matrix44<T> &M)
{
    //
    //	Plane order: Top, Right, Bottom, Left, Near, Far.
    //  Normals point outwards.
    //

    Vec3<T> a   = Vec3<T>( _left,  _bottom, -_near) * M;
    Vec3<T> b   = Vec3<T>( _left,  _top,    -_near) * M;
    Vec3<T> c   = Vec3<T>( _right, _top,    -_near) * M;
    Vec3<T> d   = Vec3<T>( _right, _bottom, -_near) * M;
    if (! _orthographic)
    {
        double s    = _far / double(_near);
        T farLeft   = (T) (s * _left);
        T farRight  = (T) (s * _right);
        T farTop    = (T) (s * _top);
        T farBottom = (T) (s * _bottom);
        Vec3<T> e   = Vec3<T>( farLeft,  farBottom, -_far) * M;
        Vec3<T> f   = Vec3<T>( farLeft,  farTop,    -_far) * M;
        Vec3<T> g   = Vec3<T>( farRight, farTop,    -_far) * M;
        Vec3<T> o   = Vec3<T>(0,0,0) * M;
        p[0].set( o, c, b );
        p[1].set( o, d, c );
        p[2].set( o, a, d );
        p[3].set( o, b, a );
        p[4].set( a, d, c );
        p[5].set( e, f, g );
     }
    else
    {
        Vec3<T> e   = Vec3<T>( _left,  _bottom, -_far) * M;
        Vec3<T> f   = Vec3<T>( _left,  _top,    -_far) * M;
        Vec3<T> g   = Vec3<T>( _right, _top,    -_far) * M;
        Vec3<T> h   = Vec3<T>( _right, _bottom, -_far) * M;
        p[0].set( c, g, f );
        p[1].set( d, h, g );
        p[2].set( a, e, h );
        p[3].set( b, f, e );
        p[4].set( a, d, c );
        p[5].set( e, f, g );
    }
}

typedef Frustum<float>	Frustumf;
typedef Frustum<double> Frustumd;


} // namespace Imath


#if defined _WIN32 || defined _WIN64
    #ifdef _redef_near
        #define near
    #endif
    #ifdef _redef_far
        #define far
    #endif
#endif

#endif
