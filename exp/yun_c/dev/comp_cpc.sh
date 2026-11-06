#!/bin/sh
# Yun CPC, third load (NES levels 7-8) - build script (Linux). comp_cpc.bat does the same.
# MK1 with the CPC_DIRTY backend (no superbuffer), mode 0, split palette.
#
# Needs: the MK1 z88dk for Linux in PATH and the MK1 tools built for Linux
# (FreeBASIC) in $MK1_TOOLS: mkts_om, mapcnv, ene2h (the versions with the `bin`
# option). zx0 and cpctbin2sna have Python stand-ins in ../tools (set ZX0 to use
# a native zx0 instead).
#
#   ./comp_cpc.sh            convert resources + compile + snapshot
#   ./comp_cpc.sh justcompile

set -e
game=yun
# This load: levels (number:map width:map height:lock tile, 99 = no locks),
# screens of the largest map (the level area, yun_level.h), fixed sprite cells
LEVELS="0:6:4:15 1:9:3:99"
SCREENS=27
CELLS=32
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
ZX0=${ZX0:-"$PY ../tools/zx0.py"}

cd "$(dirname "$0")"

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	# Patterns + tile maps: cpc/patterns.bin, cpc/tsx_tmaps.bin, cpc/lvlN_tmaps.bin (gfx/cpc/tiles.spt)
	$T/mkts_om platform=cpc mode=scripted in=../gfx/cpc/tiles.spt > /dev/null
	$ZX0 cpc/patterns.bin cpc/patterns.zx0 > /dev/null
	# Fixed sprite cells, in the binary
	$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/cpc/pal_cnvspr.png mode=sprites in=../gfx/cpc/sprites.png out=cpc/sprites.bin max=$CELLS silent > /dev/null
	# Palettes: pal.png (titles, frame, ending: my_inks), per level background inks, sprite inks
	$T/mkts_om platform=cpc mode=palsasassembly in=../gfx/cpc/pal.png prefix=my_inks out=cpc/pal.h silent > /dev/null
	$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_cnvspr.png prefix=spr_pal out=cpc/spr_pal.h silent > /dev/null
	if [ -f ../gfx/cpc/pal_dark.png ]; then
		$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_dark.png prefix=dark_pal out=cpc/dark_pal.h silent > /dev/null
		$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_light.png prefix=light_pal out=cpc/light_pal.h silent > /dev/null
	fi
	# Levels: cpc/lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), cpc/lvlN_spr.zx0
	locks=$(sed -n 's/^#define MAX_LOCKS[[:space:]]*\([0-9]*\).*/\1/p' yun_level.h)
	for l in $LEVELS; do
		set -- $(echo $l | tr ':' ' ')
		n=$1; wh="$2 $3"; lock=$4
		$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_cnvtiles$n.png prefix=lvl${n}_pal out=cpc/lvl${n}_pal.h silent > /dev/null
		$T/mapcnv ../map/level$n.map cpc/lvl${n}_map.bin $wh 15 10 $lock packed bin maxlocks=$locks screens=$SCREENS > /dev/null
		$T/ene2h ../enems/level$n.ene cpc/lvl${n}_ene.h bin=cpc/lvl${n}_ene.bin screens=$SCREENS > /dev/null
		cat cpc/lvl${n}_map.bin cpc/lvl${n}_ene.bin cpc/lvl${n}_tmaps.bin > cpc/lvl${n}_area.bin
		$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/cpc/pal_cnvspr.png mode=sprites in=../gfx/cpc/sprites$n.png out=cpc/lvl${n}_spr.bin max=12 silent > /dev/null
		$ZX0 cpc/lvl${n}_area.bin cpc/lvl${n}_area.zx0 > /dev/null
		$ZX0 cpc/lvl${n}_spr.bin cpc/lvl${n}_spr.zx0 > /dev/null
		rm cpc/lvl${n}_map.bin cpc/lvl${n}_ene.bin cpc/lvl${n}_ene.h cpc/lvl${n}_area.bin cpc/lvl${n}_spr.bin
	done
	# Screens
	for s in title marco ending; do
		$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/cpc/pal.png mode=superbuffer in=../gfx/cpc/$s.png out=cpc/$s.bin silent > /dev/null
		$ZX0 cpc/$s.bin cpc/${s}c.bin > /dev/null
		rm cpc/$s.bin
	done
fi

echo "Compiling"
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o ${game}c.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY
$PY ../tools/mksna.py ${game}c.bin 0x400 -pc 0x400 -o $game.sna > /dev/null
size=$(wc -c < ${game}c.bin)
echo "Output: $game.sna ($size bytes, 0400-$(printf %04X $((1024 + size))), must end below 8800)"
if [ $((1024 + size)) -gt $((0x8800)) ]; then echo "ERROR: the binary runs into 8800"; exit 1; fi
