#!/bin/sh
# Nanako in Classic Japanese Monster Castle - Amstrad CPC 464 build (Linux). comp_cpc.bat does the same.
# MK1 v3.2's standard CPC engine (cpcrslib, superbuffer), mode 0, 16 colours.
# The graphics are placeholders made from the original CPC ones (tools/nanako_conv.py),
# pen 0 is the sprites' transparent colour. The songs are the original's (tools/nanako_music.py).
#
# Needs: the MK1 z88dk for Linux in PATH and the MK1 tools built for Linux in
# $MK1_TOOLS (ene2h, mkts_om, cpctape, cpcdsk). zx0, the snapshot and the LUT
# are the Python versions in ../tools.
#
#   ./comp_cpc.sh               convert resources + compile + snapshot, tape, disk
#   ./comp_cpc.sh justcompile

set -e
game=nanako_cpc
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
G=../gfx/cpc
OM="platform=cpc cpcmode=0 pal=$G/pal.png"

cd "$(dirname "$0")"

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	$T/ene2h ../enems/enems.ene enems.h 2bytes > /dev/null
	$T/mkts_om $OM mode=chars greyordered in=$G/font.png out=font.bin silent > /dev/null
	$T/mkts_om $OM mode=strait2x2 greyordered in=$G/work.png out=work.bin silent > /dev/null
	$T/mkts_om $OM mode=sprites in=$G/sprites.png out=sprites.bin mappings=cpc/spriteset_mappings.h max=19 pixelperfectm0 silent > /dev/null
	$T/mkts_om $OM mode=sprites in=$G/sprites_extra.png out=sprites_extra.bin max=2 silent > /dev/null
	$T/mkts_om platform=cpc mode=palsasassembly in=$G/pal.png prefix=my_inks out=cpc/pal.h silent > /dev/null
	for s in title marco ending; do
		$T/mkts_om $OM mode=superbuffer in=$G/$s.png out=$s.bin silent > /dev/null
		$PY ../tools/zx0.py $s.bin ${s}c_cpc.bin > /dev/null
	done
	$PY ../tools/defb2bin.py system/cpc_TrPixLutM0.asm trpixlut.bin
	for m in ../ogt/nanako_*.mus; do
		$PY ../tools/zx0.py $m $m.bin > /dev/null
	done
	$PY ../tools/zx0.py trpixlut.bin trpixlutc.bin > /dev/null
fi

echo "Compiling"
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o $game.bin system/tilemap_conf.asm churromain.c -DCPC -DMODE_0
$PY ../tools/mksna.py $game.bin 0x400 -pc 0x400 -o $game.sna > /dev/null
echo "Output: $game.sna ($(wc -c < $game.bin) bytes, 0400-$(printf %04X $((1024 + $(wc -c < $game.bin)))))"

# Tape: system/nkpre.asm (firmware file, RUN"") loads system/nkloader.asm (raw) at
# BA00, which loads the game (zx0, raw) and unpacks it to 0400. Disk: RUN"NANAKO
$PY ../tools/zx0.py $game.bin main.zx0 > /dev/null
MAIN=$(wc -c < main.zx0)
printf "\tdefc MAIN_SIZE = %d\n\tdefc LOADER_SIZE = 0\n" $MAIN > system/sizes.inc
(cd system && z80asm -b nkloader.asm)
LDR=$(wc -c < system/nkloader.bin)
printf "\tdefc MAIN_SIZE = %d\n\tdefc LOADER_SIZE = %d\n" $MAIN $LDR > system/sizes.inc
(cd system && z80asm -b nkpre.asm)
$T/cpctape in=system/nkpre.bin out=$game.cdt mode=fw name=NANAKO new > /dev/null
$T/cpctape in=system/nkloader.bin out=$game.cdt mode=raw pause=100 > /dev/null
$T/cpctape in=main.zx0 out=$game.cdt mode=raw > /dev/null
$T/cpcdsk out=$game.dsk file1=$game.bin,NANAKO.BIN,0x400,0x400 > /dev/null
rm -f system/*.obj system/*.map system/*.sym system/*.err main.zx0
echo "Output: $game.cdt (RUN\"\"), $game.dsk (RUN\"NANAKO)"
