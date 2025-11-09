/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <limits>
#include <string>
#include <fstream>

#include <openjph/ojph_arch.h>
#include <openjph/ojph_file.h>
#include <openjph/ojph_params.h>
#include <openjph/ojph_mem.h>
#include <openjph/ojph_codestream.h>
#include <openjph/ojph_message.h>

#include "openexr_decode.h"
#include "openexr_encode.h"
#include "internal_ht_common.h"

/**
 * OpenJPH output file that is backed by a fixed-size memory buffer
 */
class staticmem_outfile : public ojph::outfile_base
  {
  public:
    /**  A constructor */
    staticmem_outfile() {
        is_open = false;
        max_size = used_size = 0;
        buf = cur_ptr = NULL;
    }
    /**  A destructor */
    ~staticmem_outfile() override {
        is_open = false;
        max_size = used_size = 0;
        buf = cur_ptr = NULL;
    }

    /**  
     *  @brief Call this function to write a codestream to an existing memory buffer
	 *
     * 
     *  @param buf pointer to existing memory buffer.
     *  @param buf_size size of the existing memory buffer.
     */
    void open(void* buf, size_t buf_size) {
        assert(this->is_open == false);

        this->is_open = true;
        this->max_size = buf_size;
        this->used_size = 0;
        this->buf = (ojph::ui8*) buf;
        this->cur_ptr = this->buf;
    }

    /**  
     *  @brief Call this function to write data to the memory file.
     *
     *  This function adds new data to the memory file.  The memory buffer
     *  of the file grows as needed.
     *
     *  @param ptr is a pointer to new data.
     *  @param sz the number of bytes in the new data.
     */
    size_t write (const void* ptr, size_t sz) override
    {
        assert (this->is_open);
        assert (this->buf);
        assert (this->cur_ptr);

        size_t needed_size = (size_t) tell () + sz; //needed size
        if (needed_size > this->max_size) {
            throw std::range_error("Buffer size exceeded");
        }

        // copy bytes into buffer and adjust cur_ptr
        memcpy (this->cur_ptr, ptr, sz);
        cur_ptr += sz;
        used_size = ojph_max (used_size, (size_t) tell ());

        return sz;
    }
    /** 
     *  @brief Call this function to know the file size (i.e., number of 
     *         bytes used to store the file).
     *
     *  @return the file size.
     */
    ojph::si64 tell() override { return cur_ptr - buf; }

    /** 
     *  @brief Call this function to change write pointer location; the 
     *         function can expand file storage.
     *
     *  @return 0 on success, non-zero otherwise.
     */
    int seek (ojph::si64 offset, enum outfile_base::seek origin) override
    {
        if (origin == OJPH_SEEK_SET)
            ; // do nothing
        else if (origin == OJPH_SEEK_CUR)
            offset += tell ();
        else if (origin == OJPH_SEEK_END)
            offset += (ojph::si64) this->used_size;
        else
        {
            assert (0);
            return -1;
        }

        cur_ptr = buf + offset;
        return 0;
    }

    /** Call this function to close the file and deallocate memory
	   *
     *  The object can be used again after calling close
     */
    void close() override{
        is_open = false;
    }

    /** 
     *  @brief Call this function to access memory file data.
	   *
     *  It is not recommended to store the returned value because buffer
     *  storage address can change between write calls.
     *
     *  @return a constant pointer to the data.
     */
    const ojph::ui8* get_data() { return buf; }

    /** 
     *  @brief Call this function to access memory file data (for const 
     *         objects)
	   *
     *  This is similar to the above function, except that it can be used
     *  with constant objects.
     *
     *  @return a constant pointer to the data.
     */
    const ojph::ui8* get_data() const { return buf; }

    /** 
     *  Returns the size of the written data
     *
     *  @return size of the data stored in the file
     */
    size_t get_size() const { return this->used_size; }

  private:
    bool is_open;
    size_t max_size;
    size_t used_size;
    ojph::ui8 *buf;
    ojph::ui8 *cur_ptr;
  };

