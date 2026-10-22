@echo off
rem =========================================================================
rem  Build the splib2 ZX Spectrum demo.
rem
rem  Needs the z88dk (zcc) in PATH and the tools in ..\..\src\utils.
rem =========================================================================

set Z88DK_PATH=c:\z88dk10-orig
set PATH=%Z88DK_PATH%\bin;%PATH%
set Z80_OZFILES=%Z88DK_PATH%\Lib\
set ZCCCFG=%Z88DK_PATH%\Lib\Config\

set name=demo

zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %name%.bin main.c > nul
if errorlevel 1 goto :error

rem BASIC loader + code at 24200
..\..\src\utils\bas2tap.exe -q -e -a10 -s"%name%" loader.bas %name%.tap
..\..\src\utils\bin2tap.exe -o %name%.tap -a 24200 -append %name%.bin

echo Output: %name%.tap
goto :end

:error
echo ERROR

:end
