@echo off
rem =========================================================================
rem  Build the cpcrslib-mt Amstrad CPC demo.
rem
rem  Needs the z88dk (zcc) in PATH and the tools in ..\..\src\utils.
rem =========================================================================

set Z88DK_PATH=c:\z88dk10-orig
set PATH=%Z88DK_PATH%\bin;%PATH%
set Z80_OZFILES=%Z88DK_PATH%\Lib\
set ZCCCFG=%Z88DK_PATH%\Lib\Config\

set name=demo

zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %name%.bin tilemap_conf.asm main.c -DCPC -DMODE_1 > nul
if errorlevel 1 goto :error

..\..\src\utils\cpctbin2sna.exe %name%.bin 0x400 -pc 0x400 -o %name%.sna

echo Output: %name%.sna
goto :end

:error
echo ERROR

:end
