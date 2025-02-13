//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfContextInit.h"
#include "ImfIO.h"

#include <cinttypes>
#include <climits>
#include <cstring>
#include <mutex>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

struct istream_holder
{
    istream_holder (IStream* s) : _stream (s)
    {
    }

#if ILMTHREAD_THREADING_ENABLED
    std::mutex _mx;
#endif
    IStream* _stream;
};

static int64_t
istream_size (
    exr_const_context_t         ctxt,
    void*                       userdata)
{
    istream_holder* ih = static_cast<istream_holder*> (userdata);
    IStream*        s  = ih->_stream;

    return s->size ();
}

static int64_t
istream_threadsafe_read (
    exr_const_context_t         ctxt,
    void*                       userdata,
    void*                       buffer,
    uint64_t                    sz,
    uint64_t                    offset,
    exr_stream_error_func_ptr_t error_cb)
{
    istream_holder* ih    = static_cast<istream_holder*> (userdata);
    IStream*        s     = ih->_stream;
    int64_t         nread = -1;

    try
    {
        nread = s->read (buffer, sz, offset);
    }
    catch (std::exception &e)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Unable to seek to desired offset %" PRIu64 ": %s",
            offset,
            e.what());
        nread = -1;
    }
    catch (...)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Unable to seek to desired offset %" PRIu64 ": Unknown error",
            offset);
        nread = -1;
    }
    return nread;
}

static int64_t
istream_nonparallel_read (
    exr_const_context_t         ctxt,
    void*                       userdata,
    void*                       buffer,
    uint64_t                    sz,
    uint64_t                    offset,
    exr_stream_error_func_ptr_t error_cb)
{
    istream_holder* ih = static_cast<istream_holder*> (userdata);
    IStream*        s  = ih->_stream;

    if (sz > INT_MAX)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Stream interface request to read block too large");
        return -1;
    }

#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lk{ih->_mx};
#endif

    int64_t         nread = s->tellg ();
    try
    {
        if (offset != static_cast<size_t> (nread))
        {
            s->seekg (offset);
            nread = s->tellg ();
            if (offset != static_cast<size_t> (nread))
            {
                error_cb (
                    ctxt,
                    EXR_ERR_READ_IO,
                    "Unable to seek to desired offset %" PRIu64,
                    offset);
                return -1;
            }
        }

        int64_t stream_sz = s->size ();
        int64_t nend = nread + (int64_t)sz;
        if (stream_sz > 0 && nend > stream_sz)
        {
            sz = stream_sz - nend;
        }

        try
        {
            if (s->isMemoryMapped ())
            {
                char* data = s->readMemoryMapped (static_cast<int> (sz));
                // TODO: in a future release, pass this through to
                // core directly
                if (data)
                    memcpy (buffer, data, sz);
            }
            else
            {
                s->read (static_cast<char*> (buffer), static_cast<int> (sz));
            }
        }
        catch (...)
        {
            // bah, there could be two reasons for this, one is a
            // legitimate error, the other is a read past the end of file
            // (i.e. the core library tries to read a 4k block when
            // parsing the header), let's let the core deal with that and
            // clear errors
            ih->_stream->clear ();
            //error_cb (
            //    ctxt,
            //    EXR_ERR_READ_IO,
            //    "Unable to read requested bytes: %" PRIu64,
            //    sz);
        }
        nread = s->tellg () - nread;
    }
    catch (std::exception &e)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Unable to seek to desired offset %" PRIu64 ": %s",
            offset,
            e.what());
        nread = -1;
    }
    catch (...)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Unable to seek to desired offset %" PRIu64 ": Unknown error",
            offset);
        nread = -1;
    }
    return nread;
}

static void
istream_destroy (exr_const_context_t ctxt, void* userdata, int failed)
{
    istream_holder* ih = static_cast<istream_holder*> (userdata);
    delete ih;
}

ContextInitializer&
ContextInitializer::setInputStream (IStream* istr)
{
    _initializer.user_data  = new istream_holder{istr};
    if (istr->isStatelessRead ())
        _initializer.read_fn = istream_threadsafe_read;
    else
        _initializer.read_fn = istream_nonparallel_read;
    _initializer.size_fn    = istream_size;
    _initializer.write_fn   = nullptr;
    _initializer.destroy_fn = istream_destroy;
    _ctxt_type              = ContextFileType::READ;
    _prov_stream            = istr;
    return *this;
}

struct ostream_holder
{
    ostream_holder (OStream* s) : _stream (s)
    {
        if (_stream) _cur_offset = _stream->tellp ();
    }

#if ILMTHREAD_THREADING_ENABLED
    std::mutex _mx;
#endif
    uint64_t _cur_offset = 0;
    OStream* _stream;
};

static int64_t
ostream_write (
    exr_const_context_t         ctxt,
    void*                       userdata,
    const void*                 buffer,
    uint64_t                    sz,
    uint64_t                    offset,
    exr_stream_error_func_ptr_t error_cb)
{
    ostream_holder* oh = static_cast<ostream_holder*> (userdata);

    if (sz > INT_MAX)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Stream interface request to write block too large");
        return -1;
    }

#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lk{oh->_mx};
#endif

    if (offset != oh->_cur_offset)
    {
        oh->_stream->seekp (offset);
        oh->_cur_offset = oh->_stream->tellp ();
        if (offset != oh->_cur_offset)
        {
            error_cb (
                ctxt,
                EXR_ERR_READ_IO,
                "Unable to seek to desired offset %" PRIu64,
                offset);
            return -1;
        }
    }

    int64_t nwrite = oh->_cur_offset;
    try
    {
        oh->_stream->write (
            static_cast<const char*> (buffer), static_cast<int> (sz));
        oh->_cur_offset = oh->_stream->tellp ();
        nwrite          = oh->_cur_offset - nwrite;
    }
    catch (...)
    {
        error_cb (
            ctxt,
            EXR_ERR_READ_IO,
            "Unable to seek to desired offset %" PRIu64,
            offset);
        nwrite = -1;
    }
    return nwrite;
}

static void
ostream_destroy (exr_const_context_t ctxt, void* userdata, int failed)
{
    ostream_holder* oh = static_cast<ostream_holder*> (userdata);

    delete oh;
}

ContextInitializer&
ContextInitializer::setOutputStream (OStream* ostr)
{
    _initializer.user_data  = new ostream_holder{ostr};
    _initializer.read_fn    = nullptr;
    _initializer.size_fn    = nullptr;
    _initializer.write_fn   = ostream_write;
    _initializer.destroy_fn = ostream_destroy;
    _ctxt_type              = ContextFileType::WRITE;
    return *this;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
