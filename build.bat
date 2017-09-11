@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir "c:\src\from scratch\build"
pushd "c:\src\from scratch\build"
cl -Zi "c:\src\from scratch\code\win32_handmade.cpp" user32.lib gdi32.lib
popd