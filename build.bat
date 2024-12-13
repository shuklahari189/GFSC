@echo off
if not exist .\build (
    mkdir build
    pushd build
    cl ..\main.cpp -Zi /FeApp User32.lib Gdi32.lib
    popd
) else (
    pushd build
    cl ..\main.cpp -Zi /FeApp User32.lib Gdi32.lib
    popd
)