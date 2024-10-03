@echo off
if not Exist .\build (
    mkdir build
    pushd build
    cl -Zi ..\main.cpp User32.lib Gdi32.lib
    popd build
) else (
    pushd build
    cl -Zi ..\main.cpp User32.lib Gdi32.lib
    popd build
)