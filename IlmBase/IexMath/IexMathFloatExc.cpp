//
//	Copyright  (c)  1997    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

//-----------------------------------------------------
//
//	A function to control which IEEE floating
//	point exceptions will be translated into
//	C++ MathExc exceptions.
//
//-----------------------------------------------------

#include <IexMathFloatExc.h>
#include <IexMacros.h>
#include <IexMathFpu.h>

#if 0
    #include <iostream>
    #define debug(x) (std::cout << x << std::flush)
#else
    #define debug(x)
#endif

namespace Iex {
namespace {

void
fpeHandler (int type, const char explanation[])
{
    switch (type)
    {
      case IEEE_OVERFLOW:
	throw OverflowExc (explanation);

      case IEEE_UNDERFLOW:
	throw UnderflowExc (explanation);

      case IEEE_DIVZERO:
	throw DivzeroExc (explanation);

      case IEEE_INEXACT:
	throw InexactExc (explanation);

      case IEEE_INVALID:
	throw InvalidFpOpExc (explanation);
    }

    throw MathExc (explanation);
}

} // namespace


void
mathExcOn (int when)
{
    debug ("mathExcOn (when = 0x" << std::hex << when << ")\n");

    setFpExceptions (when);
    setFpExceptionHandler (fpeHandler);
}


int
getMathExcOn ()
{
    int when = fpExceptions();

    debug ("getMathExcOn () == 0x" << std::hex << when << ")\n");

    return when;
}

void
MathExcOn::handleOutstandingExceptions()
{
    handleExceptionsSetInRegisters();
}

} // namespace Iex
