//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IEXMATHFLOATEXC_H
#define INCLUDED_IEXMATHFLOATEXC_H

#ifndef IEXMATH_EXPORT_H
#define IEXMATH_EXPORT_H

#if defined(OPENEXR_DLL)
    #if defined(IEXMATH_EXPORTS)
    #define IEXMATH_EXPORT __declspec(dllexport)
    #else
    #define IEXMATH_EXPORT __declspec(dllimport)
    #endif
    #define IEXMATH_EXPORT_CONST
#else
    #define IEXMATH_EXPORT
    #define IEXMATH_EXPORT_CONST const
#endif

#endif

#include "IexNamespace.h"
#include "IexMathExc.h"
//#include <IexBaseExc.h>
#include "IexMathIeeeExc.h"

IEX_INTERNAL_NAMESPACE_HEADER_ENTER


//-------------------------------------------------------------
// Function mathExcOn() defines which floating point exceptions
// will be trapped and converted to C++ exceptions.
//-------------------------------------------------------------

IEXMATH_EXPORT
void mathExcOn (int when = (IEEE_OVERFLOW | IEEE_DIVZERO | IEEE_INVALID));


//----------------------------------------------------------------------
// Function getMathExcOn() tells you for which floating point exceptions
// trapping and conversion to C++ exceptions is currently enabled.
//----------------------------------------------------------------------

IEXMATH_EXPORT
int getMathExcOn();


//------------------------------------------------------------------------
// A classs that temporarily sets floating point exception trapping
// and conversion, and later restores the previous settings.
//
// Example:
//
//	float
//	trickyComputation (float x)
//	{
//	    MathExcOn meo (0);		// temporarily disable floating
//	    				// point exception trapping
//
//	    float result = ...;		// computation which may cause
//	    				// floating point exceptions
//
//	    return result;		// destruction of meo restores
//	}				// the program's previous floating
//					// point exception settings
//------------------------------------------------------------------------

class MathExcOn
{
  public:

    IEXMATH_EXPORT MathExcOn (int when);
    IEXMATH_EXPORT ~MathExcOn ();
    MathExcOn (const MathExcOn&) = delete;
    MathExcOn& operator= (const MathExcOn&) = delete;
    MathExcOn (MathExcOn&&) = delete;
    MathExcOn& operator= (MathExcOn&&) = delete;

    // It is possible for functions to set the exception registers
    // yet not trigger a SIGFPE.  Specifically, the implementation
    // of pow(x, y) we're using can generates a NaN from a negative x
    // and fractional y but a SIGFPE is not generated.
    // This function examimes the exception registers and calls the
    // fpHandler if those registers modulo the exception mask are set.
    // It should be called wherever this class is commonly used where it has
    // been found that certain floating point exceptions are not being thrown.

    IEXMATH_EXPORT void handleOutstandingExceptions();

  private:

    bool                        _changed;
    int	                        _saved;
};


IEX_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
