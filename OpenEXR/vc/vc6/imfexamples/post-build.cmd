set srcdir=..\..\..\IlmImfExamples
cd %srcdir%
set instdir=..\vc\vc6\examples
mkdir %instdir%
copy drawImage.cpp %instdir%
copy generalInterfaceExamples.cpp %instdir%
copy main.cpp %instdir%
copy rgbaInterfaceExamples.cpp %instdir%
copy drawImage.h %instdir%
copy generalInterfaceExamples.h %instdir%
copy rgbaInterfaceExamples.h %instdir%
cd ..\vc\vc6
copy imfexamples\imfexamples.dsp examples\imfexamples.dsp
