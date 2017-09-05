@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir c:\src\handmade\build
pushd c:\src\handmade\build
cl -Zi c:\src\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd