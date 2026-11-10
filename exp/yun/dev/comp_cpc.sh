#!/bin/sh
# Yun CPC - build script (Linux). comp_cpc.bat does the same.
# MK1 with the CPC_DIRTY backend (no superbuffer), mode 0, split palette.
#
# Needs: the MK1 z88dk for Linux in PATH and the MK1 tools built for Linux
# (FreeBASIC) in $MK1_TOOLS: mkts_om, mapcnv, ene2h (the versions with the `bin`
# option). zx0 and cpctbin2sna have Python stand-ins in ../tools (set ZX0 to use
# a native zx0 instead).
#
#   ./comp_cpc.sh [1|2]              convert resources + compile + snapshot of that load (1)
#   ./comp_cpc.sh [1|2] justcompile  (justcompile: with the resources last converted)
#   YUNFLAGS=-DYUN_DEBUG_LEVEL=2 ./comp_cpc.sh 1 justcompile   (start at the load's level 3)

set -e
game=yun
YUN_LOAD=1
case "$1" in 1|2) YUN_LOAD=$1; shift ;; esac
# Each load: levels (number:map width:map height:lock tile, 99 = no locks),
# screens of the largest map and locks (the level area: SCREENS = MAP_W * MAP_H
# and MAX_LOCKS in yun_level.h). Fixed sprite cells.
case $YUN_LOAD in
	1) LEVELS="0:4:6:15 1:4:6:15 2:4:6:15 3:4:6:15"; SCREENS=24; LOCKS=2 ;;
	2) LEVELS="4:12:2:15 5:6:4:15 6:6:4:15 7:9:3:99"; SCREENS=27; LOCKS=4 ;;
esac
CELLS=32
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
ZX0=${ZX0:-"$PY ../tools/zx0.py"}

cd "$(dirname "$0")"

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	# Patterns + tile maps: cpc/patterns.bin, cpc/tsx_tmaps.bin, cpc/lvlN_tmaps.bin (gfx/cpc/tilesN.spt)
	$T/mkts_om platform=cpc mode=scripted in=../gfx/cpc/tiles$YUN_LOAD.spt > /dev/null
	$ZX0 cpc/patterns.bin cpc/patterns.zx0 > /dev/null
	# Fixed sprite cells, in the binary
	$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/cpc/pal_cnvspr.png mode=sprites in=../gfx/cpc/sprites.png out=cpc/sprites.bin max=$CELLS silent > /dev/null
	# Palettes: palN.png (title and ending: my_inks), per level background inks, sprite inks
	$T/mkts_om platform=cpc mode=palsasassembly in=../gfx/cpc/pal$YUN_LOAD.png prefix=my_inks out=cpc/pal.h silent > /dev/null
	$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_cnvspr.png prefix=spr_pal out=cpc/spr_pal.h silent > /dev/null
	if [ -f ../gfx/cpc/pal_dark.png ]; then
		$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_dark.png prefix=dark_pal out=cpc/dark_pal.h silent > /dev/null
		$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_light.png prefix=light_pal out=cpc/light_pal.h silent > /dev/null
	fi
	# Levels: cpc/lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), cpc/lvlN_spr.zx0
	locks=$LOCKS
	for l in $LEVELS; do
		set -- $(echo $l | tr ':' ' ')
		n=$1; wh="$2 $3"; lock=$4
		$T/mkts_om platform=cpc mode=pals in=../gfx/cpc/pal_cnvtiles$n.png prefix=lvl${n}_pal out=cpc/lvl${n}_pal.h silent > /dev/null
		$T/mapcnv ../map/level$n.map cpc/lvl${n}_map.bin $wh 15 10 $lock packed bin maxlocks=$locks screens=$SCREENS > /dev/null
		$T/ene2h ../enems/level$n.ene cpc/lvl${n}_ene.h bin=cpc/lvl${n}_ene.bin screens=$SCREENS > /dev/null
		cat cpc/lvl${n}_map.bin cpc/lvl${n}_ene.bin cpc/lvl${n}_tmaps.bin > cpc/lvl${n}_area.bin
		# The level area must come out exactly SCREENS * 105 + LOCKS * 4 + 64 bytes
		# (old mapcnv / ene2h, without `bin`, write C text here: the game would hang)
		size=$(wc -c < cpc/lvl${n}_area.bin); want=$((SCREENS * 105 + locks * 4 + 64))
		if [ $size -ne $want ]; then echo "ERROR: cpc/lvl${n}_area.bin is $size bytes, should be $want: are mapcnv and ene2h the versions with \`bin\` (utils/*.bas)?"; exit 1; fi
		$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/cpc/pal_cnvspr.png mode=sprites in=../gfx/cpc/sprites$n.png out=cpc/lvl${n}_spr.bin max=12 silent > /dev/null
		$ZX0 cpc/lvl${n}_area.bin cpc/lvl${n}_area.zx0 > /dev/null
		$ZX0 cpc/lvl${n}_spr.bin cpc/lvl${n}_spr.zx0 > /dev/null
		rm cpc/lvl${n}_map.bin cpc/lvl${n}_ene.bin cpc/lvl${n}_ene.h cpc/lvl${n}_area.bin cpc/lvl${n}_spr.bin
	done
	# Screens
	# (DIRECT_TO_PLAY: the title is the frame; each load has its ending)
	for s in title ending; do
		f=$s; [ $s = ending ] && f=ending$YUN_LOAD
		$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/cpc/pal$YUN_LOAD.png mode=superbuffer in=../gfx/cpc/$f.png out=cpc/$s.bin silent > /dev/null
		$ZX0 cpc/$s.bin cpc/${s}c.bin > /dev/null
		rm cpc/$s.bin
	done
fi

echo "Compiling"
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o ${game}c.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY -DYUN_LOAD=$YUN_LOAD $YUNFLAGS
$PY ../tools/mksna.py ${game}c.bin 0x400 -pc 0x400 -o $game$YUN_LOAD.sna > /dev/null
size=$(wc -c < ${game}c.bin)
# Songs are unpacked to WYZ_SONG_BUFFER (definitions.h), which must leave room
# up to 9000 for the biggest .mus; the binary must end below it.
buf=$(sed -n 's/^[[:space:]]*#define WYZ_SONG_BUFFER[[:space:]]*0x\([0-9A-Fa-f]*\).*/\1/p' definitions.h)
mus=0; for m in ../ogt/*.mus; do s=$(wc -c < "$m"); [ $s -gt $mus ] && mus=$s; done
echo "Output: $game$YUN_LOAD.sna ($size bytes, 0400-$(printf %04X $((1024 + size))), must end below $buf; biggest .mus $mus bytes, room $((0x9000 - 0x$buf)))"
if [ $((1024 + size)) -gt $((0x$buf)) ]; then echo "ERROR: the binary runs into the song buffer at $buf"; exit 1; fi
if [ $mus -gt $((0x9000 - 0x$buf)) ]; then echo "ERROR: the biggest .mus doesn't fit between $buf and 9000: WYZ_SONG_BUFFER = 0x$(printf %04X $((0x9000 - mus)))"; exit 1; fi
