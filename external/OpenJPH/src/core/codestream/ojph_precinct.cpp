//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_precinct.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_precinct.h"
#include "ojph_subband.h"
#include "ojph_codeblock.h" // for coded_cb_header
#include "ojph_bitbuffer_write.h"
#include "ojph_bitbuffer_read.h"


namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    struct tag_tree
    {
      void init(ui8* buf, ui32 *lev_idx, ui32 num_levels, size s, int init_val)
      {
        for (ui32 i = 0; i <= num_levels; ++i) //on extra level
          levs[i] = buf + lev_idx[i];
        for (ui32 i = num_levels + 1; i < 16; ++i)
          levs[i] = (ui8*)INT_MAX; //make it crash on error
        width = s.w;
        height = s.h;
        for (ui32 i = 0; i < num_levels; ++i)
        {
          ui32 size = 1u << ((num_levels - 1 - i) << 1);
          memset(levs[i], init_val, size);
        }
        *levs[num_levels] = 0;
        this->num_levels = num_levels;
      }

      ui8* get(ui32 x, ui32 y, ui32 lev)
      {
        return levs[lev] + (x + y * ((width + (1 << lev) - 1) >> lev));
      }

      ui32 width, height, num_levels;
      ui8* levs[16]; // you cannot have this high number of levels
    };

    //////////////////////////////////////////////////////////////////////////
    static inline ui32 log2ceil(ui32 x)
    {
      ui32 t = 31 - count_leading_zeros(x);
      return t + (x & (x - 1) ? 1 : 0);
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 precinct::prepare_precinct(int tag_tree_size, ui32* lev_idx,
                                    mem_elastic_allocator* elastic)
    {
      bit_write_buf bb;
      coded_lists *cur_coded_list = NULL;
      ui32 cb_bytes = 0; //cb_bytes;
      ui32 ph_bytes = 0; //precinct header size
      int num_skipped_subbands = 0;
      for (int s = 0; s < 4; ++s)
      {
        if (bands[s].empty)
          continue;

        if (cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
          continue;

        ui32 num_levels = 1 +
          ojph_max(log2ceil(cb_idxs[s].siz.w), log2ceil(cb_idxs[s].siz.h));

        //create quad trees for inclusion and missing msbs
        tag_tree inc_tag, inc_tag_flags, mmsb_tag, mmsb_tag_flags;
        inc_tag.init(scratch, lev_idx, num_levels, cb_idxs[s].siz, 255);
        inc_tag_flags.init(scratch + tag_tree_size,
          lev_idx, num_levels, cb_idxs[s].siz, 0);
        mmsb_tag.init(scratch + (tag_tree_size<<1),
          lev_idx, num_levels, cb_idxs[s].siz, 255);
        mmsb_tag_flags.init(scratch + (tag_tree_size<<1) + tag_tree_size,
          lev_idx, num_levels, cb_idxs[s].siz, 0);
        ui32 band_width = bands[s].num_blocks.w;
        coded_cb_header *cp = bands[s].coded_cbs;
        cp += cb_idxs[s].org.x + cb_idxs[s].org.y * band_width;
        for (ui32 y = 0; y < cb_idxs[s].siz.h; ++y)
        {
          for (ui32 x = 0; x < cb_idxs[s].siz.w; ++x)
          {
            coded_cb_header *p = cp + x;
            *inc_tag.get(x, y, 0) = (p->next_coded == NULL); //1 if true
            *mmsb_tag.get(x, y, 0) = (ui8)p->missing_msbs;
          }
          cp += band_width;
        }
        for (ui32 lev = 1; lev < num_levels; ++lev)
        {
          ui32 height = (cb_idxs[s].siz.h + (1<<lev) - 1) >> lev;
          ui32 width = (cb_idxs[s].siz.w + (1<<lev) - 1) >> lev;
          for (ui32 y = 0; y < height; ++y)
          {
            for (ui32 x = 0; x < width; ++x)
            {
              ui8 t1, t2;
              t1 = ojph_min(*inc_tag.get(x<<1, y<<1, lev-1),
                            *inc_tag.get((x<<1) + 1, y<<1, lev-1));
              t2 = ojph_min(*inc_tag.get(x<<1, (y<<1) + 1, lev-1),
                            *inc_tag.get((x<<1) + 1, (y<<1) + 1, lev-1));
              *inc_tag.get(x, y, lev) = ojph_min(t1, t2);
              *inc_tag_flags.get(x, y, lev) = 0;
              t1 = ojph_min(*mmsb_tag.get(x<<1, y<<1, lev-1),
                            *mmsb_tag.get((x<<1) + 1, y<<1, lev-1));
              t2 = ojph_min(*mmsb_tag.get(x<<1, (y<<1) + 1, lev-1),
                            *mmsb_tag.get((x<<1) + 1, (y<<1) + 1, lev-1));
              *mmsb_tag.get(x, y, lev) = ojph_min(t1, t2);
              *mmsb_tag_flags.get(x, y, lev) = 0;
            }
          }
        }
        *inc_tag.get(0,0,num_levels) = 0;
        *inc_tag_flags.get(0,0,num_levels) = 0;
        *mmsb_tag.get(0,0,num_levels) = 0;
        *mmsb_tag_flags.get(0,0,num_levels) = 0;
        if (*inc_tag.get(0, 0, num_levels-1) != 0) //empty subband
        {
          if (coded) //non empty precinct, tag tree top is 0
            bb_put_bits(&bb, 0, 1, elastic, cur_coded_list, ph_bytes);
          else
            ++num_skipped_subbands;
          continue;
        }
        //now we are in a position to code
        if (coded == NULL)
        {
          bb_init(&bb, elastic, cur_coded_list);
          coded = cur_coded_list;
          //store non empty packet
          bb_put_bit(&bb, 1, elastic, cur_coded_list, ph_bytes);

          // if the first one or two subbands are empty (has codeblocks but
          // no data in them), we need to code them here.
          bb_put_bits(&bb, 0, num_skipped_subbands, elastic, cur_coded_list,
                      ph_bytes);
          num_skipped_subbands = 0; //this line is not needed
        }

        ui32 width = cb_idxs[s].siz.w;
        ui32 height = cb_idxs[s].siz.h;
        for (ui32 y = 0; y < height; ++y)
        {
          cp = bands[s].coded_cbs;
          cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
          for (ui32 x = 0; x < width; ++x, ++cp)
          {
            //inclusion bits
            for (ui32 cur_lev = num_levels; cur_lev > 0; --cur_lev)
            {
              ui32 levm1 = cur_lev - 1;
              //check sent
              if (*inc_tag_flags.get(x>>levm1, y>>levm1, levm1) == 0)
              {
                ui32 skipped = *inc_tag.get(x>>levm1, y>>levm1, levm1);
                skipped -= *inc_tag.get(x>>cur_lev, y>>cur_lev, cur_lev);
                assert(skipped <= 1); // for HTJ2K, this should 0 or 1
                bb_put_bits(&bb, 1 - skipped, 1,
                  elastic, cur_coded_list, ph_bytes);
                *inc_tag_flags.get(x>>levm1, y>>levm1, levm1) = 1;
              }
              if (*inc_tag.get(x>>levm1, y>>levm1, levm1) > 0)
                break;
            }

            if (cp->num_passes == 0) //empty codeblock
              continue;

            //missing msbs
            for (ui32 cur_lev = num_levels; cur_lev > 0; --cur_lev)
            {
              ui32 levm1 = cur_lev - 1;
              //check sent
              if (*mmsb_tag_flags.get(x>>levm1, y>>levm1, levm1) == 0)
              {
                int num_zeros = *mmsb_tag.get(x>>levm1, y>>levm1, levm1);
                num_zeros -= *mmsb_tag.get(x>>cur_lev, y>>cur_lev, cur_lev);
                bb_put_zeros(&bb, num_zeros,
                  elastic, cur_coded_list, ph_bytes);
                bb_put_bits(&bb, 1, 1,
                  elastic, cur_coded_list, ph_bytes);
                *mmsb_tag_flags.get(x>>levm1, y>>levm1, levm1) = 1;
              }
            }

            //number of coding passes
            switch (cp->num_passes)
            {
              case 3:
                bb_put_bits(&bb, 12, 4, elastic, cur_coded_list, ph_bytes);
                break;
              case 2:
                bb_put_bits(&bb, 2, 2, elastic, cur_coded_list, ph_bytes);
                break;
              case 1:
                bb_put_bits(&bb, 0, 1, elastic, cur_coded_list, ph_bytes);
                break;
              default:
                assert(0);
            }

            //pass lengths
            //either one, two, or three passes, but only one or two lengths
            int bits1 = 32 - (int)count_leading_zeros(cp->pass_length[0]);
            int extra_bit = cp->num_passes > 2 ? 1 : 0; //for 2nd length
            int bits2 = 0;
            if (cp->num_passes > 1)
              bits2 = 32 - (int)count_leading_zeros(cp->pass_length[1]);
            int bits = ojph_max(bits1, bits2 - extra_bit) - 3;
            bits = ojph_max(bits, 0);
            bb_put_bits(&bb, 0xFFFFFFFEu, bits+1,
              elastic, cur_coded_list, ph_bytes);

            bb_put_bits(&bb, cp->pass_length[0], bits+3,
              elastic, cur_coded_list, ph_bytes);
            if (cp->num_passes > 1)
              bb_put_bits(&bb, cp->pass_length[1], bits+3+extra_bit,
                elastic, cur_coded_list, ph_bytes);

            cb_bytes += cp->pass_length[0] + cp->pass_length[1];
          }
        }
      }

      if (coded)
      {
        bb_terminate(&bb);
        ph_bytes += cur_coded_list->buf_size - cur_coded_list->avail_size;
      }

      return coded ? cb_bytes + ph_bytes : 1; // 1 for empty packet
    }

    //////////////////////////////////////////////////////////////////////////
    void precinct::write(outfile_base *file)
    {
      if (coded)
      {
        //write packet header
        coded_lists *ccl = coded;
        while (ccl)
        {
          file->write(ccl->buf, ccl->buf_size - ccl->avail_size);
          ccl = ccl->next_list;
        }

        //write codeblocks
        for (int s = 0; s < 4; ++s)
        {
          if (bands[s].empty)
            continue;

          ui32 band_width = bands[s].num_blocks.w;
          ui32 width = cb_idxs[s].siz.w;
          ui32 height = cb_idxs[s].siz.h;
          for (ui32 y = 0; y < height; ++y)
          {
            coded_cb_header *cp = bands[s].coded_cbs;
            cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
            for (ui32 x = 0; x < width; ++x, ++cp)
            {
              coded_lists *ccl = cp->next_coded;
              while (ccl)
              {
                file->write(ccl->buf, ccl->buf_size - ccl->avail_size);
                ccl = ccl->next_list;
              }
            }
          }
        }
      }
      else
      {
        //empty packet
        char buf = 0x00;
        file->write(&buf, 1);
      }
    }


    //////////////////////////////////////////////////////////////////////////
    void precinct::parse(int tag_tree_size, ui32* lev_idx,
                         mem_elastic_allocator *elastic,
                         ui32 &data_left, infile_base *file,
                         bool skipped)
    {
      assert(data_left > 0);
      bit_read_buf bb;
      bb_init(&bb, data_left, file);
      if (may_use_sop)
        bb_skip_sop(&bb);

      bool empty_packet = true;
      for (int s = 0; s < 4; ++s)
      {
        if (bands[s].empty)
          continue;

        if (cb_idxs[s].siz.w == 0 || cb_idxs[s].siz.h == 0)
          continue;

        if (empty_packet) //one bit to check if the packet is empty
        {
          ui32 bit;
          bb_read_bit(&bb, bit);
          if (bit == 0) //empty packet
          { bb_terminate(&bb, uses_eph); data_left = bb.bytes_left; return; }
          empty_packet = false;
        }

        ui32 num_levels = 1 +
          ojph_max(log2ceil(cb_idxs[s].siz.w), log2ceil(cb_idxs[s].siz.h));

        //create quad trees for inclusion and missing msbs
        tag_tree inc_tag, inc_tag_flags, mmsb_tag, mmsb_tag_flags;
        inc_tag.init(scratch, lev_idx, num_levels, cb_idxs[s].siz, 0);
        *inc_tag.get(0, 0, num_levels) = 0;
        inc_tag_flags.init(scratch + tag_tree_size, lev_idx, num_levels,
          cb_idxs[s].siz, 0);
        *inc_tag_flags.get(0, 0, num_levels) = 0;
        mmsb_tag.init(scratch + (tag_tree_size<<1), lev_idx, num_levels,
          cb_idxs[s].siz, 0);
        *mmsb_tag.get(0, 0, num_levels) = 0;
        mmsb_tag_flags.init(scratch + (tag_tree_size<<1) + tag_tree_size,
          lev_idx, num_levels, cb_idxs[s].siz, 0);
        *mmsb_tag_flags.get(0, 0, num_levels) = 0;

        //
        ui32 band_width = bands[s].num_blocks.w;
        ui32 width = cb_idxs[s].siz.w;
        ui32 height = cb_idxs[s].siz.h;
        for (ui32 y = 0; y < height; ++y)
        {
          coded_cb_header *cp = bands[s].coded_cbs;
          cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
          for (ui32 x = 0; x < width; ++x, ++cp)
          {
            //process inclusion
            bool empty_cb = false;
            for (ui32 cl = num_levels; cl > 0; --cl)
            {
              ui32 cur_lev = cl - 1;
              empty_cb = *inc_tag.get(x>>cur_lev, y>>cur_lev, cur_lev) == 1;
              if (empty_cb)
                break;
              //check received
              if (*inc_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) == 0)
              {
                ui32 bit;
                if (bb_read_bit(&bb, bit) == false)
                { data_left = 0; throw "error reading from file p1"; }
                empty_cb = (bit == 0);
                *inc_tag.get(x>>cur_lev, y>>cur_lev, cur_lev) = (ui8)(1 - bit);
                *inc_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) = 1;
              }
              if (empty_cb)
                break;
            }

            if (empty_cb)
              continue;

            //process missing msbs
            ui32 mmsbs = 0;
            for (ui32 levp1 = num_levels; levp1 > 0; --levp1)
            {
              ui32 cur_lev = levp1 - 1;
              mmsbs = *mmsb_tag.get(x>>levp1, y>>levp1, levp1);
              //check received
              if (*mmsb_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) == 0)
              {
                ui32 bit = 0;
                while (bit == 0)
                {
                  if (bb_read_bit(&bb, bit) == false)
                  { data_left = 0; throw "error reading from file p2"; }
                  mmsbs += 1 - bit;
                }
                *mmsb_tag.get(x>>cur_lev, y>>cur_lev, cur_lev) = (ui8)mmsbs;
                *mmsb_tag_flags.get(x>>cur_lev, y>>cur_lev, cur_lev) = 1;
              }
            }

            if (mmsbs > cp->Kmax)
              throw "error in parsing a tile header; "
              "missing msbs are larger or equal to Kmax. The most likely "
              "cause is a corruption in the bitstream.";
            cp->missing_msbs = mmsbs;

            //get number of passes
            ui32 bit, num_passes = 1;
            if (bb_read_bit(&bb, bit) == false)
            { data_left = 0; throw "error reading from file p3"; }
            if (bit)
            {
              num_passes = 2;
              if (bb_read_bit(&bb, bit) == false)
              { data_left = 0; throw "error reading from file p4"; }
              if (bit)
              {
                if (bb_read_bits(&bb, 2, bit) == false)
                { data_left = 0; throw "error reading from file p5";  }
                num_passes = 3 + bit;
                if (bit == 3)
                {
                  if (bb_read_bits(&bb, 5, bit) == false)
                  { data_left = 0; throw "error reading from file p6"; }
                  num_passes = 6 + bit;
                  if (bit == 31)
                  {
                    if (bb_read_bits(&bb, 7, bit) == false)
                    { data_left = 0; throw "error reading from file p7"; }
                    num_passes = 37 + bit;
                  }
                }
              }
            }
            cp->num_passes = num_passes;

            // Parse pass lengths
            // When number of passes is one, one length.
            // When number of passes is two or three, two lengths.
            // When number of passes > 3, we have place holder passes;
            // In this case, subtract multiples of 3 from the number of
            // passes; for example, if we have 10 passes, we subtract 9,
            // producing 1 pass.

            // 1 => 1, 2 => 2, 3 => 3, 4 => 1, 5 => 2, 6 => 3
            ui32 num_phld_passes = (num_passes - 1) / 3;
            cp->missing_msbs += num_phld_passes;

            num_phld_passes *= 3;
            cp->num_passes = num_passes - num_phld_passes;
            cp->pass_length[0] = cp->pass_length[1] = 0;

            int Lblock = 3;
            bit = 1;
            while (bit)
            {
              // add any extra bits here
              if (bb_read_bit(&bb, bit) == false)
              { data_left = 0; throw "error reading from file p8"; }
              Lblock += bit;
            }

            int bits = Lblock + 31 -
              (int)count_leading_zeros(num_phld_passes + 1);
            if (bb_read_bits(&bb, bits, bit) == false)
            { data_left = 0; throw "error reading from file p9"; }
            if (bit < 2)
              throw "The cleanup segment of an HT codeblock cannot contain "
                "less than 2 bytes";
            if (bit >= 65535)
              throw "The cleanup segment of an HT codeblock must contain "
                "less than 65535 bytes";
            cp->pass_length[0] = bit;

            if (cp->num_passes > 1)
            {
              //bits = Lblock + 31 - count_leading_zeros(cp->num_passes - 1);
              // The following is simpler than the above, I think?
              bits = Lblock + (cp->num_passes > 2 ? 1 : 0);
              if (bb_read_bits(&bb, bits, bit) == false)
              { data_left = 0; throw "error reading from file p10"; }
              if (bit >= 2047)
                throw "The refinement segment (SigProp and MagRep passes) of "
                  "an HT codeblock must contain less than 2047 bytes";
              cp->pass_length[1] = bit;
            }
          }
        }
      }
      if (empty_packet)
      { // all subbands are empty
        ui32 bit = 0;
        bb_read_bit(&bb, bit);
        //assert(bit == 0);
      }
      bb_terminate(&bb, uses_eph);
      //read codeblock data
      for (int s = 0; s < 4; ++s)
      {
        if (bands[s].empty)
          continue;

        ui32 band_width = bands[s].num_blocks.w;
        ui32 width = cb_idxs[s].siz.w;
        ui32 height = cb_idxs[s].siz.h;
        for (ui32 y = 0; y < height; ++y)
        {
          coded_cb_header *cp = bands[s].coded_cbs;
          cp += cb_idxs[s].org.x + (y + cb_idxs[s].org.y) * band_width;
          for (ui32 x = 0; x < width; ++x, ++cp)
          {
            ui32 num_bytes = cp->pass_length[0] + cp->pass_length[1];
            if (data_left)
            {
              if (num_bytes)
              {
                if (skipped)
                { //no need to read
                  si64 cur_loc = file->tell();
                  ui32 t = ojph_min(num_bytes, bb.bytes_left);
                  file->seek(t, infile_base::OJPH_SEEK_CUR);
                  ui32 bytes_read = (ui32)(file->tell() - cur_loc);
                  cp->pass_length[0] = cp->pass_length[1] = 0;
                  bb.bytes_left -= bytes_read;
                  assert(bytes_read == t || bb.bytes_left == 0);
                }
                else
                {
                  if (!bb_read_chunk(&bb, num_bytes, cp->next_coded, elastic))
                  {
                    //no need to decode a broken codeblock
                    cp->pass_length[0] = cp->pass_length[1] = 0;
                    data_left = 0;
                  }
                }
              }
            }
            else
              cp->pass_length[0] = cp->pass_length[1] = 0;
          }
        }
      }
      data_left = bb.bytes_left;
    }

  }
}