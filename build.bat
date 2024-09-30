@echo off

@REM -Zi = /Zi
if not EXIST build (
    mkdir build
    pushd build
    cl -Zi ../main.cpp /Feapp User32.lib
    popd 
) else (
pushd build
cl /Zi ../main.cpp /Feapp User32.lib
popd 
)
