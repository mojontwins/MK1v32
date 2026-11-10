@echo off
rem Yun CPC - build script (Windows). comp_cpc.sh does the same.
rem MK1 with the CPC_DIRTY backend (no superbuffer), mode 0, split palette.
rem
rem Needs: MK1's z88dk (env\setenv.bat) and the MK1 tools in ..\utils. mapcnv
rem and ene2h must be the versions with the `bin` option (src\utils).
rem
rem   comp_cpc.bat [1|2]              convert resources + compile + snapshot of that load (1)
rem   comp_cpc.bat [1|2] justcompile  (justcompile: with the resources last converted)
rem   set YUNFLAGS=-DYUN_DEBUG_LEVEL=2 then comp_cpc.bat 1 justcompile   (start at the load's level 3)

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
rem Patterns + tile maps: cpc\patterns.bin, cpc\tsx_tmaps.bin, cpc\lvlN_tmaps.bin (gfx\cpc\tilesN.spt)
..\utils\mkts_om.exe platform=cpc mode=scripted in=..\gfx\cpc\tiles%YUN_LOAD%.spt > nul
if exist cpc\patterns.zx0 del cpc\patterns.zx0
..\utils\zx0.exe cpc\patterns.bin cpc\patterns.zx0 > nul
rem Fixed sprite cells, in the binary
..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\cpc\pal_cnvspr.png mode=sprites in=..\gfx\cpc\sprites.png out=cpc\sprites.bin max=%CELLS% silent > nul
rem Palettes: palN.png (title and ending: my_inks), per level background inks, sprite inks
..\utils\mkts_om.exe platform=cpc mode=palsasassembly in=..\gfx\cpc\pal%YUN_LOAD%.png prefix=my_inks out=cpc\pal.h silent > nul
..\utils\mkts_om.exe platform=cpc mode=pals in=..\gfx\cpc\pal_cnvspr.png prefix=spr_pal out=cpc\spr_pal.h silent > nul
if exist ..\gfx\cpc\pal_dark.png ..\utils\mkts_om.exe platform=cpc mode=pals in=..\gfx\cpc\pal_dark.png prefix=dark_pal out=cpc\dark_pal.h silent > nul
if exist ..\gfx\cpc\pal_light.png ..\utils\mkts_om.exe platform=cpc mode=pals in=..\gfx\cpc\pal_light.png prefix=light_pal out=cpc\light_pal.h silent > nul
rem Levels: cpc\lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), cpc\lvlN_spr.zx0
if "%YUN_LOAD%"=="2" goto :levels2
call :level 0 4 6 15
call :level 1 4 6 15
call :level 2 4 6 15
call :level 3 4 6 15
goto :screens
if defined BADLEVEL goto :error
:levels2
call :level 4 12 2 15
call :level 5 6 4 15
call :level 6 6 4 15
call :level 7 9 3 99
:screens
rem Screens (DIRECT_TO_PLAY: the title is the frame; each load has its ending)
call :screen title title
call :screen ending ending%YUN_LOAD%

:compile
echo Compiling
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%c.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY -DYUN_LOAD=%YUN_LOAD% %YUNFLAGS% || goto :error
..\utils\printsize.exe %game%c.bin
if exist %game%%YUN_LOAD%.sna del %game%%YUN_LOAD%.sna
..\utils\cpctbin2sna.exe %game%c.bin 0x400 -pc 0x400 -o %game%%YUN_LOAD%.sna > nul
rem Songs are unpacked to WYZ_SONG_BUFFER (definitions.h), which must leave room
rem up to 9000 for the biggest .mus; the binary must end below it.
for /f "tokens=3" %%a in ('findstr /c:"#define WYZ_SONG_BUFFER" definitions.h') do set BUF=%%a
set MUS=0
for %%m in (..\ogt\*.mus) do if %%~zm GTR !MUS! set MUS=%%~zm
for %%I in (%game%c.bin) do set SIZE=%%~zI
set /a END=1024+SIZE
set /a LIMIT=%BUF%
set /a ROOM=0x9000-LIMIT
echo Output: %game%%YUN_LOAD%.sna (%SIZE% bytes, ends at %END%, must end below %LIMIT% = %BUF%; biggest .mus %MUS% bytes, room %ROOM%)
if %END% GTR %LIMIT% (
	echo ERROR: the binary runs into the song buffer
	exit /b 1
)
if %MUS% GTR %ROOM% (
	echo ERROR: the biggest .mus does not fit between WYZ_SONG_BUFFER and 9000: lower WYZ_SONG_BUFFER to 0x9000 - %MUS%
	exit /b 1
)
goto :end

:screen
..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\cpc\pal%YUN_LOAD%.png mode=superbuffer in=..\gfx\cpc\%2.png out=cpc\%1.bin silent > nul
if exist cpc\%1c.bin del cpc\%1c.bin
..\utils\zx0.exe cpc\%1.bin cpc\%1c.bin > nul
del cpc\%1.bin
goto :eof

:level
..\utils\mkts_om.exe platform=cpc mode=pals in=..\gfx\cpc\pal_cnvtiles%1.png prefix=lvl%1_pal out=cpc\lvl%1_pal.h silent > nul
..\utils\mapcnv.exe ..\map\level%1.map cpc\lvl%1_map.bin %2 %3 15 10 %4 packed bin maxlocks=%LOCKS% screens=%SCREENS% > nul
..\utils\ene2h.exe ..\enems\level%1.ene cpc\lvl%1_ene.h bin=cpc\lvl%1_ene.bin screens=%SCREENS% > nul
copy /b cpc\lvl%1_map.bin + cpc\lvl%1_ene.bin + cpc\lvl%1_tmaps.bin cpc\lvl%1_area.bin > nul
rem The level area must come out exactly SCREENS * 105 + LOCKS * 4 + 64 bytes
rem (old mapcnv / ene2h, without `bin`, write C text here: the game would hang)
for %%I in (cpc\lvl%1_area.bin) do set ASIZE=%%~zI
set /a AWANT=SCREENS*105+LOCKS*4+64
if not "%ASIZE%"=="%AWANT%" (
	echo ERROR: cpc\lvl%1_area.bin is %ASIZE% bytes, should be %AWANT%: are mapcnv.exe and ene2h.exe the versions with "bin" ^(utils\*.bas^)?
	set BADLEVEL=1
)
..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\cpc\pal_cnvspr.png mode=sprites in=..\gfx\cpc\sprites%1.png out=cpc\lvl%1_spr.bin max=12 silent > nul
if exist cpc\lvl%1_area.zx0 del cpc\lvl%1_area.zx0
if exist cpc\lvl%1_spr.zx0 del cpc\lvl%1_spr.zx0
..\utils\zx0.exe cpc\lvl%1_area.bin cpc\lvl%1_area.zx0 > nul
..\utils\zx0.exe cpc\lvl%1_spr.bin cpc\lvl%1_spr.zx0 > nul
del cpc\lvl%1_map.bin cpc\lvl%1_ene.bin cpc\lvl%1_ene.h cpc\lvl%1_area.bin cpc\lvl%1_spr.bin
goto :eof

:error
echo ERROR
exit /b 1

:end
