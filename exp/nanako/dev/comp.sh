#!/bin/sh
# Nanako in Classic Japanese Monster Castle - ZX Spectrum 48K / 128K build (Linux). comp.bat does the same.
#
# Needs: the MK1 z88dk for Linux in PATH (linuxtools/make_z88dk_mk1.sh) and the MK1
# tools built for Linux in $MK1_TOOLS (ene2h, sfxcnv, ts2bin, sprcnv2, png2scr).
# zx0 and the .tap builder are the Python versions in ../tools.
#
#   ./comp.sh               convert resources + compile + tape
#   ./comp.sh justcompile

set -e
game=nanako
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}

cd "$(dirname "$0")"

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	$T/ene2h ../enems/enems.ene enems.h 2bytes > /dev/null
	$T/sfxcnv in=../sfx/sfx.txt out=sfx.h > /dev/null
	# 006: default ink yellow for one colour tiles (ts2bin ignores it if shorter than 3 chars)
	$T/ts2bin ../gfx/speccy/font.png ../gfx/speccy/work.png tileset.bin 006 > /dev/null
	$T/sprcnv2 ../gfx/speccy/sprites.png speccy/sprites.h 19 > /dev/null
	for s in title marco ending; do
		$T/png2scr ../gfx/speccy/$s.png ../gfx/speccy/$s.scr > /dev/null
		$PY ../tools/zx0.py ../gfx/speccy/$s.scr ${s}c.bin > /dev/null
	done
	$T/png2scr ../gfx/speccy/loading.png loading.bin > /dev/null
	rm -f ../gfx/speccy/*.scr
fi

echo "Compiling"
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o $game.bin churromain.c -DSPECCY
echo "$game.bin: $(wc -c < $game.bin) bytes, 24200-$((24200 + $(wc -c < $game.bin)))"

# 128K: the WYZ player, songs and effects for RAM1 (C000)
(cd ../ogt/ram1 && z80asm -b ram1.asm && rm -f ram1.obj ram1.err)
echo "ram1.bin: $(wc -c < ../ogt/ram1/ram1.bin) bytes, C000"

# Tape: BASIC, system/zxloader.asm (at 24000), then headerless: screen, game
# and RAM1 (only loaded on a 128K). Same blocks as comp.bat's GenTape.
printf "\tdefc MAIN_SIZE = %d\n\tdefc RAM1_SIZE = %d\n" $(wc -c < $game.bin) $(wc -c < ../ogt/ram1/ram1.bin) > system/zxsizes.inc
(cd system && z80asm -b zxloader.asm && rm -f zxloader.obj zxloader.err)
$PY ../tools/gentape.py $game.tap \
	basic "'$game'" 10 system/loader128.tok \
	hdata "'loader'" 5DC0 system/zxloader.bin \
	data loading.bin \
	data $game.bin \
	data ../ogt/ram1/ram1.bin
echo "Output: $game.tap (48K / 128K)"
