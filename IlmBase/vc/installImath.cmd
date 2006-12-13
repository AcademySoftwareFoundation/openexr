@echo off
set src=..\..\..\..\Imath
cd %src%
set instpath=..\..\..\Deploy\include
mkdir %instpath%
copy *.h %instpath%

