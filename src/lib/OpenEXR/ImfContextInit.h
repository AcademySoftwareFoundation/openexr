//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_CONTEXT_INIT_H
#define INCLUDED_IMF_CONTEXT_INIT_H

#include "ImfForward.h"

#include "openexr.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

/// @brief ContextInitializer provides a basic type
/// to initialize a Context with.
///
/// A context fundamentally represents an EXR file of some sort
/// (reading a file, reading a stream, etc.)
class IMF_EXPORT_TYPE ContextInitializer
{
    enum class ContextFileType
    {
        READ,
        WRITE,
        READ_WRITE,
        TEMP
    };

public:
    ContextInitializer& setErrorHandler (exr_error_handler_cb_t errfn) noexcept
    {
        _initializer.error_handler_fn = errfn;
        return *this;
    }

    ContextInitializer& setAllocationFunctions (
        exr_memory_allocation_func_t allocfn,
        exr_memory_free_func_t       freefn) noexcept
    {
        _initializer.alloc_fn = allocfn;
        _initializer.free_fn  = freefn;
        return *this;
    }

    IMF_EXPORT
    ContextInitializer& setInputStream (IStream* istr);
    IMF_EXPORT
    ContextInitializer& setOutputStream (OStream* ostr);

    ContextInitializer& setCustomInputIO (
        void*                         user,
        exr_read_func_ptr_t           readfn,
        exr_query_size_func_ptr_t     sizefn,
        exr_destroy_stream_func_ptr_t destroyfn) noexcept
    {
        _initializer.user_data  = user;
        _initializer.read_fn    = readfn;
        _initializer.size_fn    = sizefn;
        _initializer.destroy_fn = destroyfn;
        _ctxt_type              = ContextFileType::READ;
        return *this;
    }

    ContextInitializer& setCustomOutputIO (
        void*                         user,
        exr_write_func_ptr_t          writefn,
        exr_destroy_stream_func_ptr_t destroyfn,
        exr_read_func_ptr_t           readfn = nullptr,
        exr_query_size_func_ptr_t     sizefn = nullptr) noexcept
    {
        _initializer.user_data  = user;
        _initializer.read_fn    = readfn;
        _initializer.size_fn    = sizefn;
        _initializer.write_fn   = writefn;
        _initializer.destroy_fn = destroyfn;
        _ctxt_type              = (readfn) ? ContextFileType::READ_WRITE
                                           : ContextFileType::WRITE;
        return *this;
    }

    ContextInitializer& setMaxImageSize (int w, int h) noexcept
    {
        _initializer.max_image_width  = w;
        _initializer.max_image_height = h;
        return *this;
    }

    ContextInitializer& setMaxTileSize (int w, int h) noexcept
    {
        _initializer.max_tile_width  = w;
        _initializer.max_tile_height = h;
        return *this;
    }

    ContextInitializer& setZipLevel (int zl) noexcept
    {
        _initializer.zip_level = zl;
        return *this;
    }

    ContextInitializer& setDWAQuality (float dq) noexcept
    {
        _initializer.dwa_quality = dq;
        return *this;
    }

    ContextInitializer& strictHeaderValidation (bool onoff) noexcept
    {
        setFlag (EXR_CONTEXT_FLAG_STRICT_HEADER, onoff);
        return *this;
    }

    ContextInitializer& silentHeaderParse (bool onoff) noexcept
    {
        setFlag (EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE, onoff);
        return *this;
    }

    ContextInitializer& disableChunkReconstruction (bool onoff) noexcept
    {
        setFlag (EXR_CONTEXT_FLAG_DISABLE_CHUNK_RECONSTRUCTION, onoff);
        return *this;
    }

    ContextInitializer& writeLegacyHeader (bool onoff) noexcept
    {
        setFlag (EXR_CONTEXT_FLAG_WRITE_LEGACY_HEADER, onoff);
        return *this;
    }

private:
    void setFlag (const int flag, bool onoff)
    {
        _initializer.flags =
            (_initializer.flags & ~(flag)) | (onoff ? flag : 0);
    }

    friend class Context;

    exr_context_initializer_t _initializer = EXR_DEFAULT_CONTEXT_INITIALIZER;
    ContextFileType           _ctxt_type   = ContextFileType::TEMP;
    IStream*                  _prov_stream = nullptr;
}; // class ContextInitializer

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif // INCLUDED_IMF_CONTEXT_INIT_H
