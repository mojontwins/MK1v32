@echo off
rem Nanako in Classic Japanese Monster Castle - Amstrad CPC build (Windows). comp_cpc.sh does the same.
rem MK1 v3.2's standard CPC engine (cpcrslib, superbuffer), mode 0, 16 colours.
rem The graphics are placeholders made from the original CPC ones (tools/nanako_conv.py),
rem pen 0 is the sprites' transparent colour. The songs are the original's (tools/nanako_music.py).
rem
rem cpctape.exe and cpcdsk.exe are MK1 tools (utils\*.bas: compile them once with
rem fbc cpctape.bas cmdlineparser.bas, same for cpcdsk); the loaders are z80asm's.
rem
rem   comp_cpc.bat               convert resources + compile + snapshot, tape, disk
rem   comp_cpc.bat justcompile

set game=nanako_cpc
set G=..\gfx\cpc
set OM=platform=cpc cpcmode=0 pal=%G%\pal.png
cd /d %~dp0

if [%1]==[justcompile] goto :compile

echo Converting resources
..\utils\ene2h.exe ..\enems\enems.ene enems.h 2bytes > nul
..\utils\mkts_om.exe %OM% mode=chars greyordered in=%G%\font.png out=font.bin silent > nul
..\utils\mkts_om.exe %OM% mode=strait2x2 greyordered in=%G%\work.png out=work.bin silent > nul
..\utils\mkts_om.exe %OM% mode=sprites in=%G%\sprites.png out=sprites.bin mappings=cpc\spriteset_mappings.h max=19 pixelperfectm0 silent > nul
..\utils\mkts_om.exe %OM% mode=sprites in=%G%\sprites_extra.png out=sprites_extra.bin max=2 silent > nul
..\utils\mkts_om.exe platform=cpc mode=palsasassembly in=%G%\pal.png prefix=my_inks out=cpc\pal.h silent > nul
for %%s in (title marco ending) do (
	..\utils\mkts_om.exe %OM% mode=superbuffer in=%G%\%%s.png out=%%s.bin silent > nul
	if exist %%sc_cpc.bin del %%sc_cpc.bin
	..\utils\zx0.exe %%s.bin %%sc_cpc.bin > nul
)
..\utils\pasmo.exe system\cpc_TrPixLutM0.asm trpixlut.bin > nul
for %%m in (..\ogt\nanako_*.mus) do (
	if exist %%m.bin del %%m.bin
	..\utils\zx0.exe %%m %%m.bin > nul
)
if exist trpixlutc.bin del trpixlutc.bin
..\utils\zx0.exe trpixlut.bin trpixlutc.bin > nul

:compile
echo Compiling
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%.bin system\tilemap_conf.asm churromain.c -DCPC -DMODE_0 || goto :error
..\utils\printsize.exe %game%.bin
if exist %game%.sna del %game%.sna
..\utils\cpctbin2sna.exe %game%.bin 0x400 -pc 0x400 -o %game%.sna > nul
echo Output: %game%.sna

rem Tape: system\nkpre.asm (firmware file, RUN"") loads system\nkloader.asm (raw) at
rem BA00, which loads the game (zx0, raw) and unpacks it to 0400. Disk: RUN"NANAKO
if exist main.zx0 del main.zx0
..\utils\zx0.exe %game%.bin main.zx0 > nul
for %%I in (main.zx0) do set MAIN=%%~zI
(echo 	defc MAIN_SIZE = %MAIN%& echo 	defc LOADER_SIZE = 0) > system\sizes.inc
pushd system
z80asm -b nkloader.asm || (popd & goto :error)
popd
for %%I in (system\nkloader.bin) do set LDR=%%~zI
(echo 	defc MAIN_SIZE = %MAIN%& echo 	defc LOADER_SIZE = %LDR%) > system\sizes.inc
pushd system
z80asm -b nkpre.asm || (popd & goto :error)
popd
..\utils\cpctape.exe in=system\nkpre.bin out=%game%.cdt mode=fw name=NANAKO new > nul
..\utils\cpctape.exe in=system\nkloader.bin out=%game%.cdt mode=raw pause=100 > nul
..\utils\cpctape.exe in=main.zx0 out=%game%.cdt mode=raw > nul
..\utils\cpcdsk.exe out=%game%.dsk file1=%game%.bin,NANAKO.BIN,0x400,0x400 > nul
del system\*.obj system\*.map system\*.sym system\*.err main.zx0 2> nul
echo Output: %game%.cdt (RUN""), %game%.dsk (RUN"NANAKO)
goto :end

:error
echo ERROR
exit /b 1

:end
