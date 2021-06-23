// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef OPENEXR_CORE_TEST_BASE_H
#define OPENEXR_CORE_TEST_BASE_H

#include <string>

void testBase( const std::string &tempdir );
void testBaseErrors( const std::string &tempdir );
void testBaseLimits( const std::string &tempdir );
void testBaseDebug( const std::string &tempdir );

#endif // OPENEXR_CORE_TEST_BASE_H
