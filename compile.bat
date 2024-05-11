@echo off 

mkdir .\build
pushd .\build

cl /std:c++20 -FAsc -Zi ..\main.cpp ..\tgaimage.cpp /EHsc /link user32.lib Gdi32.lib D3d9.lib

popd