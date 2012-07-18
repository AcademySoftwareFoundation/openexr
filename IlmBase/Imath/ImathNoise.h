#ifndef INCLUDED_IMATHNOISE_H
#define INCLUDED_IMATHNOISE_H

//
//     Copyright (C) Pixar. All rights reserved.
//     
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//     
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//     
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//     
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
// 

//----------------------------------------------------
//
//	Perlin noise
//
//----------------------------------------------------

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

float noiseCen( float v );
double noiseCen( double v );

float noiseCen( const V2f &v );
double noiseCen( const V2d &v );

float noiseCen( const V3f &v );
double noiseCen( const V3d &v );

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

float noiseCenGrad( float v, float &grad );
double noiseCenGrad( double v, double &grad );

float noiseCenGrad( const V2f &v, V2f &grad );
double noiseCenGrad( const V2d &v, V2d &grad );

float noiseCenGrad( const V3f &v, V3f &grad );
double noiseCenGrad( const V3d &v, V3d &grad );

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
