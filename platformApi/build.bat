@echo off

@REM -Zi = /Zi
if not EXIST build (
    mkdir build
    pushd build
    @REM setting stack size with -FSize
    @REM cl -Zi -FAsc ../main.cpp User32.lib Gdi32.lib -F4194304
    @REM defining HANDMADE_WIN32 = 1 with -DNAME
    @REM cl -DHANDMADE_WIN32=1 -Zi -FAsc ../win32_handmade.cpp User32.lib Gdi32.lib 
    cl /Zi -FAsc ../win32_handmade.cpp User32.lib Gdi32.lib 
    popd
) else (
pushd build
cl /Zi -FAsc ../win32_handmade.cpp User32.lib Gdi32.lib 
popd 
)
