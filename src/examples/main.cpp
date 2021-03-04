//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#include "rgbaInterfaceExamples.h"
#include "rgbaInterfaceTiledExamples.h"
#include "generalInterfaceExamples.h"
#include "generalInterfaceTiledExamples.h"
#include "lowLevelIoExamples.h"
#include "previewImageExamples.h"

#include <iostream>
#include <stdexcept>


int
main (int argc, char *argv[])
{
    try
    {
	rgbaInterfaceExamples();
	generalInterfaceExamples();

        rgbaInterfaceTiledExamples();
        generalInterfaceTiledExamples();

	lowLevelIoExamples();

	previewImageExamples();
    }
    catch (const std::exception &exc)
    {
	std::cerr << exc.what() << std::endl;
	return 1;
    }

    return 0;
}
