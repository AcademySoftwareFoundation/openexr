// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef OPENEXR_CORE_TEST_COMPRESSION_H
#define OPENEXR_CORE_TEST_COMPRESSION_H

#include <string>

void testHUF( const std::string &tempdir );

void testNoCompression( const std::string &tempdir );
void testRLECompression( const std::string &tempdir );
void testZIPCompression( const std::string &tempdir );
void testZIPSCompression( const std::string &tempdir );
void testPIZCompression( const std::string &tempdir );
void testPXR24Compression( const std::string &tempdir );
void testB44Compression( const std::string &tempdir );
void testB44ACompression( const std::string &tempdir );
void testDWAACompression( const std::string &tempdir );
void testDWABCompression( const std::string &tempdir );

void testDeepNoCompression( const std::string &tempdir );
void testDeepZIPCompression( const std::string &tempdir );
void testDeepZIPSCompression( const std::string &tempdir );

#endif // OPENEXR_CORE_TEST_COMPRESSION_H