static exr_result_t
ht_undo_impl (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    std::vector<CodestreamChannelInfo> cs_to_file_ch (decode->channel_count);

    /* read the channel map */

    size_t header_sz;
    header_sz = read_header (
        (uint8_t*) compressed_data, comp_buf_size, cs_to_file_ch);
    if (static_cast<std::size_t>(decode->channel_count) != cs_to_file_ch.size ())
        return EXR_ERR_CORRUPT_CHUNK;

    for (int cs_i = 0; cs_i < decode->channel_count; cs_i++)
    {
        int file_i = cs_to_file_ch[cs_i].file_index;
        if (file_i >= decode->channel_count)
            return EXR_ERR_CORRUPT_CHUNK;

        size_t computedoffset = 0;
        for (int i = 0; i < file_i; ++i)
            computedoffset += decode->channels[i].width *
                              decode->channels[i].bytes_per_element;
        cs_to_file_ch[cs_i].raster_line_offset = computedoffset;
    }

    ojph::mem_infile infile;
    infile.open (
        reinterpret_cast<const ojph::ui8*> (compressed_data) + header_sz,
        comp_buf_size - header_sz);

    ojph::codestream cs;
    cs.read_headers (&infile);

    ojph::param_siz siz = cs.access_siz ();

    ojph::ui32 image_height =
        siz.get_image_extent ().y - siz.get_image_offset ().y;

    if (decode->chunk.width != siz.get_image_extent ().x - siz.get_image_offset ().x
        || decode->chunk.height != image_height
        || decode->channel_count != siz.get_num_components())
        return EXR_ERR_CORRUPT_CHUNK;

    int  bpl       = 0;
    bool is_planar = false;
    for (int16_t c = 0; c < decode->channel_count; c++)
    {
        bpl +=
            decode->channels[c].bytes_per_element * decode->channels[c].width;
        if (decode->channels[c].x_samples > 1 ||
            decode->channels[c].y_samples > 1)
        { is_planar = true; }
    }
    cs.set_planar (is_planar);

    cs.create ();

    assert (sizeof (uint16_t) == 2);
    assert (sizeof (uint32_t) == 4);
    ojph::ui32      next_comp = 0;
    ojph::line_buf* cur_line;
    if (cs.is_planar ())
    {
        for (int16_t c = 0; c < decode->channel_count; c++)
        {
            int file_c = cs_to_file_ch[c].file_index;
            assert (
                siz.get_recon_height (c) == decode->channels[file_c].height);
            assert (decode->channels[file_c].width == siz.get_recon_width (c));

            if (decode->channels[file_c].height == 0) continue;

            uint8_t* line_pixels = static_cast<uint8_t*> (uncompressed_data);

            for (int64_t y = decode->chunk.start_y;
                 y < image_height + decode->chunk.start_y;
                 y++)
            {
                for (int16_t line_c = 0; line_c < decode->channel_count;
                     line_c++)
                {
                    if (y % decode->channels[line_c].y_samples != 0) continue;

                    if (line_c == static_cast<ojph::ui32>(file_c))
                    {
                        cur_line = cs.pull (next_comp);
                        assert (next_comp == c);

                        if (decode->channels[file_c].data_type ==
                            EXR_PIXEL_HALF)
                        {
                            int16_t* channel_pixels = (int16_t*) line_pixels;
                            for (int16_t p = 0;
                                 p < decode->channels[file_c].width;
                                 p++)
                            {
                                *channel_pixels++ = cur_line->i32[p];
                            }
                        }
                        else
                        {
                            int32_t* channel_pixels = (int32_t*) line_pixels;
                            for (int16_t p = 0;
                                 p < decode->channels[file_c].width;
                                 p++)
                            {
                                *channel_pixels++ = cur_line->i32[p];
                            }
                        }
                    }

                    line_pixels += decode->channels[line_c].bytes_per_element *
                                   decode->channels[line_c].width;
                }
            }
        }
    }
    else
    {
        uint8_t* line_pixels = static_cast<uint8_t*> (uncompressed_data);

        assert (bpl * image_height == uncompressed_size);

        for (uint32_t y = 0; y < image_height; ++y)
        {
            for (int16_t c = 0; c < decode->channel_count; c++)
            {
                int file_c = cs_to_file_ch[c].file_index;
                cur_line   = cs.pull (next_comp);
                assert (next_comp == c);
                if (decode->channels[file_c].data_type == EXR_PIXEL_HALF)
                {
                    int16_t* channel_pixels =
                        (int16_t*) (line_pixels + cs_to_file_ch[c].raster_line_offset);
                    for (int16_t p = 0; p < decode->channels[file_c].width;
                         p++)
                    {
                        *channel_pixels++ = cur_line->i32[p];
                    }
                }
                else
                {
                    int32_t* channel_pixels =
                        (int32_t*) (line_pixels + cs_to_file_ch[c].raster_line_offset);
                    for (int16_t p = 0; p < decode->channels[file_c].width;
                         p++)
                    {
                        *channel_pixels++ = cur_line->i32[p];
                    }
                }
            }
            line_pixels += bpl;
        }
    }

    infile.close ();

    return rv;
}

extern "C" exr_result_t
internal_exr_undo_ht (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    try
    {
        return ht_undo_impl (decode, compressed_data, comp_buf_size,
                             uncompressed_data, uncompressed_size);
    }
    catch ( ... )
    {
    }

    return EXR_ERR_CORRUPT_CHUNK;
}


////////////////////////////////////////


