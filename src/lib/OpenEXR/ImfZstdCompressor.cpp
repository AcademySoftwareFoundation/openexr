#include <cstring>
#include "ImfZstdCompressor.h"

#include "blosc2.h"
#include "IlmThreadPool.h"
#include "ImfChannelList.h"
#include "ImfMisc.h"
namespace
{
class BloscInit
{
public:
    static void Init () { getInstance (); }
    BloscInit (const BloscInit&)            = delete;
    BloscInit& operator= (const BloscInit&) = delete;

private:
    BloscInit () { blosc2_init (); }
    ~BloscInit () { blosc2_destroy (); }
    static BloscInit& getInstance ()
    {
        static BloscInit instance;
        return instance;
    }
};
} // namespace

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER
ZstdCompressor::ZstdCompressor (
    const Header& hdr, size_t maxScanlineSize, size_t numScanLines)
    : Compressor (hdr)
    , _maxScanlineSize (maxScanlineSize)
    , _numScanLines (numScanLines)
    , _outBuffer (nullptr, &free)
    , _schunk (nullptr, &blosc2_schunk_free)
{}

int
ZstdCompressor::numScanLines () const
{
    return _numScanLines; // Needs to be in sync with ImfCompressor::numLinesInBuffer
}

int
ZstdCompressor::compress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    int typeSize = std::numeric_limits<int>::min ();
    for (auto it = header ().channels ().begin ();
         it != header ().channels ().end ();
         ++it)
    {
        // BLOSC prefilter is affected by the typesize. Initializing to max will ensure that a channel is not split in 2 and filtered separately.
        // probably compression can be improved for non-deep images by compressing every channel separately with the correct typeSize
        // (much harder to do for Deep-Data).
        typeSize = std::max (typeSize, Imf::pixelTypeSize (it.channel ().type));
    }

    auto ret   = BLOSC_compress_impl (inPtr, inSize, typeSize, outPtr);
    auto data  = malloc (Xdr::size<int> () + ret);
    auto write = (char*) data;

    Xdr::write<CharPtrIO> (write, Versions::LATEST);

    memcpy (write, outPtr, ret);
    outPtr = (char*) data;

    _outBuffer = raw_ptr ((char*) data, &free);

    return ret;
}

int
ZstdCompressor::uncompress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    auto read = (const char*) inPtr;
    int  v;
    Xdr::read<CharPtrIO> (read, v);
    if (v == Versions::SINGLE_BLOB)
    {
        return BLOSC_uncompress_impl_single_blob (
            read, inSize - Xdr::size<int> (), outPtr);
    }
    else
    {
        throw Iex::InputExc ("Unsupported ZstdCompressor version");
    }
}

int
ZstdCompressor::BLOSC_compress_impl (
    const char* inPtr, int inSize, int typeSize, const char*& out)
{
    BloscInit::Init ();
    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;

    cparams.typesize = typeSize;
    // clevel 9 is about a 20% increase in compression compared to 5.
    // Decompression speed is unchanged.
    cparams.clevel   = header ().zstdCompressionLevel ();
    cparams.nthreads = 1;
    cparams.compcode = BLOSC_ZSTD; // Codec
    cparams.splitmode =
        BLOSC_NEVER_SPLIT; // Split => multithreading, not split better compression

    blosc2_storage storage = BLOSC2_STORAGE_DEFAULTS;
    storage.cparams        = &cparams;
    storage.contiguous     = true;

    _schunk = schunk_ptr (blosc2_schunk_new (&storage), &blosc2_schunk_free);

    auto in = const_cast<char*> (inPtr);
    blosc2_schunk_append_buffer (_schunk.get (), in, inSize);

    uint8_t* buffer;
    bool     shouldFree = true;
    auto size = blosc2_schunk_to_buffer (_schunk.get (), &buffer, &shouldFree);
    out       = (char*) buffer;
    if (shouldFree) { _outBuffer = raw_ptr ((char*) buffer, &free); }
    return size;
}

int
ZstdCompressor::BLOSC_uncompress_impl_single_blob (
    const char* inPtr, int inSize, const char*& out)
{
    auto in = const_cast<char*> (inPtr);
    _schunk = schunk_ptr (
        blosc2_schunk_from_buffer (
            reinterpret_cast<uint8_t*> (in), inSize, true),
        &blosc2_schunk_free);

    auto buffSize = _maxScanlineSize * numScanLines ();
    _outBuffer =
        Imf::ZstdCompressor::raw_ptr ((char*) malloc (buffSize), &free);
    auto size = blosc2_schunk_decompress_chunk (
        _schunk.get (), 0, _outBuffer.get (), buffSize);
    out = _outBuffer.get ();
    return size;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT