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
// File: ojph_codestream.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_CODESTREAM_H
#define OJPH_CODESTREAM_H

#include <cstdlib>

#include "ojph_arch.h"
#include "ojph_defs.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //local prototyping
  namespace local {
    class codestream;
  };

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class param_siz;
  class param_cod;
  class param_qcd;
  class param_nlt;
  class comment_exchange;
  class mem_fixed_allocator;
  struct point;
  class line_buf;
  class outfile_base;
  class infile_base;

  ////////////////////////////////////////////////////////////////////////////
  /**
   *  @brief The object represent a codestream.
   *
   *  Most end-user use this object to create a j2c codestream.  The object
   *  currently can be used in one of two modes, reading or writing.
   *
   *  We try to follow the pImpl (pointer to Implementation) approach;
   *  therefore, objects in the ojph namespace hold pointers to internal
   *  implementations.  The actual implementation is usually in the
   *  ojph::local namespace. The actual implementation of the
   *  ojph::codestream object is in ojph_codestream.cpp, while the actual
   *  implementation can be found in ojph_codestream_local.h and
   *  ojph_codestream_local.cpp.
   *
   *  Most of these member functions provides nothing more than calls
   *  into the internal implementation.  See ojph_codestream_local.h for
   *  more documentation -- yet to be added.
   *
   */
  class OJPH_EXPORT codestream
  {
  public:
    /**
     *  @brief default constructor
     *
     *  This function instantiate the actual implementation object
     *  local::codestream, using new.
     *
     */
    codestream();

    /**
     *  @brief default destructor
     *
     *  This function destroys the internal local::codestream object.
     *
     */
    ~codestream();

    /**
     *  @brief Restarts the codestream.
     *
     *  This function restarts the codestream; after this call, the
     *  codestream object behaves like it has never been used before,
     *  except that all memory allocations are preserved.  Thus, after
     *  restart(), there is no need to allocate memory, unless the new
     *  codestream needs more storage to store codeblocks, or has a
     *  different structure.
     *
     *  restart() is useful if we are decoding multiple codestreams that
     *  have largely the same structure and byte length.
     *
     */
    void restart();

    /**
     *  @brief Sets the sequence of pushing or pull rows from the machinery.
     *
     *  For this function, planar means that the machinery processes one
     *  colour component in full before processing the next component.  This
     *  more efficient because the cache is used for one component instead of
     *  many components, but it is not practical when a color transform is
     *  employed. This is because we need to employ the transform to the first
     *  three  components.  Therefore, planar, while recommended, can only be
     *  used when there is no color transform.
     *
     *  @param planar true for when components are pushed in full one at
     *         a time.
     */
    void set_planar(bool planar);

    /**
     *  @brief Sets the codestream profile.
     *
     *  This is currently rather incomplete, but it accepts two profiles
     *  IMF and BROADCAST. More work is needed to improve this.
     *  Note that Rsiz field in the SIZ marker segment is not set properly.
     *
     *  @param s a string of the profile name, value can be from
     *           OJPH_PN_STRING_XXXX, where only IMF and BROADCAST
     *           are currently supported.
     */
    void set_profile(const char* s);

    /**
     *  @brief Sets the locations where a tile is partitioned into tile parts.
     *
     *  This function signals that we are interested in partitioning each tile
     *  into tile parts at resolution or component level, or both.  This is
     *  useful when used with the TLM marker segment, because the TLM marker
     *  segment provides information about the locations of these partitions in
     *  the file.  This way we can identify where resolution information ends
     *  within the codestream.  It is also useful when large images are
     *  compressed, because an unpartitioned tile cannot be more than 4GB, but
     *  when partitioned, each tile part can be 4GB -- it is possible to
     *  partition at precinct boundaries to better utilize tile parts, and
     *  achieve a tile in the vicinity of 1TB, but this option is currently
     *  unsupported.
     *
     *  @param at_resolutions partitions the tile into tile parts at
     *         resolutions.
     *  @param at_components partitions every tile into tile parts are
     *         components
     */
    void set_tilepart_divisions(bool at_resolutions, bool at_components);

    /**
     *  @brief Query if the tile will be partitioned at resolution boundary.
     *
     *  @return true if resolution-boundary tile partitioning is employed.
     *  @return false if resolution-boundary tile partitioning is not
     *          requested.
     */
    bool is_tilepart_division_at_resolutions();

    /**
     *  @brief Query if the tile will be partitioned at component boundary.
     *
     *  @return true if component-boundary tile partitioning is employed.
     *  @return false if component-boundary tile partitioning is not
     *          requested.
     */
    bool is_tilepart_division_at_components();

    /**
     *  @brief Request the addition of the optional TLM marker segment.
     *  This request should occur before writing codestream headers
     *  ojph::codestream::write_headers())
     *
     *  @param needed true when the marker is needed.
     */
    void request_tlm_marker(bool needed);

    /**
     *  @brief Query if the optional TLM marker segment is to be added.
     *
     *  @return true if the addition of the optional TLM marker segment
     *          is to be added.
     *  @return false if the addition of the optional TLM marker segment
     *          was not requested.
     */

    bool is_tlm_requested();

    /**
     *  @brief Writes codestream headers when the codestream is used for
     *  writing.  This function should be called after setting all the
     *  codestream parameters, but before pushing image lines using
     *  ojph::codestream::exchange().
     *
     *  @param file A class inherited from outfile_base, which used to store
     *              compressed image bitstream.  This enables storing the
     *              compressed bitstream to memory or an actual file.
     *  @param comments A pointer to an array of comment_exchange objects.
     *                  Each object stores one comment to be inserted in the
     *                  bitstreams.  The number of elements in the array
     *                  should be equal to num_comments.
     *  @param num_comments The number of elements in the `comments` array.
     *
     */
    void write_headers(outfile_base *file,
                       const comment_exchange* comments = NULL,
                       ui32 num_comments = 0);

    /**
     *  @brief This call is used to send image data rows to the library.
     *         We expect to send one row from a single component with
     *         each call. The first call is always with line == NULL;
     *         the call would return a line_buf, and the component
     *         number or index in `next_component.`  The caller would
     *         then need to fill the buffer of the line_buf with one
     *         row from the component indexed by `next_component`, and
     *         call exchange again to pass the component and get a
     *         new line_buf.
     *
     *  @param line A line_buf object; first call should supply NULL.
     *              Subsequent calls should pass the line_buf object
     *              obtained in the previous call.
     *  @param next_component returns a component index; the end user must
     *                        fill the returned line_buf from the component
     *                        indexed by this index.
     *  @return line_buf* A line_buf which must be filled with the component
     *                    indexed by `next_component`, before calling
     *                    exchange again to pass this line.
     */

    line_buf* exchange(line_buf* line, ui32& next_component);

    /**
     * @brief This is the last call to a writing (encoding) codestream.
     *        This will write encoded bitstream data to the file.  This
     *        call does not close the file, because, in the future, we
     *        might wish to write more data to the file.  If you do not
     *        want to write more data, then call codestream::close().
     */
    void flush();

    /**
     * @brief This enables codestream resilience; that is, the library tries
     *        its best to decode the codestream, even if there are errors.
     *        This call is for a decoding (or reading) codestream, and
     *        should be called before all other calls, before
     *        codestream::read_headers().
     */
    void enable_resilience();             // before read_headers

    /**
     * @brief This call reads the headers of a codestream.  It is for a
     *        reading (or decoding) codestream, and should be called
     *        after codestream::enable_resilience(), but before
     *        codestream::restrict_input_resolution().
     *
     * @param file The file to read from.  The file should be inherited from
     *             ojph::infile_base; this enables reading from an actual file
     *             or from memory-based file.
     */
    void read_headers(infile_base *file); // before resolution restrictions

    /**
     * @brief This function restricts resolution decoding for a codestream.
     *        It is for a reading (decoding) codestream.  We can limit the
     *        restrictions to decoding and reconstruction resolution,
     *        or decoding only.  Call this function after
     *        codestream::read_headers() but before codestream::create()
     *
     * @param skipped_res_for_data specifies for how many fine resolutions
     *                             decoding is skipped, i.e., reading and
     *                             decoding is not performed for this number
     *                             of fine resolutions.
     * @param skipped_res_for_recon specifies for how many fine resolutions
     *                              reconstruction is skipped; the resulting
     *                              image is smaller than the original.  This
     *                              number should be smaller or equal to
     *                              `skipped_res_for_data,` as it does not
     *                              make sense otherwise.
     */
    void restrict_input_resolution(ui32 skipped_res_for_data,
                                   ui32 skipped_res_for_recon); //before create

    /**
     * @brief This call is for a decoding (or reading) codestream.  Call this
     *        function after calling restrict_input_resolution(), if
     *        restrictions are needed.
     */
    void create();

    /**
     * @brief This call is to pull one row from the codestream, being
     *        decoded.  The returned line_buf object holds one row from
     *        the image; the returned comp_num tells the reader the
     *        component to which this row belongs.
     *
     * @param comp_num returns the component to which the returned
     *                 line_buf object belongs.
     * @return line_buf* this object holds one row of the component indexed
     *                   by comp_num.
     */
    line_buf* pull(ui32 &comp_num);

    /**
     * @brief Call this function to close the underlying file; works for both
     *        encoding and decoding codestreams.
     *
     */
    void close();

    /**
     * @brief Returns the underlying SIZ marker segment object
     *
     * @return param_siz This object holds SIZ marker segment information,
     *                   which deals with codestream dimensions, number
     *                   of components, bit depth, ... etc.
     */
    param_siz access_siz();

    /**
     * @brief Returns the underlying COD marker segment object
     *
     * @return param_cod This object holds COD marker segment information,
     *                   which deals with coding parameters, such as
     *                   codeblock sizes, progression order, reversible,
     *                   ... etc.
     */
    param_cod access_cod();

    /**
     * @brief Returns the underlying QCD marker segment object
     *
     * @return param_qcd This object holds QCD marker segment information,
     *                   which deals with quantization parameters --
     *                   quantization step size for each subband.
     */
    param_qcd access_qcd();

    /**
     * @brief Returns the underlying NLT marker segment object
     *
     * @return param_nlt This object holds NLT marker segment information,
     *                   which deals with non-linearity point transformation
     *                   for each component.
     */
    param_nlt access_nlt();

    /**
     * @brief Query if the codestream extraction is planar or not.
     * See the documentation for ojph::codestream::set_planar()
     *
     * @return true if it is planar
     * @return false if it is not planar (interleaved)
     */
    bool is_planar() const;

  private:
    local::codestream* state;
  };

}

#endif // !OJPH_CODESTREAM_H
