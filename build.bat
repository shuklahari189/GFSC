@echo off

@REM -Zi = /Zi
if not EXIST build (
    mkdir build
    pushd build
    @REM setting stack size with -FSize
    @REM cl -Zi -FAsc ../main.cpp /Feapp User32.lib Gdi32.lib -F4194304
    cl -Zi -FAsc ../main.cpp /Feapp User32.lib Gdi32.lib 
    popd
) else (
pushd build
@REM cl /Zi -FAsc ../main.cpp /Feapp User32.lib Gdi32.lib -F4194304
cl /Zi -FAsc ../main.cpp /Feapp User32.lib Gdi32.lib 
popd 
)
