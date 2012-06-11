#ifndef INCLUDED_IMATHNOISE_H
#define INCLUDED_IMATHNOISE_H

//
//	Copyright  (c)  1999    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

//----------------------------------------------------
//
//	Perlin noise, ala renderman
//
//----------------------------------------------------

#include <ImathExport.h>
#include <ImathVec.h>

//----------------------------------------------------------------------
// README!!!!
// These noise functions are not templated because they only make sense
// for floating point types.  Furthermore, because static gradient tables
// are used, I've got a different gradient table for floats and doubles
// to avoid the cost of float-double conversion, since this is a speed
// critical function. Therefore, no templates... 
//----------------------------------------------------------------------

namespace Imath {

//----------------------
// Centered Perlin Noise
// Range -1.0 to 1.0
//----------------------

IMATH_EXPORT float noiseCen( float v );
IMATH_EXPORT double noiseCen( double v );

IMATH_EXPORT float noiseCen( const V2f &v );
IMATH_EXPORT double noiseCen( const V2d &v );

IMATH_EXPORT float noiseCen( const V3f &v );
IMATH_EXPORT double noiseCen( const V3d &v );

inline V3f noiseCen3d( const V3f &v )
{
    return V3f( noiseCen( v               ),
    	    	noiseCen( v + V3f( 7,0,3) ),
		noiseCen( v + V3f(14,0,6) ) );
}

inline V3d noiseCen3d( const V3d &v )
{
    return V3d( noiseCen( v               ),
    	    	noiseCen( v + V3d( 7,0,3) ),
		noiseCen( v + V3d(14,0,6) ) );
}

//-----------------------------
// Perlin noise centered at 0.5
// Range 0.0 to 1.0
//-----------------------------

float noise( float v );
double noise( double v );

float noise( const V2f &v );
double noise( const V2d &v );

float noise( const V3f &v );
double noise( const V3d &v );

inline V3f noise3d( const V3f &v )
{
    return V3f( noise( v               ),
    	    	noise( v + V3f( 7,0,3) ),
		noise( v + V3f(14,0,6) ) );
}

inline V3d noise3d( const V3d &v )
{
    return V3d( noise( v               ),
    	    	noise( v + V3d( 7,0,3) ),
		noise( v + V3d(14,0,6) ) );
}

 
//----------------------
// Centered Perlin noise
// With Gradients
// Range -1.0 to 1.0
//----------------------

IMATH_EXPORT float noiseCenGrad( float v, float &grad );
IMATH_EXPORT double noiseCenGrad( double v, double &grad );

IMATH_EXPORT float noiseCenGrad( const V2f &v, V2f &grad );
IMATH_EXPORT double noiseCenGrad( const V2d &v, V2d &grad );

IMATH_EXPORT float noiseCenGrad( const V3f &v, V3f &grad );
IMATH_EXPORT double noiseCenGrad( const V3d &v, V3d &grad );

//-----------------------------
// Perlin noise centered at 0.5
// With Gradients
// Range 0.0 to 1.0
//-----------------------------

float noiseGrad( float v, float &grad );
double noiseGrad( double v, double &grad );

float noiseGrad( const V2f &v, V2f &grad );
double noiseGrad( const V2d &v, V2d &grad );

float noiseGrad( const V3f &v, V3f &grad );
double noiseGrad( const V3d &v, V3d &grad );

//-----------------------
// INLINE IMPLEMENTATIONS
//-----------------------

namespace {
template <class T>
inline T remap( T v )
{
    return ((T)0.5)*v + ((T)0.5);
}
} // End unnamed namespace

inline float noise( float v )
{
    return remap( noiseCen( v ) );
}

inline double noise( double v )
{
    return remap( noiseCen( v ) );
}

inline float noise( const V2f &v )
{
    return remap( noiseCen( v ) );
}

inline double noise( const V2d &v )
{
    return remap( noiseCen( v ) );
}

inline float noise( const V3f &v )
{
    return remap( noiseCen( v ) );
}

inline double noise( const V3d &v )
{
    return remap( noiseCen( v ) );
}

inline float noiseGrad( float v, float &grad )
{
    const float ret = remap( noiseCenGrad( v, grad ) );
    grad *= ((float)0.5);
    return ret;
}

inline double noiseGrad( double v, double &grad )
{
    const double ret = remap( noiseCenGrad( v, grad ) );
    grad *= ((double)0.5);
    return ret;
}

inline float noiseGrad( const V2f &v, V2f &grad )
{
    const float ret = remap( noiseCenGrad( v, grad ) );
    grad *= ((float)0.5);
    return ret;
}

inline double noiseGrad( const V2d &v, V2d &grad )
{
    const double ret = remap( noiseCenGrad( v, grad ) );
    grad *= ((double)0.5);
    return ret;
}

inline float noiseGrad( const V3f &v, V3f &grad )
{
    const float ret = remap( noiseCenGrad( v, grad ) );
    grad *= ((float)0.5);
    return ret;
}
    
inline double noiseGrad( const V3d &v, V3d &grad )
{
    const double ret = remap( noiseCenGrad( v, grad ) );
    grad *= ((double)0.5);
    return ret;
}

} // End namespace Imath

#endif
