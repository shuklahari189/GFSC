@echo off

if not EXIST .\build (
    mkdir build
    pushd build
    cl ..\win32_handmade.cpp -Zi User32.lib Gdi32.lib -DHANDMADE_WIN32=1
    popd
) else (
    pushd build
    cl ..\win32_handmade.cpp -Zi User32.lib Gdi32.lib -DHANDMADE_WIN32=1
    popd
)