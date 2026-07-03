// Copyright (c) 2019 - 2026, Osamu Watanabe
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
//    modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

// ---------------------------------------------------------------------------
// Analytic per-subband visual (CSF) weighting for Qfactor quantization.
//
// PROTOTYPE: the default model is `legacy_table`, which returns the exact
// hard-coded Zeng et al. (Signal Processing: Image Communication 17, 2002,
// Table 2) weights that the Qfactor path has always used -- so the emitted
// QCD/QCC bytes are bit-identical unless an analytic model is explicitly
// requested. The analytic models replace that single-viewing-condition table
// with a contrast-sensitivity-function (CSF) evaluated at each subband's
// radial frequency, so the weighting follows an arbitrary viewing distance /
// zoom. See scripts/csf_fit_luma.py for the fit that calibrates `ref_ppd`:
//   * mannos_sakrison reproduces W_b_Y to RMS ~0.029 at ref_ppd ~= 72
//   * daly            reproduces W_b_Y near the paper's ~1700 px viewing
//                     distance but with a looser shape (RMS ~0.056)
// Header-only and dependency-free (only <cmath>/<vector>/<cstdint>) so it can
// be exercised by a standalone demo as well as by the marker code.
// ---------------------------------------------------------------------------

#include <cmath>
#include <cstdint>
#include <vector>

