@echo off
rem Nin Nin CPC, first load (levels 1-4) - build script (Windows). comp_cpc.sh does the same.
rem MK1 with the CPC_DIRTY backend (no superbuffer), mode 1.
rem
rem Needs: MK1's z88dk (env\setenv.bat) and the MK1 tools in ..\utils.
rem rle44map_sp must be the version with keepempty and maxlocks, ene2h the one
rem with bin (src\utils of the patched MK1).
rem
rem   comp_cpc.bat            convert resources + compile + snapshot
rem   comp_cpc.bat justcompile
rem   set NNFLAGS=-DNN_DEBUG_LEVEL=2 then comp_cpc.bat justcompile   (start at level 3)

set game=ninnin
rem This load: its number (the ending screen), screens of the level area and
rem largest map (nn_load.h); below, one "call :level" per level of this load:
rem its number in the game, map width and height, lock tile (99 = no locks),
rem as in ..\map\levels.txt
set NN_LOAD=1
set SCREENS=24
set MAPMAX=680
set LOCKS=4
set G=..\gfx\cpc
set M=platform=cpc cpcmode=1 pal=%G%\pal.png
cd /d %~dp0

if [%1]==[justcompile] goto :compile

echo Converting resources
rem Patterns: the font (0-63) and the 48 tiles (64-255)
..\utils\mkts_om.exe %M% mode=chars in=%G%\font.png out=cpc\font.bin silent > nul
..\utils\mkts_om.exe %M% mode=strait2x2 in=%G%\work.png out=cpc\work.bin silent > nul
copy /b cpc\font.bin + cpc\work.bin cpc\patterns.bin > nul
if exist cpc\patterns.zx0 del cpc\patterns.zx0
..\utils\zx0.exe cpc\patterns.bin cpc\patterns.zx0 > nul
del cpc\font.bin cpc\work.bin
rem Fixed sprite cells, in the binary
..\utils\mkts_om.exe %M% mode=sprites in=%G%\sprites.png out=cpc\sprites.bin max=11 silent > nul
rem Palettes: pal0.png (titles: my_inks), the inks of every level
..\utils\mkts_om.exe platform=cpc mode=palsasassembly in=%G%\pal0.png prefix=my_inks out=cpc\pal.h silent > nul
for %%n in (0 1 2 3 4 5 6 7 8 9) do ..\utils\mkts_om.exe platform=cpc mode=pals in=%G%\pal%%n.png prefix=lvl%%n_pal out=cpc\lvl%%n_pal.h silent > nul
rem Levels: cpc\lvlN_area.zx0 (locks, enemies + hotspots, map), cpc\lvlN_spr.zx0
set I=0
call :level 0 10 2 99 || goto :error
call :level 1 8 3 15 || goto :error
call :level 2 12 1 99 || goto :error
call :level 3 8 2 15 || goto :error
rem Screens
for %%s in (title marco ending%NN_LOAD%) do (
	..\utils\mkts_om.exe %M% mode=superbuffer in=%G%\%%s.png out=cpc\%%s.bin silent > nul
)
for %%s in (title marco) do (
	if exist cpc\%%sc.bin del cpc\%%sc.bin
	..\utils\zx0.exe cpc\%%s.bin cpc\%%sc.bin > nul
	del cpc\%%s.bin
)
if exist cpc\endingc.bin del cpc\endingc.bin
..\utils\zx0.exe cpc\ending%NN_LOAD%.bin cpc\endingc.bin > nul
del cpc\ending%NN_LOAD%.bin

:compile
echo Compiling
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%.bin churromain.c -DCPC -DMODE_1 -DCPC_DIRTY %NNFLAGS% || goto :error
..\utils\printsize.exe %game%.bin
if exist %game%.sna del %game%.sna
..\utils\cpctbin2sna.exe %game%.bin 0x400 -pc 0x400 -o %game%.sna > nul
echo Output: %game%.sna (the binary must end below 8800)
goto :end

:level
rem %1 level, %2 %3 map size, %4 lock tile. Blobs are numbered in this load (I).
..\utils\rle44map_sp.exe in=..\map\level%1.map out=cpc\lvl%1 size=%2,%3 tlock=%4 mk1locks keepempty maxlocks=%LOCKS% > nul
for %%f in (cpc\lvl%1.map.bin) do if %%~zf GTR %MAPMAX% (echo ERROR: level %1 map is %%~zf bytes, NN_MAP_MAX is %MAPMAX% & exit /b 1)
..\utils\ene2h.exe ..\enems\level%1.ene cpc\lvl%1_ene.h bin=cpc\lvl%1_ene.bin screens=%SCREENS% > nul
copy /b cpc\lvl%1.locks.bin + cpc\lvl%1_ene.bin + cpc\lvl%1.map.bin cpc\area.bin > nul
if exist cpc\lvl%I%_area.zx0 del cpc\lvl%I%_area.zx0
..\utils\zx0.exe cpc\area.bin cpc\lvl%I%_area.zx0 > nul
..\utils\mkts_om.exe %M% mode=sprites in=%G%\sprites%1.png out=cpc\spr.bin max=16 silent > nul
if exist cpc\lvl%I%_spr.zx0 del cpc\lvl%I%_spr.zx0
..\utils\zx0.exe cpc\spr.bin cpc\lvl%I%_spr.zx0 > nul
del cpc\lvl%1.locks.bin cpc\lvl%1.map.bin cpc\lvl%1_ene.h cpc\lvl%1_ene.bin cpc\area.bin cpc\spr.bin
set /a I=%I%+1
goto :eof

:error
echo ERROR
exit /b 1

:end
