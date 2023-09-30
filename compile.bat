@echo off 

mkdir .\build
pushd .\build

cl -Zi ..\main.cpp /std:c++17 /link user32.lib Gdi32.lib D3d9.lib

popd .\build