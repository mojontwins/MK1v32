@echo off
rem Punchy (MK1 v3.2) - build script (Windows). comp.sh does the same on Linux.
rem
rem   comp.bat zx|cpc              convert resources + compile + tape / snapshot
rem   comp.bat zx|cpc justcompile
rem   set PNFLAGS=-DPN_TEST_SCR=1 -DPN_TEST_X=4     (start elsewhere, to test)

set game=punchy
set T=..\utils
set PF=%1
if [%PF%]==[zx] goto :ok
if [%PF%]==[cpc] goto :ok
echo usage: comp.bat zx^|cpc [justcompile]
goto :end
:ok

if [%2]==[justcompile] goto :compile

echo Converting resources (%PF%)
%T%\mapcnv.exe ..\map\level0.map mapa.h 4 1 15 10 99 packed > nul
%T%\ene2h.exe ..\enems\level0.ene enems.h dsall > nul
if [%PF%]==[cpc] goto :rescpc

set G=..\gfx\speccy
%T%\mkts_om.exe platform=zx mode=scripted in=%G%\tiles.spt > nul
set M=platform=zx mode=sprites silent
%T%\mkts_om.exe %M% in=%G%\spr_tall.png out=speccy\spr_tall metasize=2,4 size=8,4 max=27 > nul
%T%\mkts_om.exe %M% in=%G%\spr_boom.png out=speccy\spr_boom metasize=2,4 size=1,1 > nul
%T%\mkts_om.exe %M% in=%G%\spr_empty.png out=speccy\spr_empty metasize=2,4 size=1,1 > nul
%T%\mkts_om.exe %M% in=%G%\spr_piece.png out=speccy\spr_piece metasize=1,2 size=3,1 > nul
%T%\mkts_om.exe %M% in=%G%\spr_piece_empty.png out=speccy\spr_piece_empty metasize=1,2 size=1,1 > nul
%T%\mkts_om.exe %M% in=%G%\spr_coco.png out=speccy\spr_coco metasize=1,1 size=1,1 > nul
for %%s in (title marco ending) do (
	%T%\png2scr.exe %G%\%%s.png %%s.scr > nul
	%T%\zx0.exe %%s.scr %%sc.bin > nul
	del %%s.scr
)
%T%\png2scr.exe %G%\loading.png loading.bin > nul
goto :compile

:rescpc
set G=..\gfx\cpc
%T%\mkts_om.exe platform=cpc mode=scripted in=%G%\tiles.spt > nul
%T%\zx0.exe cpc\patterns.bin cpc\patterns.zx0 > nul
set M=platform=cpc cpcmode=0 pal=%G%\pal_cnvspr.png mode=sprites silent
%T%\mkts_om.exe %M% in=%G%\spr_tall.png out=cpc\spr_tall metasize=2,4 size=8,4 max=27 > nul
%T%\mkts_om.exe %M% in=%G%\spr_boom.png out=cpc\spr_boom metasize=2,4 size=1,1 > nul
%T%\mkts_om.exe %M% in=%G%\spr_piece.png out=cpc\spr_piece metasize=1,2 size=3,1 > nul
%T%\mkts_om.exe %M% in=%G%\spr_coco.png out=cpc\spr_coco metasize=1,1 size=1,1 > nul
%T%\mkts_om.exe platform=cpc mode=palsasassembly in=%G%\pal.png prefix=my_inks out=cpc\pal silent > nul
%T%\mkts_om.exe platform=cpc mode=pals in=%G%\pal_cnvtiles.png prefix=bg_pal out=cpc\bg_pal silent > nul
%T%\mkts_om.exe platform=cpc mode=pals in=%G%\pal_cnvspr.png prefix=spr_pal out=cpc\spr_pal silent > nul
for %%s in (title marco ending) do (
	%T%\mkts_om.exe platform=cpc cpcmode=0 pal=%G%\pal.png mode=superbuffer in=%G%\%%s.png out=cpc\%%s silent > nul
	%T%\zx0.exe cpc\%%s.bin cpc\%%sc.bin > nul
	del cpc\%%s.bin
)

:compile
echo Compiling (%PF%)
if [%PF%]==[cpc] goto :compcpc

zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin churromain.c -DSPECCY %PNFLAGS%
if %errorlevel% neq 0 goto :error
%T%\printsize.exe %game%.bin
%T%\bas2tap.exe -q -e -a10 -s"%game%" system\loader.bas %game%.tap > nul
%T%\bin2tap.exe -o %game%.tap -a 32768 -append loading.bin > nul
%T%\bin2tap.exe -o %game%.tap -a 24200 -append %game%.bin > nul
echo Output: %game%.tap (must end below 59392, the flip table at E800)
goto :end

:compcpc
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%c.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY %PNFLAGS%
if %errorlevel% neq 0 goto :error
%T%\printsize.exe %game%c.bin
if exist %game%.sna del %game%.sna
%T%\cpctbin2sna.exe %game%c.bin 0x400 -pc 0x400 -o %game%.sna
echo Output: %game%.sna (must end below 8800)
goto :end

:error
echo ERROR

:end
