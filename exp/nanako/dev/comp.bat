@echo off
rem Nanako in Classic Japanese Monster Castle - ZX Spectrum 48K / 128K build (Windows). comp.sh does the same.
rem
rem   comp.bat               convert resources + compile + tape
rem   comp.bat justcompile

set game=nanako
cd /d %~dp0

if [%1]==[justcompile] goto :compile

echo Converting resources
..\utils\ene2h.exe ..\enems\enems.ene enems.h 2bytes > nul
..\utils\sfxcnv.exe in=..\sfx\sfx.txt out=sfx.h > nul
rem 006: default ink yellow for one colour tiles (ts2bin ignores it if shorter than 3 chars)
..\utils\ts2bin.exe ..\gfx\speccy\font.png ..\gfx\speccy\work.png tileset.bin 006 > nul
..\utils\sprcnv2.exe ..\gfx\speccy\sprites.png speccy\sprites.h 19 > nul
for %%s in (title marco ending) do (
	..\utils\png2scr.exe ..\gfx\speccy\%%s.png ..\gfx\speccy\%%s.scr > nul
	if exist %%sc.bin del %%sc.bin
	..\utils\zx0.exe ..\gfx\speccy\%%s.scr %%sc.bin > nul
)
..\utils\png2scr.exe ..\gfx\speccy\loading.png loading.bin > nul
del ..\gfx\speccy\*.scr > nul 2> nul

:compile
echo Compiling
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin churromain.c -DSPECCY || goto :error
..\utils\printsize.exe %game%.bin

rem 128K: the WYZ player, songs and effects for RAM1 (C000)
pushd ..\ogt\ram1
z80asm -b ram1.asm || (popd & goto :error)
del ram1.obj ram1.err 2> nul
popd

rem Tape: BASIC, system\zxloader.asm (at 24000), then headerless: screen, game
rem and RAM1 (only loaded on a 128K). Same blocks as comp.sh's gentape.py.
for %%I in (%game%.bin) do set MAIN=%%~zI
for %%I in (..\ogt\ram1\ram1.bin) do set RAM1=%%~zI
(echo 	defc MAIN_SIZE = %MAIN%& echo 	defc RAM1_SIZE = %RAM1%) > system\zxsizes.inc
pushd system
z80asm -b zxloader.asm || (popd & goto :error)
del zxloader.obj zxloader.err 2> nul
popd
..\utils\GenTape.exe %game%.tap ^
	basic '%game%' 10 system\loader128.tok ^
	hdata 'loader' 5DC0 system\zxloader.bin ^
	data loading.bin ^
	data %game%.bin ^
	data ..\ogt\ram1\ram1.bin > nul
echo Output: %game%.tap (48K / 128K)
goto :end

:error
echo ERROR
exit /b 1

:end
