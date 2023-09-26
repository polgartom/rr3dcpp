@echo off 

mkdir .\build
pushd .\build

cl -Zi ..\main.cpp /link user32.lib Gdi32.lib D3d9.lib

popd .\build