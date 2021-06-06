// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef OPENEXR_CORE_TEST_WRITE_H
#define OPENEXR_CORE_TEST_WRITE_H

#include <string>

void testWriteBadArgs( const std::string &tempdir );
void testWriteBadFiles( const std::string &tempdir );

void testWriteAttrs( const std::string &tempdir );

void testUpdateMeta( const std::string &tempdir );

void testWriteScans( const std::string &tempdir );
void testWriteTiles( const std::string &tempdir );
void testWriteMultiPart( const std::string &tempdir );
void testWriteDeep( const std::string &tempdir );

#endif // OPENEXR_CORE_TEST_WRITE_H
