#ifndef INCLUDED_IEXMATHIEEE_EXC_H
#define INCLUDED_IEXMATHIEEE_EXC_H

//
//	Copyright  (c)  1997    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

//---------------------------------------------------------------------------
//
//	Names for the loating point exceptions defined by IEEE standard 754
//
//---------------------------------------------------------------------------

namespace Iex {


enum IeeeExcType
{
    IEEE_OVERFLOW  = 1,
    IEEE_UNDERFLOW = 2,
    IEEE_DIVZERO   = 4,
    IEEE_INEXACT   = 8,
    IEEE_INVALID   = 16
};


} // namespace Iex

#endif
