// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef OPENEXR_CORE_TEST_GEN_ATTR_H
#define OPENEXR_CORE_TEST_GEN_ATTR_H

#include <string>

void testAttrSizes (const std::string& tempdir);
void testAttrStrings (const std::string& tempdir);
void testAttrStringVectors (const std::string& tempdir);
void testAttrFloatVectors (const std::string& tempdir);
void testAttrChlists (const std::string& tempdir);
void testAttrPreview (const std::string& tempdir);
void testAttrOpaque (const std::string& tempdir);
void testAttrHandler (const std::string& tempdir);

void testAttrLists (const std::string& tempdir);

void testXDR (const std::string& tempdir);

#endif // OPENEXR_CORE_TEST_GEN_ATTR_H
