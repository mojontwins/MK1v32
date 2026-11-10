#!/bin/sh
# MTE MK1 v3.2 - Ramiro el Vampiro en "No tiene un respiro" - Linux CPC build
# (mode 1, placeholder graphics in ../gfx/cpc, WYZ music in ../ogt).
#
#   ./comp_cpc.sh              convert everything, compile, make the .sna
#   ./comp_cpc.sh justcompile  only recompile the script and the C code
#
# Output: game.sna (64K snapshot, for emulators). The .cdt (tape) needs the
# Windows-only loader tools (pasmo, imanol, cpc2cdt): see comp.bat.
#
# Set Z88DK to the z88dk10 directory (with linbin/, lib/, include/).

set -e
cd "$(dirname "$0")"

game=ramiro1
mode=1
PY="python3 -I"
U=../utils/py
Z88DK=${Z88DK:-$HOME/z88dk10-orig}
export PATH="$Z88DK/linbin:$Z88DK/bin:$PATH"
export ZCCCFG="$Z88DK/lib/config/"
export Z80_OZFILES="$Z88DK/lib/"
G=../gfx/cpc
P="platform=cpc cpcmode=$mode pal=$G/pal.png"

echo "Making $game (CPC)"

if [ "$1" != "justcompile" ]; then
	$PY $U/apor2mk1.py ../apor/$game.apor
	$PY $U/mk1data.py rle53map in=../map/mapa.MAP out=mapa.bin size=15,2 scrsize=15,10 mk1h=mapa.h
	$PY $U/mk1data.py ene2h ../enems/enems.ene enems.h 2bytes

	$PY $U/cpcgfx.py $P mode=chars greyordered in=$G/font.png out=font.bin
	$PY $U/cpcgfx.py $P mode=strait2x2 greyordered in=$G/work.png out=work.bin
	$PY $U/cpcgfx.py $P mode=sprites in=$G/sprites.png out=sprites.bin mappings=cpc/spriteset_mappings.h max=16 pixelperfectm1lut
	$PY $U/cpcgfx.py platform=cpc mode=palsasassembly in=$G/pal.png prefix=my_inks out=cpc/pal.h
	$PY $U/cpcgfx.py $P mode=superbuffer in=$G/title.png out=title.bin
	$PY $U/cpcgfx.py $P mode=superbuffer in=$G/ending.png out=ending.bin
	$PY $U/zx0.py title.bin titlec.bin
	$PY $U/zx0.py ending.bin endingc.bin
	$PY $U/cpctools.py defb2bin system/cpc_TrPixLutM$mode.asm trpixlut.bin
	$PY $U/zx0.py trpixlut.bin trpixlutc.bin
	# The loading screen, for the tape version (comp.bat)
	$PY $U/cpcgfx.py platform=cpc cpcmode=0 pal=$G/pal_loading.png mode=scr in=$G/loading.png out=loading.bin
fi

echo "Making script"
cd ../script
$PY ../utils/py/msc4.py in=$game.spt v=3 target=cpc interpreter=msc4i.asm text=text.bin debug > ../dev/msc.txt || { tail -20 ../dev/msc.txt; exit 1; }
cp $game.spt.bin ../dev/script.spt.bin
cp text.bin ../dev/
mv msc4i.asm ../dev/
cd ../dev

rm -f zcc_opt.def
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o $game.bin system/tilemap_conf.asm msc4i.asm churromain.c -DCPC -DMODE_$mode
echo "$game.bin: $(stat -c %s $game.bin) bytes, ends at $((1024 + $(stat -c %s $game.bin))) (must be < 34816 = WYZ_SONG_BUFFER)"
$PY $U/cpctools.py bin2sna $game.bin 0x400 $game.sna
mkdir -p ../show
cp $game.sna ../show/
echo "Output: ../show/$game.sna"
