@echo off
rem Yun CPC, first load (NES levels 1-3) - build script (Windows). comp_cpc.sh does the same.
rem MK1 with the CPC_DIRTY backend (no superbuffer), mode 0, split palette.
rem
rem Needs: MK1's z88dk (env\setenv.bat) and the MK1 tools in ..\utils. mapcnv
rem and ene2h must be the versions with the `bin` option (src\utils).
rem
rem   comp_cpc.bat            convert resources + compile + snapshot
rem   comp_cpc.bat justcompile

set game=yun
cd /d %~dp0

if [%1]==[justcompile] goto :compile

echo Converting resources
rem Patterns + tile maps: cpc\patterns.bin, cpc\tsx_tmaps.bin, cpc\lvlN_tmaps.bin (gfx\cpc\tiles.spt)
..\utils\mkts_om.exe platform=cpc mode=scripted in=..\gfx\cpc\tiles.spt > nul
if exist cpc\patterns.zx0 del cpc\patterns.zx0
..\utils\zx0.exe cpc\patterns.bin cpc\patterns.zx0 > nul
rem Fixed sprite cells, in the binary
..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\cpc\pal_cnvspr.png mode=sprites in=..\gfx\cpc\sprites.png out=cpc\sprites.bin max=22 silent > nul
rem Palettes: pal.png (titles, frame, ending: my_inks), per level background inks, sprite inks
..\utils\mkts_om.exe platform=cpc mode=palsasassembly in=..\gfx\cpc\pal.png prefix=my_inks out=cpc\pal.h silent > nul
..\utils\mkts_om.exe platform=cpc mode=pals in=..\gfx\cpc\pal_cnvspr.png prefix=spr_pal out=cpc\spr_pal.h silent > nul
rem Levels: cpc\lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), cpc\lvlN_spr.zx0
for %%n in (0 1 2) do call :level %%n
rem Screens
for %%s in (title marco ending) do (
	..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\cpc\pal.png mode=superbuffer in=..\gfx\cpc\%%s.png out=cpc\%%s.bin silent > nul
	if exist cpc\%%sc.bin del cpc\%%sc.bin
	..\utils\zx0.exe cpc\%%s.bin cpc\%%sc.bin > nul
	del cpc\%%s.bin
)

:compile
echo Compiling
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%c.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY || goto :error
..\utils\printsize.exe %game%c.bin
if exist %game%.sna del %game%.sna
..\utils\cpctbin2sna.exe %game%c.bin 0x400 -pc 0x400 -o %game%.sna > nul
echo Output: %game%.sna (the binary must end below 8800)
goto :end

:level
..\utils\mkts_om.exe platform=cpc mode=pals in=..\gfx\cpc\pal_cnvtiles%1.png prefix=lvl%1_pal out=cpc\lvl%1_pal.h silent > nul
..\utils\mapcnv.exe ..\map\level%1.map cpc\lvl%1_map.bin 4 6 15 10 15 packed bin maxlocks=2 > nul
..\utils\ene2h.exe ..\enems\level%1.ene cpc\lvl%1_ene.h bin=cpc\lvl%1_ene.bin > nul
copy /b cpc\lvl%1_map.bin + cpc\lvl%1_ene.bin + cpc\lvl%1_tmaps.bin cpc\lvl%1_area.bin > nul
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
