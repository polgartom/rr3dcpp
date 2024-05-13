@echo off 

mkdir .\build
pushd .\build

cl -FAsc -Zi ..\main.cpp ..\tgaimage.cpp /std:c++20 /EHsc /link user32.lib Gdi32.lib D3d9.lib

popd