@ECHO OFF

REM This is for normal build
REM for uniti builds, this have to be commented

REM get a list of all the .cpp files
REM SET cFileNames=
REM FOR /R %%f in (*.cpp) do (
REM     SET cFileNames=!cFileNames! %%f
REM )

SET compilerFLags= -Od -nologo -Gm- -GR- -Oi -WX -W3 -wd4530 -wd4201 -wd4100 -wd4189 -wd4505 -wd4101 -Zi
SET includeFlags=
SET linkerFlags= -incremental:no User32.lib Gdi32.lib Winmm.lib
SET defines=-D_DEBUG

PUSHD ..\build
cl %compilerFLags% ..\src\win32_platform.cpp -Fewin32_game %defines% %includeFlags% /link %linkerFlags%
cl %compilerFLags% -LD ..\src\game.cpp -Fegame %defines% %includeFlags% /link %linkerFlags%
POPD
