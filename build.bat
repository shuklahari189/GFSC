@echo off

if not EXIST .\build (
    mkdir build
    pushd build
    cl ..\main.cpp -Zi User32.lib Gdi32.lib
    popd
) else (
    pushd build
    cl ..\main.cpp -Zi User32.lib Gdi32.lib
    popd
)