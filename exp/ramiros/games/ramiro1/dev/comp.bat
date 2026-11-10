@echo off
rem MTE MK1 v3.2 - Ramiro 1 - Windows build
rem   comp.bat zx [justcompile]     ZX Spectrum 48K (.tap)
rem   comp.bat cpc [justcompile]    Amstrad CPC, mode 1, placeholder gfx (.cdt / .sna)
rem Linux: comp.sh (ZX) and comp_cpc.sh (CPC).
rem Map, enemies and behaviours: export apor\ramiro1.apor from Aportador first
rem (and copy behs.txt into comportamiento_tiles in config.h).

set game=ramiro1
set om=%1
if [%om%]==[] set om=zx
set mode=1

echo Making %game% (%om%)

if [%2]==[justcompile] goto :compile

..\utils\rle53map_sp.exe in=..\map\mapa.MAP out=mapa.bin size=15,2 scrsize=15,10 mk1h=mapa.h > nul
..\utils\ene2h.exe ..\enems\enems.ene enems.h 2bytes  > nul

if [%om%]==[cpc] goto :cpcgfx

..\utils\ts2bin.exe ..\gfx\speccy\font.png ..\gfx\speccy\work.png tileset.bin 7 > nul
..\utils\sprcnv.exe ..\gfx\speccy\sprites.png speccy\sprites.h  > nul
..\utils\png2scr.exe ..\gfx\speccy\title.png ..\gfx\speccy\title.scr  > nul
..\utils\png2scr.exe ..\gfx\speccy\ending.png ..\gfx\speccy\ending.scr  > nul
..\utils\png2scr.exe ..\gfx\speccy\loading.png loading.bin  > nul
..\utils\zx0.exe ..\gfx\speccy\title.scr titlec.bin  > nul
..\utils\zx0.exe ..\gfx\speccy\ending.scr endingc.bin  > nul
goto :compile

:cpcgfx
..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=chars greyordered in=..\gfx\cpc\font.png out=font.bin silent > nul
..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=strait2x2 greyordered in=..\gfx\cpc\work.png out=work.bin silent > nul
..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=sprites in=..\gfx\cpc\sprites.png out=sprites.bin mappings=cpc\spriteset_mappings.h max=16 pixelperfectm1lut silent > nul
..\utils\mkts_om.exe platform=cpc mode=palsasassembly in=..\gfx\cpc\pal.png prefix=my_inks out=cpc\pal.h silent > nul
..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=superbuffer in=..\gfx\cpc\ending.png out=ending.bin silent > nul
..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=superbuffer in=..\gfx\cpc\title.png out=title.bin silent > nul
..\utils\zx0.exe title.bin titlec.bin > nul
..\utils\zx0.exe ending.bin endingc.bin > nul

:compile
echo Making script
cd ..\script
..\utils\msc4.exe in=%game%.spt v=3 target=%om% interpreter=msc4i.asm text=text.bin debug >..\dev\msc.txt
copy %game%.spt.bin ..\dev\script.spt.bin > nul
copy text.bin ..\dev > nul
move msc4i.asm ..\dev > nul
cd ..\dev

if [%om%]==[cpc] goto :cpc

zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin msc4i.asm churromain.c -DSPECCY > nul
if %errorlevel% neq 0 goto :error
..\utils\printsize.exe %game%.bin
..\utils\bas2tap.exe -q -e -a10 -s"%game%" system/loader.bas %game%.tap  > nul
..\utils\bin2tap.exe -o %game%.tap -a 32768 -append loading.bin  > nul
..\utils\bin2tap.exe -o %game%.tap -a 24200 -append %game%.bin  > nul
echo Output: %game%.tap
goto :end

:cpc
..\utils\pasmo.exe system\cpc_TrPixLutM%mode%.asm trpixlut.bin
..\utils\zx0.exe trpixlut.bin trpixlutc.bin > nul 2> nul

zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%.bin system\tilemap_conf.asm msc4i.asm churromain.c -DCPC -DMODE_%mode% > nul
if %errorlevel% neq 0 goto :error
..\utils\printsize.exe %game%.bin

del %game%.sna > nul 2> nul
..\utils\cpctbin2sna.exe %game%.bin 0x400 -pc 0x400 -o %game%.sna
echo Output: %game%.sna

..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\cpc\pal_loading.png mode=scr in=..\gfx\cpc\loading.png out=loading.bin silent > nul
..\utils\zx7.exe loading.bin loading.c.bin > nul
..\utils\zx7.exe %game%.bin %game%.c.bin > nul
..\utils\imanol.exe in=system\loadercpc.asm-orig out=system\loadercpc.asm scrc_size=?loading.c.bin mainbin_size=?%game%.c.bin loading_palette=!..\gfx\cpc\pal_loading.png loader_mode=0 > nul
..\utils\pasmo.exe system\loadercpc.asm loader.bin  > nul
..\utils\imanol.exe in=system\preloadercpc.asm-orig out=system\preloadercpc.asm loader_size=?loader.bin loader_mode=0 > nul
..\utils\pasmo.exe system\preloadercpc.asm preloader.bin  > nul
del %game%.cdt > nul 2> nul
..\utils\cpc2cdt.exe -r %game% -m cpc -l 1024 -x 1024 -p 2000 preloader.bin %game%.cdt > nul
..\utils\cpc2cdt.exe -r LOADER -m raw1full -rl 740 -p 2000 loader.bin %game%.cdt > nul
..\utils\cpc2cdt.exe -r SCR -m raw1full -rl 740 -p 2000 loading.c.bin %game%.cdt > nul
..\utils\cpc2cdt.exe -r MAIN -m raw1full -rl 740 -p 2000 %game%.c.bin %game%.cdt > nul
echo Output: %game%.cdt
goto :end

:error
echo ERROR

:end
