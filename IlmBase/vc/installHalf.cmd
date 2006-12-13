@echo off
set src=..\..\..\..\Half
cd %src%
set deploypath=..\..\..\Deploy
mkdir %deploypath%
set instpath=..\..\..\Deploy\include
mkdir %instpath%

copy half.h %instpath%
copy halfFunction.h %instpath%
copy halfLimits.h %instpath%

mkdir %deploypath%\vc7\bin

if exist ..\vc\vc7\IlmBase\release\createDLL.exe goto l1
copy ..\vc\vc7\IlmBase\debug\createDLL.exe %deploypath%\vc7\bin
goto l2
:l1
copy ..\vc\vc7\IlmBase\release\createDLL.exe %deploypath%\vc7\bin
:l2
