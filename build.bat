@echo off
@REM "args": ["/K", "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat", "x64"],
if not exist .\build ( mkdir build) 
pushd build
cl -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 ..\win32_handmade.cpp -Zi -FeApp User32.lib Gdi32.lib
popd