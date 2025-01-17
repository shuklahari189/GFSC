@echo off

set commonCompilerFlags= -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4459 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -FC -Z7 
set commonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib Winmm.lib

if not EXIST build (mkdir build)
pushd build
@REM x64
del *.pdb > NUL 2>NUL
cl %commonCompilerFlags% ..\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no -PDB:handmade_%random%%random%.pdb -EXPORT:gameGetSoundSamples -EXPORT:gameUpdateAndRender
cl %commonCompilerFlags% ..\code\win32Handmade.cpp -Fmwin32Handmade.map /link %commonLinkerFlags%  
@REM x86
@REM cl %commonCompilerFlags% ..\win32Handmade.cpp /link -subsystem:windows,5.1 %commonLinkerFlags%  
popd