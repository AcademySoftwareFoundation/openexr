set srcdir=..\..\..\Half
cd %srcdir%
set instdir=..\vc\vc7\include
mkdir %instdir%
copy half.h %instdir%
copy halfFunction.h %instdir%
copy halfLimits.h %instdir%
