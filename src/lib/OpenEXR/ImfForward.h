//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_FORWARD_H
#define INCLUDED_IMF_FORWARD_H

////////////////////////////////////////////////////////////////////
//
// Forward declarations for OpenEXR - correctly declares namespace
//
////////////////////////////////////////////////////////////////////

#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


// classes for basic types;
template<class T> class Array;
template<class T> class Array2D;
struct Channel;
class  ChannelList;
struct Chromaticities;

// attributes used in headers are TypedAttributes
class Attribute;

class Header;

// file handling classes
class OutputFile;
class TiledInputFile;
class ScanLineInputFile;
class InputFile;
class TiledOutputFile;
class DeepScanLineInputFile;
class DeepScanLineOutputFile;
class DeepTiledInputFile;
class DeepTiledOutputFile;
class AcesInputFile;
class AcesOutputFile;
class TiledInputPart;
class TiledInputFile;
class TileOffsets;

// multipart file handling
class GenericInputFile;
class GenericOutputFile;
class MultiPartInputFile;
class MultiPartOutputFile;

class InputPart;
class TiledInputPart;
class DeepScanLineInputPart;
class DeepTiledInputPart;

class OutputPart;
class ScanLineOutputPart;
class TiledOutputPart;
class DeepScanLineOutputPart;
class DeepTiledOutputPart;


// internal use only
struct InputPartData;
struct OutputStreamMutex;
struct OutputPartData;
struct InputStreamMutex;

// frame buffers

class  FrameBuffer;
class  DeepFrameBuffer;
struct DeepSlice;

// compositing
class DeepCompositing;
class CompositeDeepScanLine;

// preview image
class PreviewImage;
struct PreviewRgba;

// streams
class OStream;
class IStream;

class IDManifest;
class CompressedIDManifest;


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif // include guard
