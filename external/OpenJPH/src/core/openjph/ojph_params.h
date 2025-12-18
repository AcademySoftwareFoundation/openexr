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
// File: ojph_params.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_PARAMS_H
#define OJPH_PARAMS_H

#include "ojph_arch.h"
#include "ojph_base.h"

namespace ojph {

  /***************************************************************************/
  // defined here
  class param_siz;
  class param_cod;
  class param_coc;
  class param_qcd;
  class param_cap;
  class param_nlt;
  class codestream;

  /***************************************************************************/
  // prototyping from local
  namespace local {
    struct param_siz;
    struct param_cod;
    struct param_coc;
    struct param_qcd;
    struct param_cap;
    struct param_nlt;
    class codestream;
  }

  /***************************************************************************/
  class OJPH_EXPORT param_siz
  {
  public:
    param_siz(local::param_siz *p) : state(p) {}

    //setters
    void set_image_extent(point extent);
    void set_tile_size(size s);
    void set_image_offset(point offset);
    void set_tile_offset(point offset);
    void set_num_components(ui32 num_comps);
    void set_component(ui32 comp_num, const point& downsampling,
                       ui32 bit_depth, bool is_signed);

    //getters
    point get_image_extent() const;
    point get_image_offset() const;
    size get_tile_size() const;
    point get_tile_offset() const;
    ui32 get_num_components() const;
    ui32 get_bit_depth(ui32 comp_num) const;
    bool is_signed(ui32 comp_num) const;
    point get_downsampling(ui32 comp_num) const;

    //deeper getters
    ui32 get_recon_width(ui32 comp_num) const;
    ui32 get_recon_height(ui32 comp_num) const;

  private:
    local::param_siz* state;
  };

  /***************************************************************************/
  class OJPH_EXPORT param_cod
  {
  public:
    param_cod(local::param_cod* p) : state(p) {}

    void set_num_decomposition(ui32 num_decompositions);
    void set_block_dims(ui32 width, ui32 height);
    void set_precinct_size(int num_levels, size* precinct_size);
    void set_progression_order(const char *name);
    void set_color_transform(bool color_transform);
    void set_reversible(bool reversible);
    param_coc get_coc(ui32 component_idx);

    ui32 get_num_decompositions() const;
    size get_block_dims() const;
    size get_log_block_dims() const;
    bool is_reversible() const;
    size get_precinct_size(ui32 level_num) const;
    size get_log_precinct_size(ui32 level_num) const;
    int get_progression_order() const;
    const char* get_progression_order_as_string() const;
    int get_num_layers() const;
    bool is_using_color_transform() const;
    bool packets_may_use_sop() const;
    bool packets_use_eph() const;
    bool get_block_vertical_causality() const;

  private:
    local::param_cod* state;
  };

  /***************************************************************************/
  class OJPH_EXPORT param_coc
  {
  public:
    param_coc(local::param_cod* p) : state(p) {}

    void set_num_decomposition(ui32 num_decompositions);
    void set_block_dims(ui32 width, ui32 height);
    void set_precinct_size(int num_levels, size* precinct_size);
    void set_reversible(bool reversible);

    ui32 get_num_decompositions() const;
    size get_block_dims() const;
    size get_log_block_dims() const;
    bool is_reversible() const;
    size get_precinct_size(ui32 level_num) const;
    size get_log_precinct_size(ui32 level_num) const;
    bool get_block_vertical_causality() const;

  private:
    local::param_cod* state;
  };

  /***************************************************************************/
  /**
    * @brief Quantization parameters object
    * 
    */
  class OJPH_EXPORT param_qcd
  {
  public:
    param_qcd(local::param_qcd* p) : state(p) {}

    /**
     * @brief Set the irreversible quantization base delta.  
     *  
     * This represents the default base delta and influences QCD marker 
     * segment
     * 
     * @param delta 
     */
    void set_irrev_quant(float delta);

    /**
     * @brief Set the irreversible quantization base delta for a specific 
     *        component
     * 
     * This represents the default base delta for component comp_idx, and 
     * influences QCC marker segment for the component, inserting one
     * if needed, which is usually the case.
     * 
     * @param comp_idx 
     * @param delta 
     */
    void set_irrev_quant(ui32 comp_idx, float delta);

  private:
    local::param_qcd* state;
  };

