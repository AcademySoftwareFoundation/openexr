#include <testSize.h>
#include <half.h>
#include <iostream>
#include <assert.h>


using namespace std;


void
testSize ()
{
    cout << "size and alignment\n";

    half h[2];

    int size = sizeof (half);
    int algn = (char *)&h[1] - (char *)&h[0];

    cout << "sizeof  (half) = " << size << endl;
    cout << "alignof (half) = " << algn << endl;

    assert (size == 2 && algn == 2);

    cout << "ok\n\n" << flush;
}
