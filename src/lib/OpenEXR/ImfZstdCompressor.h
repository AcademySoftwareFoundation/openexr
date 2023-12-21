#pragma once
#include <memory>
#include "ImfNamespace.h"
#include "ImfCompressor.h"
#include "ImfHeader.h"
#include "blosc2.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER
class ZstdCompressor : public Compressor
{
public:
    ZstdCompressor (
        const Header& hdr, size_t maxScanLines, size_t numScanLines);

private:
    using schunk_ptr =
        std::unique_ptr<blosc2_schunk, decltype (&blosc2_schunk_free)>;
    using raw_ptr = std::unique_ptr<char, decltype (&free)>;
    raw_ptr    _outBuffer;
    schunk_ptr _schunk;
    size_t     _maxScanlineSize;
    size_t     _numScanLines;
    int        numScanLines () const override; // max
    int        compress (
               const char* inPtr, int inSize, int minY, const char*& outPtr) override;
    int uncompress (
        const char* inPtr, int inSize, int minY, const char*& outPtr) override;
    int BLOSC_compress_impl (
        const char* inPtr, int inSize, int typeSize, const char*& out);
    int BLOSC_uncompress_impl_single_blob (
        const char* inPtr, int inSize, const char*& out);
    enum Versions : int
    {
        SINGLE_BLOB = 1,
        LATEST      = SINGLE_BLOB
    };
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT