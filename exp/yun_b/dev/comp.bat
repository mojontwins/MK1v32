@echo off
rem Yun ZX, second load (NES levels 4-6) - build script (Windows). comp.sh does the same.
rem
rem Needs: MK1's z88dk (env\setenv.bat) and the MK1 tools in ..\utils. mapcnv,
rem ene2h and sprcnv2 must be the versions with the `bin` option (src\utils).
rem
rem   comp.bat            convert resources + compile + tape
rem   comp.bat justcompile

set game=yun
rem This load: screens of the largest map (the level area, yun_level.h), fixed
rem sprite cells; below, one "call :level" per level: number, map width and
rem height, lock tile (99 = no locks)
set SCREENS=24
set CELLS=32
cd /d %~dp0
for /f "tokens=3" %%a in ('findstr /b /c:"#define MAX_LOCKS" yun_level.h') do set LOCKS=%%a

if [%1]==[justcompile] goto :compile

echo Converting resources
rem Charset + tile maps: tileset.bin, tsx_tmaps.bin, lvlN_tmaps.bin (gfx\speccy\tiles.spt)
..\utils\mkts_om.exe platform=zx mode=scripted in=..\gfx\speccy\tiles.spt > nul
rem Fixed sprite cells, in the binary
..\utils\sprcnv2.exe ..\gfx\speccy\sprites.png speccy\sprites.h %CELLS% nomask > nul
rem Levels: lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), lvlN_spr.zx0
call :level 0 4 6 15
call :level 1 12 2 15
call :level 2 6 4 15
rem Screens
for %%s in (title marco ending) do (
	..\utils\png2scr.exe ..\gfx\speccy\%%s.png %%s.scr > nul
	if exist %%sc.bin del %%sc.bin
	..\utils\zx0.exe %%s.scr %%sc.bin > nul
	del %%s.scr
)
..\utils\png2scr.exe ..\gfx\speccy\loading.png loading.bin > nul

:compile
echo Compiling
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin churromain.c -DSPECCY || goto :error
..\utils\printsize.exe %game%.bin
..\utils\bas2tap.exe -q -e -a10 -s"%game%" system/loader.bas %game%.tap > nul
..\utils\bin2tap.exe -o %game%.tap -a 32768 -append loading.bin > nul
..\utils\bin2tap.exe -o %game%.tap -a 24200 -append %game%.bin > nul
echo Output: %game%.tap (the binary must end below SPR_VAR_BASE = 59376, yun_level.h)
goto :end

:level
..\utils\mapcnv.exe ..\map\level%1.map lvl%1_map.bin %2 %3 15 10 %4 packed bin maxlocks=%LOCKS% screens=%SCREENS% > nul
..\utils\ene2h.exe ..\enems\level%1.ene lvl%1_ene.h bin=lvl%1_ene.bin screens=%SCREENS% > nul
copy /b lvl%1_map.bin + lvl%1_ene.bin + lvl%1_tmaps.bin lvl%1_area.bin > nul
..\utils\sprcnv2.exe ..\gfx\speccy\sprites%1.png lvl%1_spr.bin 12 nomask bin > nul
if exist lvl%1_area.zx0 del lvl%1_area.zx0
if exist lvl%1_spr.zx0 del lvl%1_spr.zx0
..\utils\zx0.exe lvl%1_area.bin lvl%1_area.zx0 > nul
..\utils\zx0.exe lvl%1_spr.bin lvl%1_spr.zx0 > nul
del lvl%1_map.bin lvl%1_ene.bin lvl%1_ene.h lvl%1_area.bin lvl%1_spr.bin
goto :eof

:error
echo ERROR
exit /b 1

:end
