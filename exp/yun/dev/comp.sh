#!/bin/sh
# Yun ZX - build script (Linux). comp.bat does the same.
#
# Needs: the MK1 z88dk for Linux in PATH and the MK1 tools built for Linux
# (FreeBASIC) in $MK1_TOOLS: mkts_om, sprcnv2, mapcnv, ene2h (the versions with
# the `bin` option) and png2scr. zx0, bas2tap and bin2tap have Python stand-ins
# in ../tools (set ZX0 to use a native zx0 instead).
#
#   ./comp.sh [1|2]              convert resources + compile + tape of that load (1)
#   ./comp.sh [1|2] justcompile  (justcompile: with the resources last converted)
#   YUNFLAGS=-DYUN_DEBUG_LEVEL=2 ./comp.sh 1 justcompile   (start at the load's level 3)

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
	# Charset + tile maps: tileset.bin, tsx_tmaps.bin, lvlN_tmaps.bin (gfx/speccy/tilesN.spt)
	$T/mkts_om platform=zx mode=scripted in=../gfx/speccy/tiles$YUN_LOAD.spt > /dev/null
	# Fixed sprite cells, in the binary
	# Fixed sprite cells from cell 8 (0-7 were Yun's 16x16 frames), and 16x24 Yun
	$T/sprcnv2 ../gfx/speccy/sprites.png speccy/sprites.h $CELLS nomask from=8 > /dev/null
	$T/sprcnv2 ../gfx/speccy/player.png speccy/player.h 8 nomask tall label=player > /dev/null
	# (an old sprcnv2 ignores tall, from= and label=: stop here, not with a compiler error)
	if ! grep -q "_player_1_a" speccy/player.h || grep -q "_sprite_1_a" speccy/sprites.h; then
		echo "ERROR: sprcnv2 is an old version (no tall / from= / label=): rebuild it from utils/sprcnv2.bas"; exit 1
	fi
	# Levels: lvlN_area.zx0 (map + locks, enemies + hotspots, tile map), lvlN_spr.zx0
	locks=$LOCKS
	for l in $LEVELS; do
		set -- $(echo $l | tr ':' ' ')
		n=$1; wh="$2 $3"; lock=$4
		$T/mapcnv ../map/level$n.map lvl${n}_map.bin $wh 15 10 $lock packed bin maxlocks=$locks screens=$SCREENS > /dev/null
		$T/ene2h ../enems/level$n.ene lvl${n}_ene.h bin=lvl${n}_ene.bin screens=$SCREENS > /dev/null
		cat lvl${n}_map.bin lvl${n}_ene.bin lvl${n}_tmaps.bin > lvl${n}_area.bin
		# The level area must come out exactly SCREENS * 105 + LOCKS * 4 + 128 bytes
		# (old mapcnv / ene2h, without `bin`, write C text here: the game would hang)
		size=$(wc -c < lvl${n}_area.bin); want=$((SCREENS * 105 + locks * 4 + 128))
		if [ $size -ne $want ]; then echo "ERROR: lvl${n}_area.bin is $size bytes, should be $want: are mapcnv and ene2h the versions with \`bin\` (utils/*.bas)?"; exit 1; fi
		$T/sprcnv2 ../gfx/speccy/sprites$n.png lvl${n}_spr.bin 12 nomask bin > /dev/null
		$ZX0 lvl${n}_area.bin lvl${n}_area.zx0 > /dev/null
		$ZX0 lvl${n}_spr.bin lvl${n}_spr.zx0 > /dev/null
		rm lvl${n}_map.bin lvl${n}_ene.bin lvl${n}_ene.h lvl${n}_area.bin lvl${n}_spr.bin
	done
	# Screens (DIRECT_TO_PLAY: the title is the frame; each load has its ending)
	$T/png2scr ../gfx/speccy/title.png title.scr > /dev/null
	$ZX0 title.scr titlec.bin > /dev/null
	$T/png2scr ../gfx/speccy/ending$YUN_LOAD.png ending.scr > /dev/null
	$ZX0 ending.scr endingc.bin > /dev/null
	rm title.scr ending.scr
	$T/png2scr ../gfx/speccy/loading.png loading.bin > /dev/null
fi

echo "Compiling"
zcc +zx -m -vn -unsigned -zorg=24200 -llib/splib2a -o $game.bin churromain.c -DSPECCY -DYUN_LOAD=$YUN_LOAD $YUNFLAGS
$PY ../tools/mktap.py $game$YUN_LOAD.tap $game system/loader.bas loading.bin $game.bin 24200
end=$((24200 + $(wc -c < $game.bin)))
top=$(sed -n 's/^[[:space:]]*#define SPR_VAR_BASE[[:space:]]*\([0-9][0-9]*\)[[:space:]].*/\1/p' yun_level.h)
echo "Output: $game$YUN_LOAD.tap ($(wc -c < $game.bin) bytes, ends at $end; level sprites at $top)"
if [ "$end" -gt "$top" ]; then echo "ERROR: the binary runs into the level sprites"; exit 1; fi
