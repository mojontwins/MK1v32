@echo off
rem MTE MK1 v3.2 - Camp Escape - Windows build (z88dk10 + the .exe tools in ..\utils)
rem
rem Before building: open ..\apor\greatesc.apor in Aportador and press Exportar
rem (CTRL+E). That writes ..\map\mapa.map, ..\enems\enems.ene and ..\map\behs.txt.
rem If you changed tile behaviours, copy behs.txt into comportamiento_tiles in
rem config.h (comp.sh does that automatically on Linux).

set game=greatesc
set om=speccy

echo Making %game%

if [%1]==[justcompile] goto :compile

..\utils\rle53map_sp.exe in=..\map\mapa.map out=mapa.bin size=6,5 scrsize=15,10 mk1h=mapa.h > nul
..\utils\ene2h.exe ..\enems\enems.ene enems.h 2bytes > nul

..\utils\ts2bin.exe ..\gfx\%om%\font.png ..\gfx\%om%\work.png tileset.bin 7 > nul
..\utils\sprcnv.exe ..\gfx\%om%\sprites.png speccy\sprites.h > nul
..\utils\png2scr.exe ..\gfx\%om%\title.png ..\gfx\%om%\title.scr > nul
..\utils\png2scr.exe ..\gfx\%om%\marco.png ..\gfx\%om%\marco.scr > nul
..\utils\png2scr.exe ..\gfx\%om%\ending.png ..\gfx\%om%\ending.scr > nul
..\utils\png2scr.exe ..\gfx\%om%\loading.png loading.bin > nul
..\utils\zx0.exe ..\gfx\%om%\title.scr titlec.bin > nul
..\utils\zx0.exe ..\gfx\%om%\marco.scr marcoc.bin > nul
..\utils\zx0.exe ..\gfx\%om%\ending.scr endingc.bin > nul

:compile

echo Making script
cd ..\script
..\utils\msc4.exe in=script.spt v=3 target=zx interpreter=msc4i.asm text=text.bin debug > ..\dev\msc.txt
if %errorlevel% neq 0 goto :scripterror
copy script.spt.bin ..\dev > nul
copy text.bin ..\dev > nul
move msc4i.asm ..\dev > nul
cd ..\dev

del zcc_opt.def > nul 2> nul
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin msc4i.asm churromain.c -DSPECCY > nul
if %errorlevel% neq 0 goto :error

..\utils\printsize.exe %game%.bin
..\utils\bas2tap.exe -q -e -a10 -s"%game%" system/loader.bas %game%.tap > nul
..\utils\bin2tap.exe -o %game%.tap -a 32768 -append loading.bin > nul
..\utils\bin2tap.exe -o %game%.tap -a 24200 -append %game%.bin > nul
copy %game%.tap ..\show > nul

echo Output: ..\show\%game%.tap
goto :noerror

:scripterror
cd ..\dev
echo SCRIPT ERROR, see msc.txt
goto :end

:error
echo ERROR

:noerror

if [%1]==[noclean] goto :end
if [%2]==[noclean] goto :end

del ..\gfx\%om%\*.scr > nul 2> nul
del *.bin > nul

:end
