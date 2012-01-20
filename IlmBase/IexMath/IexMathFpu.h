#ifndef INCLUDED_IEXMATHFPU_H
#define INCLUDED_IEXMATHFPU_H

//
//	Copyright  (c)  1997    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

//------------------------------------------------------------------------
//
//	Functions to control floating point exceptions.
//
//------------------------------------------------------------------------

#include <IexMathIeeeExc.h>

namespace Iex {

//-----------------------------------------
// setFpExceptions() defines which floating
// point exceptions cause SIGFPE signals.
//-----------------------------------------

void setFpExceptions (int when = (IEEE_OVERFLOW | IEEE_DIVZERO | IEEE_INVALID));


//----------------------------------------
// fpExceptions() tells you which floating
// point exceptions cause SIGFPE signals.
//----------------------------------------

int fpExceptions ();


//------------------------------------------
// setFpExceptionHandler() defines a handler
// that will be called when SIGFPE occurs.
//------------------------------------------

extern "C" typedef void (* FpExceptionHandler) (int type, const char explanation[]);

void setFpExceptionHandler (FpExceptionHandler handler);

// -----------------------------------------
// handleExceptionsSetInRegisters() examines
// the exception registers and calls the
// floating point exception handler if the
// bits are set.  This function exists to 
// allow trapping of exception register states
// that can get set though no SIGFPE occurs.
// -----------------------------------------

void handleExceptionsSetInRegisters();

} // namespace Iex

#endif
