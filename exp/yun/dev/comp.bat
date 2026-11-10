@echo off
rem Yun ZX - build script (Windows). comp.sh does the same.
rem
rem Needs: MK1's z88dk (env\setenv.bat) and the MK1 tools in ..\utils. mapcnv,
rem ene2h and sprcnv2 must be the versions with the `bin` option (src\utils).
rem
rem   comp.bat [1|2]              convert resources + compile + tape of that load (1)
rem   comp.bat [1|2] justcompile  (justcompile: with the resources last converted)
rem   set YUNFLAGS=-DYUN_DEBUG_LEVEL=2 then comp.bat 1 justcompile   (start at the load's level 3)

setlocal enabledelayedexpansion
set game=yun
set YUN_LOAD=1
set JC=%1
if "%1"=="1" (set YUN_LOAD=1& set JC=%2)
if "%1"=="2" (set YUN_LOAD=2& set JC=%2)
rem Each load: screens of the largest map and locks (the level area: SCREENS =
rem MAP_W * MAP_H and MAX_LOCKS in yun_level.h); below, one "call :level" per
rem level: number, map width and height, lock tile (99 = no locks). Fixed sprite cells.
if "%YUN_LOAD%"=="1" (set SCREENS=24& set LOCKS=2) else (set SCREENS=27& set LOCKS=4)
set CELLS=32
cd /d %~dp0
set BADLEVEL=

if [%JC%]==[justcompile] goto :compile

echo Converting resources
rem Charset + tile maps: tileset.bin, tsx_tmaps.bin, lvlN_tmaps.bin (gfx\speccy\tilesN.spt)
..\utils\mkts_om.exe platform=zx mode=scripted in=..\gfx\speccy\tiles%YUN_LOAD%.spt > nul
rem Fixed sprite cells from cell 8 (0-7 were Yun's 16x16 frames), and 16x24 Yun
rem (sprcnv2 with from=, label= and tall: utils\sprcnv2.bas)
..\utils\sprcnv2.exe ..\gfx\speccy\sprites.png speccy\sprites.h %CELLS% nomask from=8 > nul
..\utils\sprcnv2.exe ..\gfx\speccy\player.png speccy\player.h 8 nomask tall label=player > nul
rem (an old sprcnv2 ignores tall, from= and label=: stop here, not with a compiler error)
findstr /c:"_player_1_a" speccy\player.h > nul || goto :oldsprcnv
findstr /c:"_sprite_1_a" speccy\sprites.h > nul && goto :oldsprcnv
rem Levels: lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), lvlN_spr.zx0
if "%YUN_LOAD%"=="2" goto :levels2
call :level 0 4 6 15
call :level 1 4 6 15
call :level 2 4 6 15
call :level 3 4 6 15
goto :screens
if defined BADLEVEL goto :oldsprcnv
echo ERROR: sprcnv2.exe is an old version (no tall / from= / label=): rebuild it from utils\sprcnv2.bas
exit /b 1

:error
:levels2
call :level 4 12 2 15
call :level 5 6 4 15
call :level 6 6 4 15
call :level 7 9 3 99
:screens
rem Screens (DIRECT_TO_PLAY: the title is the frame; each load has its ending)
..\utils\png2scr.exe ..\gfx\speccy\title.png title.scr > nul
if exist titlec.bin del titlec.bin
..\utils\zx0.exe title.scr titlec.bin > nul
..\utils\png2scr.exe ..\gfx\speccy\ending%YUN_LOAD%.png ending.scr > nul
if exist endingc.bin del endingc.bin
..\utils\zx0.exe ending.scr endingc.bin > nul
del title.scr ending.scr
..\utils\png2scr.exe ..\gfx\speccy\loading.png loading.bin > nul

:compile
echo Compiling
zcc +zx -m -vn -unsigned -zorg=24200 -llib/splib2a -o %game%.bin churromain.c -DSPECCY -DYUN_LOAD=%YUN_LOAD% %YUNFLAGS% || goto :error
..\utils\printsize.exe %game%.bin
if exist %game%%YUN_LOAD%.tap del %game%%YUN_LOAD%.tap
..\utils\bas2tap.exe -q -e -a10 -s"%game%" system/loader.bas %game%%YUN_LOAD%.tap > nul
..\utils\bin2tap.exe -o %game%%YUN_LOAD%.tap -a 32768 -append loading.bin > nul
..\utils\bin2tap.exe -o %game%%YUN_LOAD%.tap -a 24200 -append %game%.bin > nul
rem The binary must end below SPR_VAR_BASE (yun_level.h, the ZX value is the last one)
for /f "tokens=3" %%a in ('findstr /c:"#define SPR_VAR_BASE" yun_level.h') do set TOP=%%a
for %%I in (%game%.bin) do set SIZE=%%~zI
set /a END=24200+SIZE
echo Output: %game%%YUN_LOAD%.tap (%SIZE% bytes, ends at %END%; level sprites at %TOP%)
if %END% GTR %TOP% (
	echo ERROR: the binary runs into the level sprites
	exit /b 1
)
goto :end

:level
..\utils\mapcnv.exe ..\map\level%1.map lvl%1_map.bin %2 %3 15 10 %4 packed bin maxlocks=%LOCKS% screens=%SCREENS% > nul
..\utils\ene2h.exe ..\enems\level%1.ene lvl%1_ene.h bin=lvl%1_ene.bin screens=%SCREENS% > nul
copy /b lvl%1_map.bin + lvl%1_ene.bin + lvl%1_tmaps.bin lvl%1_area.bin > nul
rem The level area must come out exactly SCREENS * 105 + LOCKS * 4 + 128 bytes
rem (old mapcnv / ene2h, without `bin`, write C text here: the game would hang)
for %%I in (lvl%1_area.bin) do set ASIZE=%%~zI
set /a AWANT=SCREENS*105+LOCKS*4+128
if not "%ASIZE%"=="%AWANT%" (
	echo ERROR: lvl%1_area.bin is %ASIZE% bytes, should be %AWANT%: are mapcnv.exe and ene2h.exe the versions with "bin" ^(utils\*.bas^)?
	set BADLEVEL=1
)
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
