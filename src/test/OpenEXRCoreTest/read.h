// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef OPENEXR_CORE_TEST_READ_H
#define OPENEXR_CORE_TEST_READ_H

#include <string>

void testReadBadArgs( const std::string &tempdir );
void testReadBadFiles( const std::string &tempdir );

void testReadMeta( const std::string &tempdir );

void testOpenScans( const std::string &tempdir );
void testOpenTiles( const std::string &tempdir );
void testOpenMultiPart( const std::string &tempdir );
void testOpenDeep( const std::string &tempdir );

void testReadScans( const std::string &tempdir );
void testReadTiles( const std::string &tempdir );
void testReadMultiPart( const std::string &tempdir );

void testReadUnpack( const std::string &tempdir );

#endif // OPENEXR_CORE_TEST_READ_H
