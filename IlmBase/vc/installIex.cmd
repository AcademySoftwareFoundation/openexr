@echo off
set src=..\..\..\..\Iex
cd %src%
set instpath=..\..\..\Deploy\include
mkdir %instpath%
copy *.h %instpath%

