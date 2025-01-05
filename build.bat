@echo off

set commonCompilerFlags= -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4459 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -FC -Z7 -Fmwin32Handmade.map 
set commonLinkerFlags= -opt:ref user32.lib gdi32.lib Winmm.lib

if not EXIST build (mkdir build)
pushd build
@REM x64
cl %commonCompilerFlags% ..\win32Handmade.cpp /link %commonLinkerFlags%  
@REM x86
@REM cl %commonCompilerFlags% ..\win32Handmade.cpp /link -subsystem:windows,5.1 %commonLinkerFlags%  
popd