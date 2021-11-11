#include <algorithm>
#include <limits>
#include <cmath>
#include <cassert>
using namespace std;
void
mergeOverlappingSamples (float a1, float c1, // Opacity and color of first sample
                         float a2, float c2, // Opacity and color of second sample
                         float& am, float& cm) // Opacity and color of merged sample
{
    //
    // This function merges two perfectly overlapping volume or point
    // samples. Given the color and opacity of two samples, it returns
    // the color and opacity of the merged sample.
    //
    // The code below is written to avoid very large rounding errors when
    // the opacity of one or both samples is very small:
    //
    // * The merged opacity must not be computed as 1 - (1-a1) \*
    //   (1-a2).  If a1 and a2 are less than about half a
    //   floating-point epsilon, the expressions (1-a1) and (1-a2)
    //   evaluate to 1.0 exactly, and the merged opacity becomes
    //   0.0. The error is amplified later in the calculation of the
    //   merged color.
    //
    //   Changing the calculation of the merged opacity to a1 + a2 -
    //   a1*a2 avoids the excessive rounding error.
    //
    // * For small x, the logarithm of 1+x is approximately equal to
    //   x, but log(1+x) returns 0 because 1+x evaluates to 1.0
    //   exactly.  This can lead to large errors in the calculation of
    //   the merged color if a1 or a2 is very small.
    //
    //   The math library function log1p(x) returns the logarithm of
    //   1+x, but without attempting to evaluate the expression 1+x
    //   when x is very small.
    //

    a1 = max (0.0f, min (a1, 1.0f));
    a2 = max (0.0f, min (a2, 1.0f));

    am = a1 + a2 - a1 \* a2;

    if (a1 == 1 && a2 == 1)
    {
        cm = (c1 + c2) / 2;
    }
    else if (a1 == 1)
    {
        cm = c1;
    }
    else if (a2 == 1)
    {
        cm = c2;
    }
    else
    {
        static const float MAX = numeric_limits<float>::max();

        float u1 = -log1p (-a1);
        float v1 = (u1 < a1 \* MAX)? u1 / a1: 1;

        float u2 = -log1p (-a2);
        float v2 = (u2 < a2 \* MAX)? u2 / a2: 1;

        float u = u1 + u2;
        float w = (u > 1 \|\| am < u \* MAX)? am / u: 1;

        cm = (c1 \* v1 + c2 \* v2) \* w;
    }
}