namespace open_htj2k {

enum class csf_model {
  legacy_table,     // exact Zeng Table 2 weights (default; bit-identical output)
  mannos_sakrison,  // analytic Mannos-Sakrison CSF
  daly              // analytic Daly CSF (light-adaptation form)
};

// Color decorrelation actually in force, which drives BOTH the per-component
// synthesis gain and whether a QCC component is treated as chroma (opponent) or
// luma (e.g. undecorrelated RGB). Quantization error in component c is amplified
// by ||column_c(MCT^-1)||_2 in reconstructed (R,G,B) space, so the step is scaled
// by 1/G_c. With no MCT every component is independent (unit gain, luma role).
enum class color_transform {
  ict,  // irreversible 9/7 MCT (YCbCr): ICT inverse column-norm gains, chroma roles
  none  // no MCT: unit gains, luma role for every component (RGB / generic)
};

// Viewing condition for analytic weighting. Defaults select the legacy table,
// so a default-constructed value reproduces the historical Qfactor output.
struct visual_weighting_params {
  csf_model model = csf_model::legacy_table;
  // Reference pixels-per-degree at zoom 1.0. Calibrated so that
  // mannos_sakrison reproduces the legacy W_b_Y table (see header note).
  double ref_ppd = 72.0;
  // Display magnification. zoom > 1 (zoom-in) lowers the effective ppd, shifts
  // every subband to a lower cycles/degree, and flattens the weighting toward
  // 1.0 (i.e. toward flat MSE-optimal quantization) -- the correct limit when
  // the viewer is close / heavily magnified.
  double zoom = 1.0;
  // Radial-frequency multiplier for the diagonal (HH) subband relative to the
  // horizontal/vertical (LH/HL) center. Geometric value is sqrt(2); the legacy
  // table behaves closer to ~1.25 (no oblique-effect penalty).
  double hh_factor = 1.4142135623730951;
};

// --- color synthesis gain --------------------------------------------------
//
// G_c = ||column c of the inverse decorrelating transform||_2. Quantization
// error in component c is amplified by this in reconstructed (R,G,B) space, so
// the step is scaled by 1/G_c. For a future Part-2 custom MCT this would be a
// column norm of the actual inverse matrix; the built-in ICT path uses the
// historical 4-dp literals so the emitted QCD/QCC stays bit-identical.

// Per-component synthesis gain. `none` (no MCT) -> unit gain (independent
// components). `ict` -> the inverse-ICT column norms {sqrt(3), 1.80511, 1.57340}
// rounded to 4 dp exactly as the legacy encoder stored them.
inline double color_gain(color_transform ct, int comp_index) {
  if (ct == color_transform::none) return 1.0;
  static const double g[3] = {1.7321, 1.8051, 1.5734};
  return g[(comp_index >= 0 && comp_index < 3) ? comp_index : 0];
}

// Resolve the effective transform for color scaling. Legacy mode reproduces the
// historical encoder, which assumed ICT regardless of the real MCT; analytic
// modes honor the transform actually applied (identity when the MCT is off).
inline color_transform resolve_color_transform(const visual_weighting_params &vp, bool mct_on) {
  if (vp.model == csf_model::legacy_table) return color_transform::ict;
  return mct_on ? color_transform::ict : color_transform::none;
}

// --- Qfactor -> base quantization scale ------------------------------------
//
// Maps the 1..100 Qfactor to the reference step delta and the exponent applied
// to the per-subband visual weights. Shared verbatim by QCD, QCC, and
// estimate_qfactor so all three invert to identical steps (guarded by the
// qfest_* round-trip tests). RI is the component dynamic range in bits.
struct q_scaling {
  double delta_Q;        // reference step before basis / visual / color normalization
  double qfactor_power;  // exponent applied to each subband's visual weight
};
inline q_scaling q_to_delta(uint8_t qfactor, uint8_t RI) {
  const uint8_t t0 = 65, t1 = 97;
  const double alpha_T0 = 0.04, alpha_T1 = 0.10;
  const double M_T0     = 2.0 * (1.0 - t0 / 100.0);
  const double M_T1     = 2.0 * (1.0 - t1 / 100.0);
  const double M_Q      = (qfactor < 50) ? 50.0 / qfactor : 2.0 * (1.0 - qfactor / 100.0);
  double alpha_Q        = alpha_T0;
  double qfactor_power  = 1.0;
  if (qfactor >= t1) {
    qfactor_power = 0.0;
    alpha_Q       = alpha_T1;
  } else if (qfactor > t0) {
    qfactor_power = (std::log(M_T1) - std::log(M_Q)) / (std::log(M_T1) - std::log(M_T0));
    alpha_Q       = alpha_T1 * std::pow(alpha_T0 / alpha_T1, qfactor_power);
  }
  // eps0 = sqrt(1/2) / 2^RI. Use ldexp rather than `1 << RI`: bit-identical for
  // the encoder's RI (<= 16 bpp) but well-defined for any RI, so a crafted
  // high-bit-depth file fed to estimate_qfactor cannot trigger signed-shift UB.
  const double eps0 = std::sqrt(0.5) * std::ldexp(1.0, -static_cast<int>(RI));
  return {alpha_Q * M_Q + eps0, qfactor_power};
}

// --- CSF models (spatial frequency f in cycles/degree) ---------------------

inline double mannos_sakrison_csf(double f) {
  return 2.6 * (0.0192 + 0.114 * f) * std::exp(-std::pow(0.114 * f, 1.1));
}

inline double daly_csf(double f) {
  // Daly 1993 CSF core, light-adaptation form at L = 100 cd/m^2. The low-
  // frequency image-area term is omitted because the low side is clamped flat.
  constexpr double L = 100.0, eps = 0.9;
  const double A = 0.801 * std::pow(1.0 + 0.7 / L, -0.2);
  const double B = 0.3 * std::pow(1.0 + 100.0 / L, 0.15);
  const double x = B * eps * f;
  return A * eps * f * std::exp(-x) * std::sqrt(1.0 + 0.06 * std::exp(x));
}

inline double csf_value(double f, csf_model m) {
  return (m == csf_model::daly) ? daly_csf(f) : mannos_sakrison_csf(f);
}

// Guard a visual weight against extreme viewing conditions (very large effective
// ppd) where the CSF underflows to 0 or Daly's exp() overflows to NaN. Either
// would make the step `delta/(basis*w*G)` divide by zero / propagate NaN and
// emit a corrupt QCD/QCC. Replaces only non-finite or non-positive values, so it
// is a no-op for every realistic weight (bit-identical normal output).
inline double finite_weight(double w) {
  return (std::isfinite(w) && w > 0.0) ? w : 1e-300;
}

// Peak (frequency, amplitude) of a CSF, found once by a coarse scan. Used to
// normalize the weight to <= 1 and to clamp the band-pass low side to flat.
struct csf_peak_t {
  double f_peak;
  double h_peak;
};

inline csf_peak_t csf_peak(csf_model m) {
  auto scan = [](csf_model mm) {
    csf_peak_t pk{0.0, 0.0};
    for (int i = 1; i < 100000; ++i) {
      const double f = i * 0.002;  // 0.002 .. 200 cpd
      const double v = csf_value(f, mm);
      if (v > pk.h_peak) {
        pk.h_peak = v;
        pk.f_peak = f;
      }
    }
    return pk;
  };
  // Peak depends only on the model -> compute each once (thread-safe magic statics).
  static const csf_peak_t mannos_pk = scan(csf_model::mannos_sakrison);
  static const csf_peak_t daly_pk   = scan(csf_model::daly);
  return (m == csf_model::daly) ? daly_pk : mannos_pk;
}

// Normalized CSF weight: flat (= 1) at/below the peak, falling CSF above it.
inline double csf_weight(double f, csf_model m, const csf_peak_t &pk) {
  if (f <= pk.f_peak) return 1.0;
  return finite_weight(csf_value(f, m) / pk.h_peak);
}

// Square-root-domain luminance visual weights, one per detail subband, in the
// QCD/wmse build order (per level, finest first): [HH_l, LH_l, HL_l]. Length
// is 3 * dwt_levels for analytic models; the LL band is handled by the caller
// (weight 1.0). In legacy mode the historical 15-entry table is returned
// verbatim, so the caller's existing out-of-range guard still applies.
inline std::vector<double> luma_visual_weights(uint8_t dwt_levels,
                                               const visual_weighting_params &vp) {
  if (vp.model == csf_model::legacy_table) {
    // Zeng et al., Table 2, Y column (= sqrt of the MSE-domain weights).
    return {0.0901, 0.2758, 0.2758, 0.7018, 0.8378, 0.8378, 1.0000, 1.0000,
            1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000};
  }

  const csf_peak_t pk = csf_peak(vp.model);
  const double zoom    = (vp.zoom > 0.0) ? vp.zoom : 1.0;
  const double ppd     = vp.ref_ppd / zoom;  // zoom-in lowers effective ppd
  const double f_N     = ppd / 2.0;          // Nyquist in cycles/degree

  std::vector<double> w;
  w.reserve(static_cast<size_t>(3) * dwt_levels);
  for (uint8_t lvl = 1; lvl <= dwt_levels; ++lvl) {
    // Geometric-mean radial center of octave band [f_N/2^lvl, f_N/2^(lvl-1)].
    const double f_r  = f_N * std::pow(2.0, -static_cast<double>(lvl)) * std::sqrt(2.0);
    const double f_hh = f_r * vp.hh_factor;
    w.push_back(csf_weight(f_hh, vp.model, pk));  // HH
    w.push_back(csf_weight(f_r, vp.model, pk));   // LH
    w.push_back(csf_weight(f_r, vp.model, pk));   // HL
  }
  return w;
}

// --- chroma (chrominance) analytic weighting -------------------------------
//
// Chrominance CSF is low-pass (not band-pass like luminance) and rolls off well
// below the luminance CSF. A single per-channel CSF, evaluated at each subband's
// per-axis radial frequency, reproduces ALL of the historical 4:4:4 / 4:2:0 /
// 4:2:2 QCC tables: chroma subsampling is just a frequency shift (sx, sy)
// applied to the same CSF, which also yields the 4:2:2 LH != HL anisotropy for
// free. Parameters below were fit to the 4:4:4 Cb/Cr tables at ref_ppd = 72
// (see scripts/csf_fit_chroma.py): Cb RMS 0.010, Cr RMS 0.021; predicted 4:2:0 / 4:2:2
// RMS <= 0.064. The chroma CSF is independent of vp.model's luminance choice.

// Stretched-exponential low-pass chroma CSF (= 1 at DC). f in cycles/degree.
inline double chroma_csf(double f, double a, double b) { return std::exp(-std::pow(a * f, b)); }

// Calibrated chroma CSF parameters per opponent channel (at ref_ppd = 72).
struct chroma_csf_params {
  double a, b;
};
inline chroma_csf_params chroma_params_for(int comp_index) {
  // comp_index 1 = Cb (blue-yellow, steeper roll-off); otherwise Cr (red-green).
  return (comp_index == 1) ? chroma_csf_params{0.1173, 0.840} : chroma_csf_params{0.0699, 1.050};
}

// Exact historical QCC table row (sqrt-domain). comp_index 1 = Cb, else Cr;
// chroma_format 0 = 4:4:4, 1 = 4:2:0, 2 = 4:2:2 (matches j2kmarkers YCC*).
inline std::vector<double> legacy_chroma_row(int comp_index, int chroma_format) {
  static const double Cb444[15] = {0.0263, 0.0863, 0.0863, 0.1362, 0.2564, 0.2564, 0.3346, 0.4691,
                                   0.4691, 0.5444, 0.6523, 0.6523, 0.7078, 0.7797, 0.7797};
  static const double Cr444[15] = {0.0773, 0.1835, 0.1835, 0.2598, 0.4130, 0.4130, 0.5040, 0.6464,
                                   0.6464, 0.7220, 0.8254, 0.8254, 0.8769, 0.9424, 0.9424};
  static const double Cb420[15] = {0.1362, 0.2564, 0.2564, 0.3346, 0.4691, 0.4691, 0.5444, 0.6523,
                                   0.6523, 0.7078, 0.7797, 0.7797, 1.0000, 1.0000, 1.0000};
  static const double Cr420[15] = {0.2598, 0.4130, 0.4130, 0.5040, 0.6464, 0.6464, 0.7220, 0.8254,
                                   0.8254, 0.8769, 0.9424, 0.9424, 1.0000, 1.0000, 1.0000};
  static const double Cb422[15] = {0.0863, 0.0863, 0.2564, 0.2564, 0.2564, 0.4691, 0.4691, 0.4691,
                                   0.6523, 0.6523, 0.6523, 0.7797, 0.7797, 0.7797, 1.0000};
  static const double Cr422[15] = {0.1835, 0.1835, 0.4130, 0.4130, 0.4130, 0.6464, 0.6464, 0.6464,
                                   0.8254, 0.8254, 0.8254, 0.9424, 0.9424, 0.9424, 1.0000};
  const double *r;
  switch (chroma_format) {
    case 1:  r = (comp_index == 1) ? Cb420 : Cr420; break;  // 4:2:0
    case 2:  r = (comp_index == 1) ? Cb422 : Cr422; break;  // 4:2:2
    default: r = (comp_index == 1) ? Cb444 : Cr444; break;  // 4:4:4
  }
  return std::vector<double>(r, r + 15);
}

// Square-root-domain chroma visual weights, one per detail subband, in QCC order
// (per level, finest first): [HH_l, LH_l, HL_l]. legacy_table returns the
// historical row verbatim (bit-identical). Analytic models fold chroma
// subsampling into the effective horizontal/vertical ppd, so LH (vertical
// detail) and HL (horizontal detail) diverge under 4:2:2 as they should.
inline std::vector<double> chroma_visual_weights(uint8_t dwt_levels,
                                                 const visual_weighting_params &vp, int comp_index,
                                                 int chroma_format,
                                                 color_transform ct = color_transform::ict) {
  if (vp.model == csf_model::legacy_table) {
    return legacy_chroma_row(comp_index, chroma_format);
  }
  // Without a luma/chroma decorrelating transform this component carries
  // luminance (e.g. a raw RGB channel), so it takes the luminance CSF, never
  // the chroma roll-off. Such components are not subsampled (full resolution).
  if (ct == color_transform::none) {
    return luma_visual_weights(dwt_levels, vp);
  }

  double sx = 1.0, sy = 1.0;  // horizontal/vertical chroma subsampling factors
  if (chroma_format == 1) {   // 4:2:0
    sx = 2.0;
    sy = 2.0;
  } else if (chroma_format == 2) {  // 4:2:2
    sx = 2.0;
    sy = 1.0;
  }

  const chroma_csf_params cp = chroma_params_for(comp_index);
  const double zoom          = (vp.zoom > 0.0) ? vp.zoom : 1.0;
  const double f_N           = (vp.ref_ppd / zoom) / 2.0;  // luma Nyquist (cpd)
  const double f_Nx          = f_N / sx;                   // chroma horizontal Nyquist
  const double f_Ny          = f_N / sy;                   // chroma vertical Nyquist

  std::vector<double> w;
  w.reserve(static_cast<size_t>(3) * dwt_levels);
  for (uint8_t lvl = 1; lvl <= dwt_levels; ++lvl) {
    const double dx = f_Nx * std::pow(2.0, -static_cast<double>(lvl)) * std::sqrt(2.0);
    const double dy = f_Ny * std::pow(2.0, -static_cast<double>(lvl)) * std::sqrt(2.0);
    // HH radial = geometric per-axis diagonal, scaled by hh_factor relative to the
    // isotropic sqrt(2) so the same knob tunes luma and chroma (default sqrt(2) = no change).
    const double f_hh = (vp.hh_factor / std::sqrt(2.0)) * std::sqrt(dx * dx + dy * dy);
    // chroma_csf = exp(-(a*f)^b) is <= 1 by construction; finite_weight only
    // guards the underflow-to-0 case at extreme effective ppd.
    w.push_back(finite_weight(chroma_csf(f_hh, cp.a, cp.b)));  // HH
    w.push_back(finite_weight(chroma_csf(dy, cp.a, cp.b)));    // LH (vertical detail)
    w.push_back(finite_weight(chroma_csf(dx, cp.a, cp.b)));    // HL (horizontal detail)
  }
  return w;
}

}  // namespace open_htj2k
