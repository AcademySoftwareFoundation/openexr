//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//
// The Imf::checkOpenEXRFile()-based fuzzers iterate
// parts and tiles in linear order with in-bounds coordinates. This fuzzer
// targets four paths that complementary code coverage misses:
//
//   Mode 0: TiledInputPart::readTile(dx, dy, lx, ly) with adversarial
//           coordinate sequencing, stressing tile-cache eviction and
//           ripmap level resolution.
//   Mode 1: TiledInputPart::readTiles(dx1, dx2, dy1, dy2, lx, ly) with
//           attacker-controlled ranges (rather than the natural
//           (0..numXTiles-1, 0..numYTiles-1) range).
//   Mode 2: DeepTiledInputPart::readTile(dx, dy, lx, ly) with
//           attacker-controlled coordinates.
//   Mode 3: MultiPartInputFile::header(p) iteration in reverse part
//           order, with per-attribute name lookup, exercising the
//           part-cache and the typed-attribute path.
//
// The harness writes the input to a temporary file because the public
// C++ MultiPartInputFile/TiledInputPart API operates on file paths.
//

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <unistd.h>

#include <ImfMultiPartInputFile.h>
#include <ImfTiledInputPart.h>
#include <ImfDeepTiledInputPart.h>
#include <ImfHeader.h>
#include <ImfFrameBuffer.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfPartType.h>

namespace IMF = OPENEXR_IMF_NAMESPACE;

namespace {

constexpr size_t kMaxInput          = 4 * 1024 * 1024;
constexpr int    kMaxTilesPerCall   = 16;
constexpr int    kMaxParts          = 8;
// Bound on (dx2-dx1+1)*(dy2-dy1+1) for readTiles() to avoid pathological
// 1024-tile calls slowing fuzz throughput. 8x8 = 64 tiles per call covers
// boundary and small-range cases without burning iteration time.
constexpr int    kMaxRangeSide      = 8;

std::string write_temp (const uint8_t* data, size_t size)
{
    char path[] = "/tmp/exr_gaps_XXXXXX";
    int  fd     = mkstemp (path);
    if (fd < 0) return std::string ();
    size_t total = 0;
    while (total < size)
    {
        ssize_t n = write (fd, data + total, size - total);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            close (fd);
            unlink (path);
            return std::string ();
        }
        if (n == 0) break;
        total += static_cast<size_t> (n);
    }
    close (fd);
    if (total != size)
    {
        unlink (path);
        return std::string ();
    }
    return std::string (path);
}

uint32_t read_u32 (const uint8_t* p, size_t size, size_t off)
{
    if (off + 4 > size) return 0;
    return static_cast<uint32_t> (p[off])
         | (static_cast<uint32_t> (p[off + 1]) << 8)
         | (static_cast<uint32_t> (p[off + 2]) << 16)
         | (static_cast<uint32_t> (p[off + 3]) << 24);
}

void
test_tiled_random_coords (
    const std::string& path, const uint8_t* params, size_t param_size)
{
    if (param_size < 8) return;
    try
    {
        IMF::MultiPartInputFile mpif (path.c_str ());
        int n_parts = std::min (mpif.parts (), kMaxParts);
        for (int p = 0; p < n_parts; p++)
        {
            const IMF::Header& hdr = mpif.header (p);
            if (!hdr.hasTileDescription ()) continue;
            // TiledInputPart is for non-deep tiled parts. Mode 2 covers
            // deep tiled. MultiPartInputFile auto-synthesizes a "type"
            // attribute, so we must check the value rather than presence.
            if (hdr.hasType () && hdr.type () == IMF::DEEPTILE) continue;
            try
            {
                IMF::TiledInputPart tip (mpif, p);
                IMF::FrameBuffer    fb;
                tip.setFrameBuffer (fb);
                for (int t = 0; t < kMaxTilesPerCall; t++)
                {
                    uint32_t r =
                        read_u32 (params, param_size, (t * 8) % param_size);
                    uint32_t s =
                        read_u32 (params, param_size, ((t * 8) + 4) % param_size);
                    int dx = r & 0xff;
                    int dy = (r >> 8) & 0xff;
                    int lx = s & 0x0f;
                    int ly = (s >> 4) & 0x0f;
                    try
                    {
                        tip.readTile (dx, dy, lx, ly);
                    }
                    catch (...)
                    {
                    }
                }
            }
            catch (...)
            {
            }
        }
    }
    catch (...)
    {
    }
}

