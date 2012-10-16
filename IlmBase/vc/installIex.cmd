@echo off
set deploypath=..\..\..\..\..\..\Deploy
set src=..\..\..\..\Iex

if not exist %deploypath% mkdir %deploypath%

set intdir=%1%
if %intdir%=="" set intdir=Release
echo Installing into %intdir%
set instpath=%deploypath%\lib\%intdir%
if not exist %instpath% mkdir %instpath%
echo copying ..\%intdir%\Iex.lib %instpath%
copy ..\%intdir%\Iex.lib %instpath%
echo copying ..\%intdir%\Iex.exp %instpath%
copy ..\%intdir%\Iex.exp %instpath%

set instpath=%deploypath%\bin\%intdir%
if not exist %instpath% mkdir %instpath%
echo copying ..\%intdir%\Iex.dll %instpath%
copy ..\%intdir%\Iex.dll %instpath%

cd %src%
set instpath=..\..\..\Deploy\include
mkdir %instpath%
echo copying headers to %instpath%
copy *.h %instpath%

