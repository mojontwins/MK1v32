#!/bin/sh
# Nin Nin CPC - build script (Linux). comp_cpc.bat does the same.
# MK1 with the CPC_DIRTY backend (no superbuffer), mode 1.
#
# Needs: the MK1 z88dk for Linux in PATH and the MK1 tools built for Linux
# (FreeBASIC) in $MK1_TOOLS: mkts_om, rle44map_sp (with keepempty and
# maxlocks), ene2h (with bin). zx0 and cpctbin2sna have Python stand-ins in
# ../tools (set ZX0 to use a native zx0 instead).
#
#   ./comp_cpc.sh [1|2|3]              convert resources + compile + snapshot of that load (1)
#   ./comp_cpc.sh [1|2|3] justcompile
#   NNFLAGS=-DNN_DEBUG_LEVEL=2 ./comp_cpc.sh 1 justcompile    (start at the load's level 3)

set -e
game=ninnin
NN_LOAD=1
case "$1" in 1|2|3) NN_LOAD=$1; shift ;; esac
# Each load: levels of the game (map/levels.txt: number width height lock tile,
# 99 = no locks), screens of the level area and the map limit (nn_load.h)
SCREENS=24
LOCKS=4
case $NN_LOAD in
	1) LEVELS="0 1 2 3"; MAPMAX=680 ;;
	2) LEVELS="4 5 6"; MAPMAX=600 ;;
	3) LEVELS="7 8 9"; MAPMAX=830 ;;
esac
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
ZX0=${ZX0:-"$PY ../tools/zx0.py"}
G=../gfx/cpc
M="platform=cpc cpcmode=1 pal=$G/pal.png"

cd "$(dirname "$0")"

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	# Patterns: the font (0-63) and the 48 tiles (64-255)
	$T/mkts_om $M mode=chars in=$G/font.png out=cpc/font.bin silent > /dev/null
	$T/mkts_om $M mode=strait2x2 in=$G/work.png out=cpc/work.bin silent > /dev/null
	cat cpc/font.bin cpc/work.bin > cpc/patterns.bin
	$ZX0 cpc/patterns.bin cpc/patterns.zx0 > /dev/null
	rm cpc/font.bin cpc/work.bin
	# Fixed sprite cells, in the binary
	$T/mkts_om $M mode=sprites in=$G/sprites.png out=cpc/sprites.bin max=11 silent > /dev/null
	# Palettes: pal.png (titles: my_inks), the inks of every level
	$T/mkts_om platform=cpc mode=palsasassembly in=$G/pal0.png prefix=my_inks out=cpc/pal.h silent > /dev/null
	for n in 0 1 2 3 4 5 6 7 8 9; do
		$T/mkts_om platform=cpc mode=pals in=$G/pal$n.png prefix=lvl${n}_pal out=cpc/lvl${n}_pal.h silent > /dev/null
	done
	# Levels: cpc/lvlN_area.zx0 (locks, enemies + hotspots, map), cpc/lvlN_spr.zx0,
	# N = 0.. in this load
	rm -f cpc/lvl?_area.zx0 cpc/lvl?_spr.zx0
	i=0
	for n in $LEVELS; do
		set -- $(grep "^$n " ../map/levels.txt)
		$T/rle44map_sp in=../map/level$n.map out=cpc/lvl$n size=$2,$3 tlock=$4 mk1locks keepempty maxlocks=$LOCKS > /dev/null
		size=$(wc -c < cpc/lvl$n.map.bin)
		if [ $size -gt $MAPMAX ]; then echo "ERROR: level $n map is $size bytes, NN_MAP_MAX is $MAPMAX"; exit 1; fi
		$T/ene2h ../enems/level$n.ene cpc/lvl${n}_ene.h bin=cpc/lvl${n}_ene.bin screens=$SCREENS > /dev/null
		cat cpc/lvl$n.locks.bin cpc/lvl${n}_ene.bin cpc/lvl$n.map.bin > cpc/area.bin
		$ZX0 cpc/area.bin cpc/lvl${i}_area.zx0 > /dev/null
		$T/mkts_om $M mode=sprites in=$G/sprites$n.png out=cpc/spr.bin max=16 silent > /dev/null
		$ZX0 cpc/spr.bin cpc/lvl${i}_spr.zx0 > /dev/null
		rm cpc/lvl$n.locks.bin cpc/lvl$n.map.bin cpc/lvl${n}_ene.h cpc/lvl${n}_ene.bin cpc/area.bin cpc/spr.bin
		i=$((i + 1))
	done
	# Screens
	for s in title marco ending$NN_LOAD; do
		$T/mkts_om $M mode=superbuffer in=$G/$s.png out=cpc/$s.bin silent > /dev/null
		$ZX0 cpc/$s.bin cpc/${s%[0-9]}c.bin > /dev/null
		rm cpc/$s.bin
	done
fi

echo "Compiling"
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o ${game}.bin churromain.c -DCPC -DMODE_1 -DCPC_DIRTY -DNN_LOAD=$NN_LOAD $NNFLAGS
$PY ../tools/mksna.py ${game}.bin 0x400 -pc 0x400 -o $game$NN_LOAD.sna > /dev/null
size=$(wc -c < ${game}.bin)
echo "Output: $game$NN_LOAD.sna ($size bytes, 0400-$(printf %04X $((1024 + size))), must end below 8800)"
if [ $((1024 + size)) -gt $((0x8800)) ]; then echo "ERROR: the binary runs into 8800"; exit 1; fi