void
test_tiled_range (
    const std::string& path, const uint8_t* params, size_t param_size)
{
    if (param_size < 8) return;
    try
    {
        IMF::MultiPartInputFile mpif (path.c_str ());
        int n_parts = std::min (mpif.parts (), kMaxParts);
        for (int p = 0; p < n_parts; p++)
        {
            const IMF::Header& hdr = mpif.header (p);
            if (!hdr.hasTileDescription ()) continue;
            try
            {
                IMF::TiledInputPart tip (mpif, p);
                IMF::FrameBuffer    fb;
                tip.setFrameBuffer (fb);
                uint32_t r   = read_u32 (params, param_size, 0);
                uint32_t s   = read_u32 (params, param_size, 4);
                int      dx1 = r & 0xff;
                int      dy1 = (r >> 16) & 0xff;
                // Clamp range size so total tile count stays <= kMaxRangeSide^2.
                int      dx2 = dx1 + (((r >> 8) & 0x1f) % kMaxRangeSide);
                int      dy2 = dy1 + (((r >> 24) & 0x1f) % kMaxRangeSide);
                int      lx  = s & 0x0f;
                int      ly  = (s >> 4) & 0x0f;
                try
                {
                    tip.readTiles (dx1, dx2, dy1, dy2, lx, ly);
                }
                catch (...)
                {
                }
            }
            catch (...)
            {
            }
        }
    }
    catch (...)
    {
    }
}

void
test_deep_tiled (
    const std::string& path, const uint8_t* params, size_t param_size)
{
    if (param_size < 4) return;
    try
    {
        IMF::MultiPartInputFile mpif (path.c_str ());
        int n_parts = std::min (mpif.parts (), kMaxParts);
        for (int p = 0; p < n_parts; p++)
        {
            const IMF::Header& hdr = mpif.header (p);
            // DeepTiledInputPart only accepts deep tiled parts.
            if (!hdr.hasType () || hdr.type () != IMF::DEEPTILE) continue;
            try
            {
                IMF::DeepTiledInputPart dtip (mpif, p);
                IMF::DeepFrameBuffer    fb;
                dtip.setFrameBuffer (fb);
                for (int t = 0; t < kMaxTilesPerCall; t++)
                {
                    uint32_t r =
                        read_u32 (params, param_size, (t * 4) % param_size);
                    int dx = r & 0xff;
                    int dy = (r >> 8) & 0xff;
                    int lx = (r >> 16) & 0x0f;
                    int ly = (r >> 20) & 0x0f;
                    try
                    {
                        dtip.readTile (dx, dy, lx, ly);
                    }
                    catch (...)
                    {
                    }
                }
            }
            catch (...)
            {
            }
        }
    }
    catch (...)
    {
    }
}

void
test_part_iteration (
    const std::string& path, const uint8_t* params, size_t param_size)
{
    (void) params;
    (void) param_size;
    try
    {
        IMF::MultiPartInputFile mpif (path.c_str ());
        int n_parts = std::min (mpif.parts (), kMaxParts);
        for (int p = n_parts - 1; p >= 0; p--)
        {
            const IMF::Header& hdr = mpif.header (p);
            for (auto it = hdr.begin (); it != hdr.end (); ++it)
            {
                (void) it.name ();
                try
                {
                    (void) hdr.findTypedAttribute<
                        IMF::TileDescriptionAttribute> (it.name ());
                }
                catch (...)
                {
                }
            }
        }
    }
    catch (...)
    {
    }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    if (size < 4 || size > kMaxInput) return 0;

    uint8_t mode      = data[0] % 4;
    size_t  param_len = std::min<size_t> (32, size - 1);
    const uint8_t* params = data + 1;
    if (size < 1 + param_len + 256) return 0;
    const uint8_t* exr_blob = data + 1 + param_len;
    size_t         exr_size = size - 1 - param_len;

    std::string path = write_temp (exr_blob, exr_size);
    if (path.empty ()) return 0;

    switch (mode)
    {
        case 0: test_tiled_random_coords (path, params, param_len); break;
        case 1: test_tiled_range (path, params, param_len); break;
        case 2: test_deep_tiled (path, params, param_len); break;
        case 3: test_part_iteration (path, params, param_len); break;
    }

    unlink (path.c_str ());
    return 0;
}
