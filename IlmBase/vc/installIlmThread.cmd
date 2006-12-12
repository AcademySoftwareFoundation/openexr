@echo off
set src=..\..\..\..\IlmThread
cd %src%
set instpath=..\..\Deploy\include
mkdir %instpath%
copy *.h %instpath%

copy ..\config.windows\*.h %instpath%