  /*************************************************************************/
  /**
    * @brief non-linearity point transformation object
    *        (implements NLT marker segment)
    * 
    *  There are a few things to know here.  
      * The NLT marker segment contains the nonlinearity type and the 
      * bit depth and signedness of the component to which it applies.
      * There is the default component ALL_COMPS which applies to all 
      * components unless it is overridden by another NLT segment marker.
      * The library checks that the settings make sense, and also make
      * sure that bit depth and signedness are correct, creating any missing
      * NLT marker segments in the process.
      * If all components have the same bit depth and signedness, and need
      * nonlinearity type 3 (Binary Complement to Sign Magnitude Conversion), 
      * then the best option is to set ALL_COMPS to type 3.
      * Otherwise, the best option is to set type 3 only to components that 
      * need it, leaving out the default ALL_COMPS nonlinearity not set.
      * Another option is for the end-user can set the ALL_COMPS to type 3, 
      * and then put exception for the components that does not need type 3, 
      * by setting them to type 0.
      * 
      * The library, during validity check, which is run when the codestream
      * is created for writing, will do the following:
      * -- If ALL_COMPS is set to type 0, it will be ignored, and the 
      * codestream will NOT have the corresponding NLT marker segment.
      * -- If ALL_COMPS is set to type 3, then the following will happen:
      *   - If all the components (except those with type 0 set for them) have 
      *   the same bit depth and signedness, then the ALL_COMPS NLT marker 
      *   segment will be respected and inserted into the codestream.
      *   Of course, components with NLT 0 will also have the corresponding
      *   NLT marker segment inserted.
      *   - If components, for which no NTL type 0 is specified, have differing
      *   bit depth or signedness, then the ALL_COMPS will be ignored, and 
      *   NLT markers are inserted for each component that needs type 3.
      * Components that have their component field larger than the number of
      * components in the codestream are removed.
      * 
      * It also worth noting that type 3 nonlinearity has no effect on 
      * positive image samples.  It is also not recommended for integer-valued 
      * types. It is only recommended for floating-point image samples, for 
      * which some of the samples are negative, where type 3 nonlinearity 
      * should be beneficial.  This is because the encoding engine expects 
      * two-complement representation for negative values while floating point 
      * numbers have a sign bit followed by an exponent, which has a biased 
      * integer representation.  The core idea is to make floating-point
      * representation more compatible with integer representation.

    * 
    */
  class OJPH_EXPORT param_nlt
  {
  public:
    enum special_comp_num : ui16 { ALL_COMPS = 65535 };
    enum nonlinearity : ui8 { 
      OJPH_NLT_NO_NLT = 0,                // supported
      OJPH_NLT_GAMMA_STYLE_NLT = 1,       // not supported
      OJPH_NLT_LUT_STYLE_NLT = 2,         // not supported
      OJPH_NLT_BINARY_COMPLEMENT_NLT = 3, // supported
      OJPH_NLT_UNDEFINED = 255          // This is used internally and is 
                                          // not part of the standard 
    };
  public:
    param_nlt(local::param_nlt* p) : state(p) {}

    /**
      * @brief enables or disables type 3 nonlinearity for a component 
      *        or the default setting
      * 
      * When creating a codestream for writing, call this function before
      * you call codestream::write_headers.
      * 
      * 
      * @param comp_num: component number, or 65535 for the default setting
      * @param type: desired non-linearity from enum nonlinearity
      */
    void set_nonlinear_transform(ui32 comp_num, ui8 nl_type);

    /**
      * @brief get the nonlinearity type associated with comp_num, which 
      *        should be one from enum nonlinearity
      *
      * @param comp_num: component number, or 65535 for the default setting
      * @param bit_depth: returns the bit depth of the component/default
      * @param is_signed: returns true if the component/default is signed
      * @param type: nonlinearity type
      * @return true if the nonlinearity for comp_num is set
      */
    bool get_nonlinear_transform(ui32 comp_num, ui8& bit_depth, 
                                 bool& is_signed, ui8& nl_type) const;

  private:
    local::param_nlt* state;
  };

  /***************************************************************************/
  class OJPH_EXPORT comment_exchange
  {
    friend class local::codestream;
  public:
    comment_exchange() : data(NULL), len(0), Rcom(0) {}
    void set_string(const char* str);
    void set_data(const char* data, ui16 len);

  private:
    const char* data;
    ui16 len;
    ui16 Rcom;
  };

}

#endif // !OJPH_PARAMS_H
