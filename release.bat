@echo off 

mkdir .\build
pushd .\build

cl -Zi -O2 ..\main.cpp ..\tgaimage.cpp /std:c++20 /EHsc /link user32.lib Gdi32.lib D3d9.lib

popd .\build