@echo off

@REM "x64"
@REM "args": ["/K", "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat", "x64"],
@REM "x86"
@REM "args": ["/K", "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat", "x86"],

set commonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Oi -Od -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FeApp -Z7 -Fmwin32_handmade.map

set commonLinkerFlags=-opt:ref User32.lib Gdi32.lib winmm.lib

if not exist .\build ( mkdir build) 
pushd build
@REM "x86"
@REM cl %commonCompilerFlags ..\win32_handmade.cpp /link -subsystem:windows,5.1 %commonLinkerFlags
@REM "x64"
cl %commonCompilerFlags% ..\win32_handmade.cpp /link  %commonLinkerFlags%
popd