#ifndef INCLUDED_MAKE_TILED_H
#define INCLUDED_MAKE_TILED_H

//
//	Copyright  (c)  2004    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

//----------------------------------------------------------------------------
//
//	Produce a tiled version of an OpenEXR image.
//
//----------------------------------------------------------------------------

#include <ImfTileDescription.h>
#include <string>
#include <set>


void	makeTiled (const char inFileName[],
	           const char outFileName[],
		   Imf::LevelMode mode,
		   int tileSizeX,
		   int tileSizeY,
		   const std::set<std::string> &doNotFilter,
		   bool verbose);


#endif
