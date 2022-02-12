#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

using namespace std;

void
splitVolumeSample (
    float  a,
    float  c, // Opacity and color of original sample
    float  zf,
    float  zb, // Front and back of original sample
    float  z,  // Position of split
    float& af,
    float& cf, // Opacity and color of part closer than z
    float& ab,
    float& cb) // Opacity and color of part further away than z
{
    //
    // Given a volume sample whose front and back are at depths zf and
    // zb respectively, split the sample at depth z. Return the opacities
    // and colors of the two parts that result from the split.
    //
    // The code below is written to avoid excessive rounding errors when
    // the opacity of the original sample is very small:
    //
    // The straightforward computation of the opacity of either part
    // requires evaluating an expression of the form
    //
    // 1 - pow (1-a, x).
    //
    // However, if a is very small, then 1-a evaluates to 1.0 exactly,
    // and the entire expression evaluates to 0.0.
    //
    // We can avoid this by rewriting the expression as
    //
    // 1 - exp (x \* log (1-a)),
    //
    // and replacing the call to log() with a call to the function log1p(),
    // which computes the logarithm of 1+x without attempting to evaluate
    // the expression 1+x when x is very small.
    //
    // Now we have
    //
    // 1 - exp (x \* log1p (-a)).
    //
    // However, if a is very small then the call to exp() returns 1.0, and
    // the overall expression still evaluates to 0.0. We can avoid that
    // by replacing the call to exp() with a call to expm1():
    //
    // -expm1 (x \* log1p (-a))
    //
    // expm1(x) computes exp(x) - 1 in such a way that the result is
    // even if x is very small.
    //

    assert (zb > zf && z >= zf && z <= zb);

    a = max (0.0f, min (a, 1.0f));

    if (a == 1)
    {
        af = ab = 1;
        cf = cb = c;
    }
    else
    {
        float xf = (z - zf) / (zb - zf);
        float xb = (zb - z) / (zb - zf);

        if (a > numeric_limits<float>::min ())
        {
            af = -expm1 (xf * log1p (-a));
            cf = (af / a) * c;

            ab = -expm1 (xb * log1p (-a));
            cb = (ab / a) * c;
        }
        else
        {
            af = a * xf;
            cf = c * xf;

            ab = a * xb;
            cb = c * xb;
        }
    }
}
