#!/bin/sh
# Yun ZX, first load (NES levels 1-3) - build script (Linux). comp.bat does the same.
#
# Needs: the MK1 z88dk for Linux in PATH and the MK1 tools built for Linux
# (FreeBASIC) in $MK1_TOOLS: mkts_om, sprcnv2, mapcnv, ene2h (the versions with
# the `bin` option) and png2scr. zx0, bas2tap and bin2tap have Python stand-ins
# in ../tools (set ZX0 to use a native zx0 instead).
#
#   ./comp.sh            convert resources + compile + tape
#   ./comp.sh justcompile

set -e
game=yun
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
ZX0=${ZX0:-"$PY ../tools/zx0.py"}

cd "$(dirname "$0")"

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	# Charset + tile maps: tileset.bin, tsx_tmaps.bin, lvlN_tmaps.bin (gfx/speccy/tiles.spt)
	$T/mkts_om platform=zx mode=scripted in=../gfx/speccy/tiles.spt > /dev/null
	# Fixed sprite cells, in the binary
	$T/sprcnv2 ../gfx/speccy/sprites.png speccy/sprites.h 22 nomask > /dev/null
	# Levels: lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), lvlN_spr.zx0
	for n in 0 1 2; do
		$T/mapcnv ../map/level$n.map lvl${n}_map.bin 4 6 15 10 15 packed bin maxlocks=2 > /dev/null
		$T/ene2h ../enems/level$n.ene lvl${n}_ene.h bin=lvl${n}_ene.bin > /dev/null
		cat lvl${n}_map.bin lvl${n}_ene.bin lvl${n}_tmaps.bin > lvl${n}_area.bin
		$T/sprcnv2 ../gfx/speccy/sprites$n.png lvl${n}_spr.bin 12 nomask bin > /dev/null
		$ZX0 lvl${n}_area.bin lvl${n}_area.zx0 > /dev/null
		$ZX0 lvl${n}_spr.bin lvl${n}_spr.zx0 > /dev/null
		rm lvl${n}_map.bin lvl${n}_ene.bin lvl${n}_ene.h lvl${n}_area.bin lvl${n}_spr.bin
	done
	# Screens
	for s in title marco ending; do
		$T/png2scr ../gfx/speccy/$s.png $s.scr > /dev/null
		$ZX0 $s.scr ${s}c.bin > /dev/null
		rm $s.scr
	done
	$T/png2scr ../gfx/speccy/loading.png loading.bin > /dev/null
fi

echo "Compiling"
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o $game.bin churromain.c -DSPECCY
$PY ../tools/mktap.py $game.tap $game system/loader.bas loading.bin $game.bin 24200
end=$((24200 + $(wc -c < $game.bin)))
top=$(sed -n 's/^[[:space:]]*#define SPR_VAR_BASE[[:space:]]*\([0-9][0-9]*\)[[:space:]].*/\1/p' yun_level.h)
echo "Output: $game.tap ($(wc -c < $game.bin) bytes, ends at $end; level sprites at $top)"
if [ "$end" -gt "$top" ]; then echo "ERROR: the binary runs into the level sprites"; exit 1; fi
