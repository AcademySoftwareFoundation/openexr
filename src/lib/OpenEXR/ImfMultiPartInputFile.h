//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFMULTIPARTINPUTFILE_H_
#define IMFMULTIPARTINPUTFILE_H_

#include "ImfForward.h"

#include "ImfThreading.h"

#include "ImfContext.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

/// \brief
///
/// TODO: Document this
class IMF_EXPORT_TYPE MultiPartInputFile
{
public:
    IMF_EXPORT
    MultiPartInputFile (
        const char fileName[],
        int        numThreads                  = globalThreadCount (),
        bool       reconstructChunkOffsetTable = true);

    IMF_EXPORT
    MultiPartInputFile (
        IStream& is,
        int      numThreads                  = globalThreadCount (),
        bool     reconstructChunkOffsetTable = true);

    //-----------------------------------------------------------
    // A constructor that opens the file with the specified name
    // and context initialization routines
    // Destroying the InputFile object will close the file.
    //-----------------------------------------------------------
    IMF_EXPORT
    MultiPartInputFile (
        const char*               filename,
        const ContextInitializer& ctxtinit,
        int                       numThreads  = globalThreadCount (),
        bool                      autoAddType = true);

    //------------------------
    // Access to the file name
    //------------------------

    IMF_EXPORT
    const char* fileName () const;

    //----------------------------------
    // Access to the file format version
    //----------------------------------

    IMF_EXPORT
    int version () const;

    // ----------------------
    // Count of number of parts in file
    // ---------------------
    IMF_EXPORT
    int parts () const;

    //----------------------
    // Access to the headers
    //----------------------

    IMF_EXPORT
    const Header& header (int partNumber) const;

    // =----------------------------------------
    // Check whether the entire chunk offset
    // table for the part is written correctly
    // -----------------------------------------
    IMF_EXPORT
    bool partComplete (int partNumber) const;

    // ----------------------------------------
    // Flush internal part cache
    // Invalidates all 'Part' types previously
    // constructed from this file
    // Intended for test purposes, but can be
    // used to temporarily reduce memory overhead,
    // or to switch between types (e.g. TiledInputPart
    // or DeepScanLineInputPart to InputPart)
    // ----------------------------------------

    IMF_EXPORT
    void              flushPartCache ();

private:
    Context _ctxt;
    struct Data;
    std::shared_ptr<Data> _data;

    //
    // used internally by 'Part' types to access individual parts of the multipart file
    //
    // TODO: change these to value / reference semantics (smart ptr)
    template <class T> IMF_HIDDEN T* getInputPart (int partNumber);
    IMF_HIDDEN InputPartData*  getPart (int) const;

    IMF_HIDDEN void initialize ();

    friend class InputPart;
    friend class ScanLineInputPart;
    friend class TiledInputPart;
    friend class DeepScanLineInputPart;
    friend class DeepTiledInputPart;

    //
    // For backward compatibility.
    //

    friend class InputFile;
    friend class TiledInputFile;
    friend class ScanLineInputFile;
    friend class DeepScanLineInputFile;
    friend class DeepTiledInputFile;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif /* IMFMULTIPARTINPUTFILE_H_ */
