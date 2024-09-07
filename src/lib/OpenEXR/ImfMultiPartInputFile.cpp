//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfMultiPartInputFile.h"

#include "ImfBoxAttribute.h"
#include "ImfChromaticitiesAttribute.h"
#include "ImfDeepScanLineInputFile.h"
#include "ImfDeepTiledInputFile.h"
#include "ImfFloatAttribute.h"
#include "ImfInputFile.h"
#include "ImfInputPartData.h"
#include "ImfInputStreamMutex.h"
#include "ImfMisc.h"
#include "ImfPartType.h"
#include "ImfScanLineInputFile.h"
#include "ImfStdIO.h"
#include "ImfTileOffsets.h"
#include "ImfTiledInputFile.h"
#include "ImfTiledMisc.h"
#include "ImfTimeCodeAttribute.h"
#include "ImfVersion.h"
#include "ImfVersion.h"

// TODO: remove
#include "ImfInputStreamMutex.h"

#include <OpenEXRConfig.h>

#include <Iex.h>

#include <any>
#include <mutex>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

struct MultiPartInputFile::Data
{
#if ILMTHREAD_THREADING_ENABLED
    std::mutex _mx;
#endif
    struct Part
    {
        InputPartData data;
        std::any file;
    };
    std::vector<Part> parts;
};

////////////////////////////////////////

MultiPartInputFile::MultiPartInputFile (
        const char*               filename,
        const ContextInitializer& ctxtinit,
        int                       numThreads,
        bool                      autoAddType)
    : _ctxt (filename, ctxtinit, Context::read_mode_t{})
    , _data (std::make_shared<Data> ())
{
    int version = _ctxt.version ();
    int pc = _ctxt.partCount ();
    _data->parts.resize (pc);

    for ( int p = 0; p < pc; ++p )
    {
        _data->parts[p].data = InputPartData (_ctxt, p, numThreads);

        if (autoAddType && ! _data->parts[p].data.header.hasType ())
        {
            if (isTiled (version))
                _data->parts[p].data.header.setType (TILEDIMAGE);
            else
                _data->parts[p].data.header.setType (SCANLINEIMAGE);
        }
    }
}

MultiPartInputFile::MultiPartInputFile (
    const char fileName[], int numThreads, bool reconstructChunkOffsetTable)
    : MultiPartInputFile (
        fileName,
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false)
        .disableChunkReconstruction(!reconstructChunkOffsetTable),
        numThreads)
{
}

MultiPartInputFile::MultiPartInputFile (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is,
    int                                      numThreads,
    bool                                     reconstructChunkOffsetTable)
    : MultiPartInputFile (
        is.fileName (),
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false)
        .setInputStream (&is)
        .disableChunkReconstruction(!reconstructChunkOffsetTable),
        numThreads)
{
}

const char* MultiPartInputFile::fileName () const
{
    return _ctxt.fileName ();
}

int
MultiPartInputFile::parts () const
{
    return static_cast<int> (_data->parts.size ());
}

int
MultiPartInputFile::version () const
{
    return _ctxt.version ();
}

const Header&
MultiPartInputFile::header (int partNumber) const
{
    return getPart (partNumber)->header;
}

void
MultiPartInputFile::flushPartCache ()
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    for (auto &part: _data->parts)
        part.file.reset();
}

bool
MultiPartInputFile::partComplete (int partNumber) const
{
    return _ctxt.chunkTableValid (partNumber);
}

template <class T>
T*
MultiPartInputFile::getInputPart (int partNumber)
{
    if (partNumber < 0 || static_cast<size_t> (partNumber) >= _data->parts.size ())
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "MultiPartInputFile::getPart called with invalid part "
                << partNumber << " on file with " << _data->parts.size () << " parts");
    }

#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    using file_storage = std::shared_ptr<T>;
    file_storage f;
    if (!_data->parts[partNumber].file.has_value ())
    {
        // TODO: change to copy / value semantics
        // stupid make_shared and friend functions, can we remove this restriction?
        // f = std::make_shared<T> (&(_data->parts[partNumber].data));
        f.reset (new T (&(_data->parts[partNumber].data)));
        _data->parts[partNumber].file = f;
    }
    else
        f = std::any_cast<file_storage> (_data->parts[partNumber].file);

    // TODO: change to by reference / value semantics
    return f.get();
}

template InputFile*      MultiPartInputFile::getInputPart<InputFile> (int);
template TiledInputFile* MultiPartInputFile::getInputPart<TiledInputFile> (int);
template DeepScanLineInputFile*
MultiPartInputFile::getInputPart<DeepScanLineInputFile> (int);
template DeepTiledInputFile*
MultiPartInputFile::getInputPart<DeepTiledInputFile> (int);

InputPartData*
MultiPartInputFile::getPart (int n) const
{
    if (n < 0 || static_cast<size_t> (n) >= _data->parts.size ())
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "MultiPartInputFile::getPart called with invalid part "
                << n << " on file with " << _data->parts.size () << " parts");
    }
    return &(_data->parts[n].data);
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
