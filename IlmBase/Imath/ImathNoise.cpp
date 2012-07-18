//
//     Copyright (C) Pixar. All rights reserved.
//     
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//     
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//     
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//     
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
// 
  
//-----------------------------------------------------
//
// Implementation of non-template items declared in ImathNoise.h
//
//-----------------------------------------------------

#include <ImathNoise.h>
#include <ImathFun.h>

namespace Imath {

// Local namespace to avoid any possible
// conflicts
namespace {

static const int TABSIZE = 1<<8;
static const int TABMASK = TABSIZE-1;

// Need these structs because compiler disallows inline declaration of an array
// of templated types.
// FIXME someday?
struct gradF
{
    float x;
    float y;
    float z;
}; 

struct gradD
{
    double x;
    double y;
    double z;
};

// Gradient table
// Note that this gradient table is prescaled to avoid extra multiplications
#define __GRADS {                               \
    { 0.788314, -0.627167, -0.773665 },         \
    { 0.835654, 0.069542, 0.954034 },           \
    { 0.862828, -0.309454, 0.879260 },          \
    { 0.042682, 0.584865, 1.126696 },           \
    { 0.737730, 0.026284, 1.033633 },           \
    { 0.110664, -0.828499, 0.956388 },          \
    { 0.424872, -1.138934, 0.368302 },          \
    { 0.654991, 0.665466, -0.861089 },          \
    { 0.635469, 0.311065, -1.054870 },          \
    { 0.338959, 1.132728, -0.464077 },          \
    { 0.916530, 0.646707, 0.595883 },           \
    { 0.987652, 0.798522, 0.015472 },           \
    { 0.500117, -0.418634, -1.089937 },         \
    { -0.576124, -0.181586, 1.117337 },         \
    { 0.568912, -0.375714, -1.071686 },         \
    { -0.179357, -1.201633, 0.370459 },         \
    { -1.006695, 0.136077, -0.762483 },         \
    { 1.068612, -0.317531, -0.608748 },         \
    { 0.358635, -1.203018, 0.193550 },          \
    { -0.995783, -0.106278, 0.781316 },         \
    { -0.407784, 0.927005, -0.766622 },         \
    { 0.635842, 0.958725, 0.538411 },           \
    { 0.581987, 0.596964, 0.958258 },           \
    { 1.111869, 0.270864, 0.551103 },           \
    { 0.991703, -0.216817, 0.763445 },          \
    { -1.061301, 0.237904, 0.656030 },          \
    { -0.055740, -0.617158, 1.108757 },         \
    { 1.255156, -0.058470, -0.185734 },         \
    { -0.823359, -0.913780, -0.316890 },        \
    { -1.211957, -0.365441, -0.104626 },        \
    { 1.002084, -0.773715, -0.102592 },         \
    { -0.609635, 0.953535, 0.576585 },          \
    { 1.225272, 0.097679, 0.320154 },           \
    { 0.342458, -1.106212, 0.521874 },          \
    { -0.995704, 0.081499, -0.784386 },         \
    { -0.906387, -0.755130, 0.470716 },         \
    { -0.807056, -0.608014, -0.769619 },        \
    { 0.958624, -0.482824, -0.679156 },         \
    { 1.071986, 0.351351, -0.583722 },          \
    { 0.428542, -0.204119, -1.178143 },         \
    { 0.088317, 0.307805, 1.229141 },           \
    { 1.088993, 0.595928, 0.268879 },           \
    { 0.585054, -0.122038, -1.120782 },         \
    { -0.602183, -0.264804, 1.086548 },         \
    { -0.941179, -0.813802, -0.255425 },        \
    { 0.508118, 0.621588, 0.984264 },           \
    { -0.955647, 0.836402, -0.022417 },         \
    { 0.940401, -0.852504, 0.047080 },          \
    { 0.202505, -1.226986, 0.258515 },          \
    { 0.316072, 1.141490, -0.458730 },          \
    { 1.117972, -0.033154, 0.601973 },          \
    { 0.897958, 0.395254, -0.806708 },          \
    { -1.072434, -0.633395, -0.249058 },        \
    { -0.108024, 0.203028, -1.249177 },         \
    { 1.175307, -0.337381, -0.343747 },         \
    { -0.644246, -0.904131, 0.617112 },         \
    { -0.542979, -0.759639, 0.861078 },         \
    { 0.901807, 0.674722, -0.587219 },          \
    { 0.406597, -1.165070, -0.301035 },         \
    { 0.231152, 1.173626, -0.427204 },          \
    { 0.240216, -1.229258, 0.211081 },          \
    { 1.088987, -0.548564, 0.355693 },          \
    { 0.514074, -0.793976, 0.847740 },          \
    { -0.918495, -0.394216, -0.783770 },        \
    { -1.231720, 0.084433, -0.298444 },         \
    { 0.779848, -0.727575, -0.689786 },         \
    { -1.075781, -0.674133, 0.039659 },         \
    { 0.496633, 0.282983, -1.134289 },          \
    { 0.891475, -0.364374, 0.828152 },          \
    { 0.454983, -1.163763, 0.227991 },          \
    { -0.388587, -0.679547, -1.000274 },        \
    { 1.191951, 0.427388, 0.099628 },           \
    { 0.422267, 1.105700, 0.460923 },           \
    { 0.852907, -0.356047, 0.871271 },          \
    { 0.078403, -0.153410, -1.258432 },         \
    { -0.785527, 0.482813, 0.873597 },          \
    { -1.235010, -0.125301, -0.269042 },        \
    { 0.167053, -1.190004, 0.411482 },          \
    { 0.736825, 0.969256, 0.361893 },           \
    { 1.185784, -0.187628, 0.414782 },          \
    { 0.950462, 0.768893, -0.344616 },          \
    { 0.650843, 0.611536, -0.903194 },          \
    { 0.374910, -1.105191, -0.501329 },         \
    { 0.705070, -0.738210, 0.755814 },          \
    { -0.845956, -0.724609, -0.610436 },        \
    { -0.788504, -0.883724, -0.458942 },        \
    { 0.719097, 0.885464, 0.558737 },           \
    { -0.957466, -0.682968, -0.479736 },        \
    { 0.678558, -0.986781, 0.423267 },          \
    { -0.601691, 1.114939, -0.090623 },         \
    { -0.426899, -1.114558, -0.434568 },        \
    { -0.187710, 1.096872, -0.612348 },         \
    { 0.906217, 0.491324, 0.742095 },           \
    { -0.503540, -0.219473, 1.145256 },         \
    { -0.800428, 0.697720, 0.697017 },          \
    { 0.814789, -0.968967, -0.102735 },         \
    { -1.008348, 0.772234, 0.014864 },          \
    { 0.123590, 0.320934, -1.222726 },          \
    { 0.409058, 0.926047, 0.767099 },           \
    { 0.570782, -0.232945, 1.110530 },          \
    { -0.041734, -1.021842, -0.753280 },        \
    { 1.114086, 0.581667, -0.183877 },          \
    { -0.057148, 0.808571, -0.977895 },         \
    { 0.235176, 1.166170, -0.445052 },          \
    { 0.191218, 0.761957, 0.998092 },           \
    { 0.222648, -1.244770, -0.119625 },         \
    { -0.178733, 0.731520, -1.022871 },         \
    { 0.445567, 0.371098, 1.130083 },           \
    { 0.970801, 0.746381, 0.337333 },           \
    { -0.992364, -0.654808, -0.446960 },        \
    { 0.266218, 1.241710, 0.024866 },           \
    { 1.258707, -0.124089, 0.116583 },          \
    { -1.137213, 0.488463, -0.285456 },         \
    { 0.358283, -0.898286, 0.823436 },          \
    { 0.087521, 1.230420, -0.302886 },          \
    { -0.951069, 0.556370, -0.631864 },         \
    { -0.145136, 0.302903, -1.224956 },         \
    { 0.852662, -0.941234, 0.019464 },          \
    { 0.620505, 1.088370, -0.209184 },          \
    { -0.263752, -0.659466, 1.053030 },         \
    { -0.828791, -0.911600, 0.308908 },         \
    { -0.245717, 1.243459, 0.082263 },          \
    { -0.679384, -0.097832, -1.068738 },        \
    { -0.292610, 1.204928, 0.275430 },          \
    { 1.119552, 0.505687, -0.322825 },          \
    { 0.609661, 0.553285, -0.967223 },          \
    { -0.582157, 0.986451, 0.548945 },          \
    { 0.610331, -0.465619, 1.011942 },          \
    { -0.183770, 1.052674, -0.686614 },         \
    { -0.888905, 0.739944, -0.525038 },         \
    { -1.220009, 0.351023, -0.041166 },         \
    { -1.005903, 0.010069, -0.775494 },         \
    { -0.311862, 1.011240, -0.702473 },         \
    { -0.773117, 0.948144, 0.341536 },          \
    { -1.207578, 0.383727, 0.088551 },          \
    { 0.302053, -0.453340, 1.147422 },          \
    { -0.699607, -0.070289, 1.057801 },         \
    { 0.838653, 0.022693, 0.953666 },           \
    { -0.487446, -0.049690, 1.171862 },         \
    { -0.583229, -0.997283, 0.527828 },         \
    { 0.582716, 1.108410, -0.212604 },          \
    { 0.705140, -0.773962, -0.719092 },         \
    { 0.951171, -0.841553, -0.019865 },         \
    { -0.189525, 1.062100, -0.670340 },         \
    { -0.920539, 0.172259, 0.858060 },          \
    { -0.520160, -1.077708, -0.425809 },        \
    { -0.655485, 0.112743, -1.082109 },         \
    { -0.285007, -0.926371, 0.820939 },         \
    { -1.030567, -0.281194, -0.687164 },        \
    { 0.054927, 0.607216, 1.114272 },           \
    { 0.158439, 0.130549, -1.253470 },          \
    { 0.607180, 0.319168, 1.069017 },           \
    { 0.352076, 1.177935, -0.319130 },          \
    { -0.356005, -0.596175, 1.063565 },         \
    { -0.799209, 0.813891, -0.558731 },         \
    { -0.394995, 1.127402, 0.431599 },          \
    { 0.958186, 0.825377, -0.118173 },          \
    { 0.901745, 0.320422, 0.835176 },           \
    { 0.107000, -0.046214, -1.264812 },         \
    { 0.640494, 1.010901, 0.425652 },           \
    { 1.247362, 0.000485, 0.239629 },           \
    { -0.458720, 0.814407, -0.860030 },         \
    { 1.262387, 0.122437, 0.068710 },           \
    { 0.018970, 0.460893, 1.183448 },           \
    { 1.041231, 0.712477, -0.146786 },          \
    { -1.203936, -0.401310, 0.053105 },         \
    { -0.957886, -0.367324, 0.748905 },         \
    { 0.596976, -1.072770, -0.325754 },         \
    { -0.271895, -1.047016, -0.665706 },        \
    { -1.261994, 0.143359, 0.012337 },          \
    { 0.319336, -0.624347, 1.059031 },          \
    { -0.715401, -0.758312, -0.725603 },        \
    { 0.588401, -0.163489, -1.113727 },         \
    { 1.087744, -0.460141, -0.467350 },         \
    { 0.784349, 0.081183, -0.995760 },          \
    { -0.221156, 0.716483, 1.025221 },          \
    { 0.296764, -0.128480, 1.228315 },          \
    { -1.224266, 0.081436, -0.328441 },         \
    { -0.285543, -0.472121, -1.144072 },        \
    { -1.122514, -0.057485, -0.591600 },        \
    { 0.257123, 1.055719, -0.657783 },          \
    { 0.585054, -0.134138, 1.119397 },          \
    { -0.408430, -0.987509, -0.686545 },        \
    { -1.004570, -0.206791, -0.749272 },        \
    { -0.309829, 1.203774, 0.261282 },          \
    { -0.299370, 0.037320, 1.233822 },          \
    { 0.250466, -0.771841, 0.977170 },          \
    { 0.749622, 0.263278, 0.991002 },           \
    { 0.296587, -1.048701, -0.652376 },         \
    { -0.149659, -0.202717, -1.244926 },        \
    { 0.852538, 0.515000, -0.788217 },          \
    { 0.265281, -0.506737, -1.134097 },         \
    { -0.327238, -0.168619, 1.215655 },         \
    { 1.162758, -0.208046, 0.466951 },          \
    { 1.032532, 0.328815, 0.662639 },           \
    { 1.269059, -0.015245, 0.050896 },          \
    { -1.081859, -0.272825, 0.607027 },         \
    { 0.475025, 0.094649, -1.174191 },          \
    { 0.047901, -1.062886, 0.693767 },          \
    { -0.896166, -0.885498, -0.161599 },        \
    { -0.475727, -0.253498, 1.150111 },         \
    { 0.405294, 0.662314, 1.005191 },           \
    { 0.274705, 1.200667, 0.310273 },           \
    { 0.693373, 0.783180, 0.720552 },           \
    { 0.061456, 0.218791, 1.249675 },           \
    { 0.435662, -0.951332, -0.720070 },         \
    { -0.145699, -1.146880, 0.526092 },         \
    { -0.478284, 1.117453, -0.368616 },         \
    { 1.135808, -0.281828, 0.493807 },          \
    { -1.103055, -0.318719, 0.543158 },         \
    { 0.026058, -0.255878, 1.243858 },          \
    { -0.169674, 1.254070, -0.108868 },         \
    { -1.169501, -0.037295, 0.494178 },         \
    { -1.143053, -0.235727, 0.501195 },         \
    { 0.745852, -0.832007, 0.603988 },          \
    { -0.337013, 0.859032, 0.872823 },          \
    { -0.084982, 1.259383, 0.141651 },          \
    { 0.555071, -1.140266, 0.070868 },          \
    { -0.478393, -1.174910, 0.063697 },         \
    { -0.513698, 0.055100, -1.160350 },         \
    { -0.047833, -0.777682, -1.003122 },        \
    { 0.444149, 1.077675, 0.504658 },           \
    { 1.196798, -0.418619, 0.075926 },          \
    { 0.204415, -1.253602, -0.005455 },         \
    { 0.175366, -0.713941, -1.035794 },         \
    { 1.031947, -0.583500, 0.456009 },          \
    { 0.825104, -0.955708, -0.138414 },         \
    { 0.634382, -0.767285, 0.788777 },          \
    { -1.223932, 0.059993, 0.334253 },          \
    { -0.845312, -0.907721, 0.273539 },         \
    { -1.161587, -0.367260, 0.359399 },         \
    { 0.196107, -0.569707, -1.118172 },         \
    { -0.849886, 0.522510, -0.786135 },         \
    { -0.263671, 1.201331, -0.317197 },         \
    { -0.666174, -0.375187, 1.014288 },         \
    { 1.072889, -0.029637, -0.679238 },         \
    { 1.077886, 0.496687, 0.452545 },           \
    { 0.256237, -0.949089, 0.804305 },          \
    { -0.122358, -0.285378, 1.231633 },         \
    { 1.113703, -0.586025, -0.171966 },         \
    { 0.968073, -0.819611, 0.066380 },          \
    { 0.914228, 0.164627, -0.866266 },          \
    { 1.077638, 0.209107, -0.638987 },          \
    { 0.583443, 0.547600, -0.986439 },          \
    { -0.242707, 1.245431, -0.057689 },         \
    { 0.807001, -0.746150, -0.636664 },         \
    { -0.347392, -0.890262, -0.836710 },        \
    { -0.759495, -0.904449, -0.467411 },        \
    { -0.279167, 0.720129, 1.008371 },          \
    { 0.193653, 0.648045, -1.075113 },          \
    { -0.289260, -0.738665, -0.991985 },        \
    { 0.237874, 0.986703, 0.763653 },           \
    { -0.804498, 0.729487, -0.658760 },         \
    { 0.025529, -0.164599, 1.259201 },          \
    { -0.576384, -0.280181, 1.096637 },         \
    { -1.126249, -0.305677, -0.501456 },        \
}

// Gradient tables
static const gradD gradsV3d[TABSIZE] = __GRADS;
static const gradF gradsV3f[TABSIZE] = __GRADS;

// Permutation table
static unsigned const char permute[TABSIZE] = {
    225,155,210,108,175,199,221,144,203,116, 70,213, 69,158, 33,252,
    5, 82,173,133,222,139,174, 27,  9, 71, 90,246, 75,130, 91,191,
    169,138,  2,151,194,235, 81,  7, 25,113,228,159,205,253,134,142,
    248, 65,224,217, 22,121,229, 63, 89,103, 96,104,156, 17,201,129,
    36,  8,165,110,237,117,231, 56,132,211,152, 20,181,111,239,218,
    170,163, 51,172,157, 47, 80,212,176,250, 87, 49, 99,242,136,189,
    162,115, 44, 43,124, 94,150, 16,141,247, 32, 10,198,223,255, 72,
    53,131, 84, 57,220,197, 58, 50,208, 11,241, 28,  3,192, 62,202,
    18,215,153, 24, 76, 41, 15,179, 39, 46, 55,  6,128,167, 23,188,
    106, 34,187,140,164, 73,112,182,244,195,227, 13, 35, 77,196,185,
    26,200,226,119, 31,123,168,125,249, 68,183,230,177,135,160,180,
    12,  1,243,148,102,166, 38,238,251, 37,240,126, 64, 74,161, 40,
    184,149,171,178,101, 66, 29, 59,146, 61,254,107, 42, 86,154,  4,
    236,232,120, 21,233,209, 45, 98,193,114, 78, 19,206, 14,118,127,
    48, 79,147, 85, 30,207,219, 54, 88,234,190,122, 95, 67,143,109,
    137,214,145, 93, 92,100,245,  0,216,186, 60, 83,105, 97,204, 52
};

inline int phi( int x )
{
    return permute[x & TABMASK];
}

inline float cubicWeight( float t )
{
    // 3 t^2 - 2 t^3
    return t * t * ( 3.0f - 2.0f * t );
}

inline double cubicWeight( double t )
{
    // 3 t^2 - 2 t^3
    return t * t * ( ((double)3.0) - ((double)2.0) * t );
}

inline float cubicWeightGrad( float t )
{
    // 6 t - 6 t^2
    return t * ( 6.0f - 6.0f * t );
}

inline double cubicWeightGrad( double t )
{
    // 6 t - 6 t^2
    return t * ( ((double)6.0) - ((double)6.0) * t );
}

template <class T>
inline Vec2<T> cubicWeight( const Vec2<T> &t )
{
    return Vec2<T>( cubicWeight( t[0] ),
                    cubicWeight( t[1] ) );
}

template <class T>
inline Vec2<T> cubicWeightGrad( const Vec2<T> &t )
{
    return Vec2<T>( cubicWeightGrad( t[0] ),
                    cubicWeightGrad( t[1] ) );
}

template <class T>
inline Vec3<T> cubicWeight( const Vec3<T> &t )
{
    return Vec3<T>( cubicWeight( t[0] ),
                    cubicWeight( t[1] ),
                    cubicWeight( t[2] ) );
}

template <class T>
inline Vec3<T> cubicWeightGrad( const Vec3<T> &t )
{
    return Vec3<T>( cubicWeightGrad( t[0] ),
                    cubicWeightGrad( t[1] ),
                    cubicWeightGrad( t[2] ) );
}


// by not templating these inoiseGrad functions, we guarantee that this
// code will only compile for float and double.
// The mysterious "17" and "34" passed into the phi functions
// are there to make the noise match prman

inline const V3d &inoiseGrad( int ix, double *dummy )
{
    return ( const V3d & )( gradsV3d[phi(ix + phi(17 + phi(34)))] );
}

inline const V3f &inoiseGrad( int ix, float *dummy )
{
    return ( const V3f & )( gradsV3f[phi(ix + phi(17 + phi(34)))] );
}

inline const V3d &inoiseGrad( int ix, int iy, double *dummy )
{
    return ( const V3d & )( gradsV3d[phi(ix + phi(iy + phi(34)))] );
}

inline const V3f &inoiseGrad( int ix, int iy, float *dummy )
{
    return ( const V3f & )( gradsV3f[phi(ix + phi(iy + phi(34)))] );
}
 
inline const V3d &inoiseGrad( int ix, int iy, int iz, double *dummy )
{
    return ( const V3d & )( gradsV3d[phi(ix + phi(iy + phi(iz)))] );
}

inline const V3f &inoiseGrad( int ix, int iy, int iz, float *dummy )
{
    return ( const V3f & )( gradsV3f[phi(ix + phi(iy + phi(iz)))] );
}

template <class T>
inline
T dotComp( const Vec3<T> &v, T x )
{
    return v.x * x;
}

template <class T>
inline
T dotComp( const Vec3<T> &v, T x, T y )
{
    return (v.x * x) + (v.y * y);
}

template <class T>
inline
T dotComp( const Vec3<T> &v, T x, T y, T z )
{
    return (v.x * x) + (v.y * y) + (v.z * z);
}

//------------------------
// 1D noise implementation
//------------------------

template <class T>
inline T noiseCen1( T v )
{
    const int floorV = ( int ) Imath::floor( v );

    const T t = v - ( T ) floorV;

    const T tm1 = t - ((T)1.0);

    const Vec3<T> &g0 =
        inoiseGrad( floorV, (T*)NULL );
    
    const Vec3<T> &g1 = 
        inoiseGrad( floorV+1, (T*)NULL );

    const T w = cubicWeight( t );
    
    // Do the x's
    // n0x
    const T proj0 = dotComp( g0, t );
    const T proj1 = dotComp( g1, tm1 );
    return Imath::lerp( proj0, proj1, w );
}

//------------------------
// 2D noise implementation
//------------------------

template <class T>
inline T noiseCen2( const Vec2<T> &v )
{
    const V2i floorV( ( int ) Imath::floor( v[0] ),
                      ( int ) Imath::floor( v[1] ) );

    const Vec2<T> t( v[0] - ( T ) floorV[0],
                     v[1] - ( T ) floorV[1] );
    
    const Vec2<T> tm1( t[0] - ((T)1.0),
                       t[1] - ((T)1.0) );

    const Vec3<T> &g0 =
        inoiseGrad( floorV[0], floorV[1], (T*)NULL );
    
    const Vec3<T> &g1 = 
        inoiseGrad( floorV[0]+1, floorV[1], (T*)NULL );

    const Vec3<T> &g2 =
        inoiseGrad( floorV[0], floorV[1]+1, (T*)NULL );

    const Vec3<T> &g3 =
        inoiseGrad( floorV[0]+1, floorV[1]+1, (T*)NULL );

    const Vec2<T> w( cubicWeight( t ) );

    // Do the x's
    // n0x
    const T proj0 = dotComp( g0, t[0], t[1] );
    const T proj1 = dotComp( g1, tm1[0], t[1] );
    const T n0x = Imath::lerp( proj0, proj1, w[0] );
    
    // n1x
    const T proj2 = dotComp( g2, t[0], tm1[1] );
    const T proj3 = dotComp( g3, tm1[0], tm1[1] );
    const T n1x = Imath::lerp( proj2, proj3, w[0] );

    // Do the y's
    // n0y
    return Imath::lerp( n0x, n1x, w[1] );
}

//------------------------
// 3D noise implementation
//------------------------

template <class T>
inline T noiseCen3( const Vec3<T> &v )
{
    const V3i floorV( ( int ) Imath::floor( v[0] ),
                      ( int ) Imath::floor( v[1] ),
                      ( int ) Imath::floor( v[2] ) );

    const Vec3<T> t( v[0] - ( T ) floorV[0],
                     v[1] - ( T ) floorV[1],
                     v[2] - ( T ) floorV[2] );
    
    const Vec3<T> tm1( t[0] - ((T)1.0),
                       t[1] - ((T)1.0),
                       t[2] - ((T)1.0) );

    const Vec3<T> &g0 =
        inoiseGrad( floorV[0], floorV[1], floorV[2], (T*)NULL );
    
    const Vec3<T> &g1 = 
        inoiseGrad( floorV[0]+1, floorV[1], floorV[2], (T*)NULL );

    const Vec3<T> &g2 =
        inoiseGrad( floorV[0], floorV[1]+1, floorV[2], (T*)NULL );

    const Vec3<T> &g3 =
        inoiseGrad( floorV[0]+1, floorV[1]+1, floorV[2], (T*)NULL );

    const Vec3<T> &g4 =
        inoiseGrad( floorV[0], floorV[1], floorV[2]+1, (T*)NULL );
    
    const Vec3<T> &g5 =
        inoiseGrad( floorV[0]+1, floorV[1], floorV[2]+1, (T*)NULL );

    const Vec3<T> &g6 =
        inoiseGrad( floorV[0], floorV[1]+1, floorV[2]+1, (T*)NULL );

    const Vec3<T> &g7 =
        inoiseGrad( floorV[0]+1, floorV[1]+1, floorV[2]+1, (T*)NULL );

    const Vec3<T> w( cubicWeight( t ) );

    // Do the x's
    // n0x
    const T proj0 = dotComp( g0, t[0], t[1], t[2] );
    const T proj1 = dotComp( g1, tm1[0], t[1], t[2] );
    const T n0x = Imath::lerp( proj0, proj1, w[0] );
    
    // n1x
    const T proj2 = dotComp( g2, t[0], tm1[1], t[2] );
    const T proj3 = dotComp( g3, tm1[0], tm1[1], t[2] );
    const T n1x = Imath::lerp( proj2, proj3, w[0] );

    // n2x
    const T proj4 = dotComp( g4, t[0], t[1], tm1[2] );
    const T proj5 = dotComp( g5, tm1[0], t[1], tm1[2] );
    const T n2x = Imath::lerp( proj4, proj5, w[0] );

    // n3x
    const T proj6 = dotComp( g6, t[0], tm1[1], tm1[2] );
    const T proj7 = dotComp( g7, tm1[0], tm1[1], tm1[2] );
    const T n3x = Imath::lerp( proj6, proj7, w[0] );

    // Do the y's
    // n0y
    const T n0y = Imath::lerp( n0x, n1x, w[1] );

    // n1y
    const T n1y = Imath::lerp( n2x, n3x, w[1] );

    // Do the z's
    return Imath::lerp( n0y, n1y, w[2] );
}


//----------------------------------
// 1D noise with grad implementation
//----------------------------------

template <class T>
inline T noiseCenGrad1( T v,
                        T &grad )
{
    const int floorV = ( int ) Imath::floor( v );

    const T t = v - ( T ) floorV;

    const T tm1 = t - ((T)1.0);

    const Vec3<T> &g0 =
        inoiseGrad( floorV, (T*)NULL );
    
    const Vec3<T> &g1 = 
        inoiseGrad( floorV+1, (T*)NULL );

    const T w = cubicWeight( t );
    const T dw = cubicWeightGrad( t );
    
    // Do the x's
    // n0x
    const T proj0 = dotComp( g0, t );
    const T proj1 = dotComp( g1, tm1 );
    grad = Imath::lerp( g0[0], g1[0], w ) +
        dw * ( proj1 - proj0 );
    return Imath::lerp( proj0, proj1, w );
}

//----------------------------------
// 2D noise with grad implementation
//----------------------------------

template <class T>
inline T noiseCenGrad2( const Vec2<T> &v,
                        Vec2<T> &grad )
{
    const V2i floorV( ( int ) Imath::floor( v[0] ),
                      ( int ) Imath::floor( v[1] ) );

    const Vec2<T> t( v[0] - ( T ) floorV[0],
                     v[1] - ( T ) floorV[1] );
    
    const Vec2<T> tm1( t[0] - ((T)1.0),
                       t[1] - ((T)1.0) );

    const Vec3<T> &g0 =
        inoiseGrad( floorV[0], floorV[1], (T*)NULL );
    
    const Vec3<T> &g1 = 
        inoiseGrad( floorV[0]+1, floorV[1], (T*)NULL );

    const Vec3<T> &g2 =
        inoiseGrad( floorV[0], floorV[1]+1, (T*)NULL );

    const Vec3<T> &g3 =
        inoiseGrad( floorV[0]+1, floorV[1]+1, (T*)NULL );

    const Vec2<T> w( cubicWeight( t ) );
    const Vec2<T> dw( cubicWeightGrad( t ) );

    // Do the x's
    // n0x
    const T proj0 = dotComp( g0, t[0], t[1] );
    const T proj1 = dotComp( g1, tm1[0], t[1] );
    const T n0x = Imath::lerp( proj0, proj1, w[0] );
    const T d_dx_n0x = Imath::lerp( g0[0], g1[0], w[0] ) +
        dw[0] * ( proj1 - proj0 );
    const T d_dy_n0x = Imath::lerp( g0[1], g1[1], w[0] );
    
    // n1x
    const T proj2 = dotComp( g2, t[0], tm1[1] );
    const T proj3 = dotComp( g3, tm1[0], tm1[1] );
    const T n1x = Imath::lerp( proj2, proj3, w[0] );
    const T d_dx_n1x = Imath::lerp( g2[0], g3[0], w[0] ) +
        dw[0] * ( proj3 - proj2 );
    const T d_dy_n1x = Imath::lerp( g2[1], g3[1], w[0] );

    // Do the y's
    // n0y
    grad[0] = Imath::lerp( d_dx_n0x, d_dx_n1x, w[1] );
    grad[1] = Imath::lerp( d_dy_n0x, d_dy_n1x, w[1] ) +
        dw[1] * ( n1x - n0x );
    return Imath::lerp( n0x, n1x, w[1] );
}

//----------------------------------
// 3D noise with grad implementation
//----------------------------------

template <class T>
inline T noiseCenGrad3( const Vec3<T> &v,
                        Vec3<T> &grad )
{
    const V3i floorV( ( int ) Imath::floor( v[0] ),
                      ( int ) Imath::floor( v[1] ),
                      ( int ) Imath::floor( v[2] ) );

    const Vec3<T> t( v[0] - ( T ) floorV[0],
                     v[1] - ( T ) floorV[1],
                     v[2] - ( T ) floorV[2] );
    
    const Vec3<T> tm1( t[0] - ((T)1.0),
                       t[1] - ((T)1.0),
                       t[2] - ((T)1.0) );

    const Vec3<T> &g0 =
        inoiseGrad( floorV[0], floorV[1], floorV[2], (T*)NULL );
    
    const Vec3<T> &g1 = 
        inoiseGrad( floorV[0]+1, floorV[1], floorV[2], (T*)NULL );

    const Vec3<T> &g2 =
        inoiseGrad( floorV[0], floorV[1]+1, floorV[2], (T*)NULL );

    const Vec3<T> &g3 =
        inoiseGrad( floorV[0]+1, floorV[1]+1, floorV[2], (T*)NULL );

    const Vec3<T> &g4 =
        inoiseGrad( floorV[0], floorV[1], floorV[2]+1, (T*)NULL );
    
    const Vec3<T> &g5 =
        inoiseGrad( floorV[0]+1, floorV[1], floorV[2]+1, (T*)NULL );

    const Vec3<T> &g6 =
        inoiseGrad( floorV[0], floorV[1]+1, floorV[2]+1, (T*)NULL );

    const Vec3<T> &g7 =
        inoiseGrad( floorV[0]+1, floorV[1]+1, floorV[2]+1, (T*)NULL );

    const Vec3<T> w( cubicWeight( t ) );
    const Vec3<T> dw( cubicWeightGrad( t ) );

    // Do the x's
    // n0x
    const T proj0 = dotComp( g0, t[0], t[1], t[2] );
    const T proj1 = dotComp( g1, tm1[0], t[1], t[2] );
    const T n0x = Imath::lerp( proj0, proj1, w[0] );
    const T d_dx_n0x = Imath::lerp( g0[0], g1[0], w[0] ) +
        dw[0] * ( proj1 - proj0 );
    const T d_dy_n0x = Imath::lerp( g0[1], g1[1], w[0] );
    const T d_dz_n0x = Imath::lerp( g0[2], g1[2], w[0] );
    
    // n1x
    const T proj2 = dotComp( g2, t[0], tm1[1], t[2] );
    const T proj3 = dotComp( g3, tm1[0], tm1[1], t[2] );
    const T n1x = Imath::lerp( proj2, proj3, w[0] );
    const T d_dx_n1x = Imath::lerp( g2[0], g3[0], w[0] ) +
        dw[0] * ( proj3 - proj2 );
    const T d_dy_n1x = Imath::lerp( g2[1], g3[1], w[0] );
    const T d_dz_n1x = Imath::lerp( g2[2], g3[2], w[0] );

    // n2x
    const T proj4 = dotComp( g4, t[0], t[1], tm1[2] );
    const T proj5 = dotComp( g5, tm1[0], t[1], tm1[2] );
    const T n2x = Imath::lerp( proj4, proj5, w[0] );
    const T d_dx_n2x = Imath::lerp( g4[0], g5[0], w[0] ) +
        dw[0] * ( proj5 - proj4 );
    const T d_dy_n2x = Imath::lerp( g4[1], g5[1], w[0] );
    const T d_dz_n2x = Imath::lerp( g4[2], g5[2], w[0] );

    // n3x
    const T proj6 = dotComp( g6, t[0], tm1[1], tm1[2] );
    const T proj7 = dotComp( g7, tm1[0], tm1[1], tm1[2] );
    const T n3x = Imath::lerp( proj6, proj7, w[0] );
    const T d_dx_n3x = Imath::lerp( g6[0], g7[0], w[0] ) +
        dw[0] * ( proj7 - proj6 );
    const T d_dy_n3x = Imath::lerp( g6[1], g7[1], w[0] );
    const T d_dz_n3x = Imath::lerp( g6[2], g7[2], w[0] );

    // Do the y's
    // n0y
    const T n0y = Imath::lerp( n0x, n1x, w[1] );
    const T d_dx_n0y = Imath::lerp( d_dx_n0x, d_dx_n1x, w[1] );
    const T d_dy_n0y = Imath::lerp( d_dy_n0x, d_dy_n1x, w[1] ) +
        dw[1] * ( n1x - n0x );
    const T d_dz_n0y = Imath::lerp( d_dz_n0x, d_dz_n1x, w[1] );

    // n1y
    const T n1y = Imath::lerp( n2x, n3x, w[1] );
    const T d_dx_n1y = Imath::lerp( d_dx_n2x, d_dx_n3x, w[1] );
    const T d_dy_n1y = Imath::lerp( d_dy_n2x, d_dy_n3x, w[1] ) +
        dw[1] * ( n3x - n2x );
    const T d_dz_n1y = Imath::lerp( d_dz_n2x, d_dz_n3x, w[1] );

    // Do the z's
    grad[0] = Imath::lerp( d_dx_n0y, d_dx_n1y, w[2] );
    grad[1] = Imath::lerp( d_dy_n0y, d_dy_n1y, w[2] );
    grad[2] = Imath::lerp( d_dz_n0y, d_dz_n1y, w[2] ) +
        dw[2] * ( n1y - n0y );
    return Imath::lerp( n0y, n1y, w[2] );
}
 
} // End namespace


float noiseCen( float v )
{
    return noiseCen1( v );
}

double noiseCen( double v )
{
    return noiseCen1( v );
}

float noiseCen( const V2f &v )
{
    return noiseCen2( v );
}

double noiseCen( const V2d &v )
{
    return noiseCen2( v );
}

float noiseCen( const V3f &v )
{
    return noiseCen3( v );
}

double noiseCen( const V3d &v )
{
    return noiseCen3( v );
}

float noiseCenGrad( float v,
                    float &grad )
{
    return noiseCenGrad1( v, grad );
}

double noiseCenGrad( double v,
                     double &grad )
{
    return noiseCenGrad1( v, grad );
}

float noiseCenGrad( const V2f &v,
                    V2f &grad )
{
    return noiseCenGrad2( v, grad );
}

double noiseCenGrad( const V2d &v,
                     V2d &grad )
{
    return noiseCenGrad2( v, grad );
}

float noiseCenGrad( const V3f &v,
                    V3f &grad )
{
    return noiseCenGrad3( v, grad );
}

double noiseCenGrad( const V3d &v,
                     V3d &grad )
{
    return noiseCenGrad3( v, grad );
}

} // End namespace Imath