static exr_result_t
ht_apply_impl (exr_encode_pipeline_t* encode)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    std::vector<CodestreamChannelInfo> cs_to_file_ch (encode->channel_count);
    bool                               isRGB = make_channel_map (
        encode->channel_count, encode->channels, cs_to_file_ch);

    int image_height = encode->chunk.height;
    int image_width  = encode->chunk.width;

    ojph::codestream cs;

    ojph::param_siz siz = cs.access_siz ();
    ojph::param_nlt nlt = cs.access_nlt ();

    bool isPlanar = false;
    siz.set_num_components (encode->channel_count);
    int bpl = 0;
    for (int16_t c = 0; c < encode->channel_count; c++)
    {
        int file_c = cs_to_file_ch[c].file_index;
        if (encode->channels[file_c].data_type != EXR_PIXEL_UINT)
            nlt.set_nonlinear_transform (
                c,
                ojph::param_nlt::nonlinearity::OJPH_NLT_BINARY_COMPLEMENT_NLT);
        siz.set_component (
            c,
            ojph::point (
                encode->channels[file_c].x_samples,
                encode->channels[file_c].y_samples),
            encode->channels[file_c].data_type == EXR_PIXEL_HALF ? 16 : 32,
            encode->channels[file_c].data_type != EXR_PIXEL_UINT);

        if (encode->channels[file_c].x_samples > 1 ||
            encode->channels[file_c].y_samples > 1)
        { isPlanar = true; }

        bpl += encode->channels[file_c].bytes_per_element *
               encode->channels[file_c].width;
    }

    cs.set_planar (isPlanar);

    siz.set_image_offset (ojph::point (0, 0));
    siz.set_image_extent (ojph::point (image_width, image_height));

    ojph::param_cod cod = cs.access_cod ();

    cod.set_color_transform (isRGB && !isPlanar);
    cod.set_reversible (true);
    cod.set_block_dims (128, 32);
    cod.set_num_decomposition (5);

    try
    {
        /* write the header */
        size_t header_sz = write_header (
            (uint8_t*) encode->compressed_buffer,
            encode->packed_bytes,
            cs_to_file_ch);

        /* write the codestream */
        staticmem_outfile output;
        output.open ( ((uint8_t*) encode->compressed_buffer) + header_sz, encode->packed_bytes - header_sz);

        cs.write_headers (&output);

        ojph::ui32      next_comp = 0;
        ojph::line_buf* cur_line  = cs.exchange (NULL, next_comp);

        if (cs.is_planar ())
        {
            for (int16_t c = 0; c < encode->channel_count; c++)
            {
                if (encode->channels[c].height == 0) continue;

                const uint8_t* line_pixels =
                    static_cast<const uint8_t*> (encode->packed_buffer);
                int16_t file_c = cs_to_file_ch[c].file_index;

                for (int64_t y = encode->chunk.start_y;
                    y < image_height + encode->chunk.start_y;
                    y++)
                {
                    for (int16_t line_c = 0; line_c < encode->channel_count;
                        line_c++)
                    {

                        if (y % encode->channels[line_c].y_samples != 0) continue;

                        if (line_c == file_c)
                        {
                            if (encode->channels[file_c].data_type ==
                                EXR_PIXEL_HALF)
                            {
                                int16_t* channel_pixels = (int16_t*) (line_pixels);
                                for (int32_t p = 0;
                                     p < encode->channels[file_c].width;
                                    p++)
                                {
                                    cur_line->i32[p] = *channel_pixels++;
                                }
                            }
                            else
                            {
                                int32_t* channel_pixels = (int32_t*) (line_pixels);
                                for (int32_t p = 0;
                                     p < encode->channels[file_c].width;
                                    p++)
                                {
                                    cur_line->i32[p] = *channel_pixels++;
                                }
                            }

                            assert (next_comp == c);
                            cur_line = cs.exchange (cur_line, next_comp);
                        }

                        line_pixels += encode->channels[line_c].bytes_per_element *
                                    encode->channels[line_c].width;
                    }
                }
            }
        }
        else
        {
            const uint8_t* line_pixels =
                static_cast<const uint8_t*> (encode->packed_buffer);

            assert (bpl * image_height == encode->packed_bytes);

            for (int y = 0; y < image_height; y++)
            {
                for (int16_t c = 0; c < encode->channel_count; c++)
                {
                    int file_c = cs_to_file_ch[c].file_index;

                    if (encode->channels[file_c].data_type == EXR_PIXEL_HALF)
                    {
                        int16_t* channel_pixels =
                            (int16_t*) (line_pixels + cs_to_file_ch[c].raster_line_offset);
                        for (int32_t p = 0; p < encode->channels[file_c].width;
                            p++)
                        {
                            cur_line->i32[p] = *channel_pixels++;
                        }
                    }
                    else
                    {
                        int32_t* channel_pixels =
                            (int32_t*) (line_pixels + cs_to_file_ch[c].raster_line_offset);
                        for (int32_t p = 0; p < encode->channels[file_c].width;
                            p++)
                        {
                            cur_line->i32[p] = *channel_pixels++;
                        }
                    }
                    assert (next_comp == c);
                    cur_line = cs.exchange (cur_line, next_comp);
                }
                line_pixels += bpl;
            }
        }

        cs.flush ();

        assert (output.get_size () >= 0);
        encode->compressed_bytes = output.get_size () + header_sz;
    } catch (const std::range_error& e) {
        encode->compressed_bytes = encode->packed_bytes;
    }

    return rv;
}

extern "C" exr_result_t
internal_exr_apply_ht (exr_encode_pipeline_t* encode)
{
    try
    {
        return ht_apply_impl (encode);
    }
    catch ( ... )
    {
    }

    return EXR_ERR_INCORRECT_CHUNK;
}
