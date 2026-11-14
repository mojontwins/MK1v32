#!/bin/sh
# Sir Ababol 2 CPC - build script (Linux). comp_cpc.bat does the same.
# MK1 v3.2 with the CPC_DIRTY backend (lib/cpcdirty, -lcpcdirty): nibble
# mode 0 (16 pens), MK1's 64 x 192 screen.
#
# Needs: the MK1 z88dk for Linux in PATH (with cpcdirty installed) and the MK1
# tools built for Linux (FreeBASIC) in $MK1_TOOLS: mkts_om, mapcnv, ene2h (the
# versions with the `bin` option). zx0 and cpctbin2sna have Python stand-ins
# in ../tools (set ZX0 to use a native zx0 instead).
#
#   ./comp_cpc.sh [1|2]              convert resources + compile + snapshot of that load (1)
#   ./comp_cpc.sh [1|2] justcompile  (justcompile: with the resources last converted)
#   SA2FLAGS=-DSA2_DEBUG_LEVEL=2 ./comp_cpc.sh 1 justcompile   (start at the load's level 3)

set -e
game=sa2
SA2_LOAD=1
case "$1" in 1|2) SA2_LOAD=$1; shift ;; esac
# Each load: levels (number:tileset part), title of the part
case $SA2_LOAD in
	1) LEVELS="0 1 2"; TITLE=A; CUTS="1 2" ;;
	2) LEVELS="3 4 5"; TITLE=B; CUTS="3 4" ;;
esac
SCREENS=20
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
ZX0=${ZX0:-"$PY ../tools/zx0.py"}

cd "$(dirname "$0")"
mkdir -p cpc

if [ "$1" != "justcompile" ]; then
	echo "Converting resources"
	# Patterns + tile maps: cpc/patterns.bin, cpc/lvlN_tmaps.bin (gfx/tilesN.spt)
	$T/mkts_om platform=cpc mode=scripted in=../gfx/tiles$SA2_LOAD.spt > /dev/null
	$ZX0 cpc/patterns.bin cpc/patterns.zx0 > /dev/null
	# The 22 sprite frames, in the binary
	$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/pal.png mode=sprites in=../gfx/sprites.png out=cpc/sprites.bin max=22 silent > /dev/null
	# Palette (my_inks: the original's 16 inks)
	$T/mkts_om platform=cpc mode=palsasassembly in=../gfx/pal.png prefix=my_inks out=cpc/pal.h silent > /dev/null
	# Levels: cpc/lvlN_area.zx0 (map + lock, enemies + hotspots, tile map)
	for n in $LEVELS; do
		$T/mapcnv ../map/level$n.map cpc/lvl${n}_map.bin 1 20 15 10 99 packed bin maxlocks=1 screens=$SCREENS > /dev/null
		$T/ene2h ../enems/level$n.ene cpc/lvl${n}_ene.h bin=cpc/lvl${n}_ene.bin screens=$SCREENS > /dev/null
		cat cpc/lvl${n}_map.bin cpc/lvl${n}_ene.bin cpc/lvl${n}_tmaps.bin > cpc/lvl${n}_area.bin
		# The level area must come out exactly 20 * 105 + 4 + 128 bytes (old
		# mapcnv / ene2h, without `bin`, write C text here: the game would hang)
		size=$(wc -c < cpc/lvl${n}_area.bin); want=$((SCREENS * 105 + 4 + 128))
		if [ $size -ne $want ]; then echo "ERROR: cpc/lvl${n}_area.bin is $size bytes, should be $want: are mapcnv and ene2h the versions with \`bin\` (utils/*.bas)?"; exit 1; fi
		$ZX0 cpc/lvl${n}_area.bin cpc/lvl${n}_area.zx0 > /dev/null
		rm cpc/lvl${n}_map.bin cpc/lvl${n}_ene.bin cpc/lvl${n}_ene.h cpc/lvl${n}_area.bin
	done
	# Screens: mkts_om superbuffers (64 x 192) in zx0. The title is the frame
	# (DIRECT_TO_PLAY).
	$T/mkts_om platform=cpc cpcmode=0 pal=../gfx/pal.png mode=superbuffer in=../gfx/title$TITLE.png out=cpc/title silent > /dev/null
	$ZX0 cpc/title.bin cpc/titlec.bin > /dev/null
	rm cpc/title.bin
	# Cutscenes (mode 1): the frame, the load's two picture windows (16 x 9
	# cells of 2 bytes x 8 lines) and the font
	$T/mkts_om platform=cpc cpcmode=1 pal=../gfx/cut_pal.png mode=superbuffer in=../gfx/cut_frame.png out=cpc/cut_frame silent > /dev/null
	$ZX0 cpc/cut_frame.bin cpc/cut_frame.zx0 > /dev/null
	for w in $CUTS; do
		$T/mkts_om platform=cpc cpcmode=1 pal=../gfx/cut_pal.png mode=chars in=../gfx/cut_win$w.png out=cpc/cut_win$w size=16,9 silent > /dev/null
		$ZX0 cpc/cut_win$w.bin cpc/cut_win$w.zx0 > /dev/null
	done
	$T/mkts_om platform=cpc cpcmode=1 pal=../gfx/cut_pal.png mode=chars in=../gfx/cut_font.png out=cpc/cut_font size=64,1 silent > /dev/null
	$ZX0 cpc/cut_font.bin cpc/cut_font.zx0 > /dev/null
	rm -f cpc/cut_frame.bin cpc/cut_win*.bin cpc/cut_font.bin
	# Songs (zx0 of the player's format, unpacked to WYZ_SONG_BUFFER)
	for m in ../ogt/*.mus; do
		$ZX0 "$m" "$m.bin" > /dev/null
	done
fi

echo "Compiling"
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcdirty -o ${game}.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY -DSA2_LOAD=$SA2_LOAD $SA2FLAGS
$PY ../tools/mksna.py ${game}.bin 0x400 -pc 0x400 -o $game$SA2_LOAD.sna > /dev/null
size=$(wc -c < ${game}.bin)
# Songs are unpacked to WYZ_SONG_BUFFER (sa2_level.h), which must leave room
# up to 9000 for the load's biggest .mus; the binary must end below it.
buf=$(sed -n "s/^[[:space:]]*#define SA2_SONG_BUFFER_$SA2_LOAD[[:space:]]*0x\([0-9A-Fa-f]*\).*/\1/p" sa2_level.h)
mus=$(sed -n "s/^[[:space:]]*#define SA2_SONG_MAX_$SA2_LOAD[[:space:]]*\([0-9]*\).*/\1/p" sa2_level.h)
echo "Output: $game$SA2_LOAD.sna ($size bytes, 0400-$(printf %04X $((1024 + size))), must end below $buf: $((0x$buf - 1024 - size)) bytes free; biggest .mus $mus bytes)"
if [ $((1024 + size)) -gt $((0x$buf)) ]; then echo "ERROR: the binary runs into the song buffer at $buf"; exit 1; fi
if [ $((0x$buf + mus)) -gt $((0x9000)) ]; then echo "ERROR: the biggest .mus doesn't fit between $buf and 9000"; exit 1; fi

# Tape: system/sa2pre.asm (firmware file, RUN"") loads system/sa2loader.asm
# (raw) at BA00, which loads the game (zx0, raw) and unpacks it to 0400.
# cpctape and cpcdsk are MK1 tools (utils/*.bas); the loaders are z80asm's.
cp ${game}.bin ${game}$SA2_LOAD.bin
$ZX0 ${game}.bin cpc/main.zx0 > /dev/null
MAIN=$(wc -c < cpc/main.zx0)
printf "\tdefc MAIN_SIZE = %d\n\tdefc LOADER_SIZE = 0\n" $MAIN > system/sizes.inc
(cd system && z80asm -b sa2loader.asm)
LDR=$(wc -c < system/sa2loader.bin)
printf "\tdefc MAIN_SIZE = %d\n\tdefc LOADER_SIZE = %d\n" $MAIN $LDR > system/sizes.inc
(cd system && z80asm -b sa2pre.asm)
$T/cpctape in=system/sa2pre.bin out=$game$SA2_LOAD.cdt mode=fw name=SIR-ABABOL-2-$SA2_LOAD new > /dev/null
$T/cpctape in=system/sa2loader.bin out=$game$SA2_LOAD.cdt mode=raw pause=100 > /dev/null
$T/cpctape in=cpc/main.zx0 out=$game$SA2_LOAD.cdt mode=raw > /dev/null
rm -f system/*.obj system/*.map system/*.sym system/*.err
echo "Output: $game$SA2_LOAD.cdt (RUN\"\")"

# Disk: both loads (RUN"SA21, RUN"SA22), once both have been built
if [ -f ${game}1.bin ] && [ -f ${game}2.bin ]; then
	$T/cpcdsk out=$game.dsk file1=${game}1.bin,SA21.BIN,0x400,0x400 file2=${game}2.bin,SA22.BIN,0x400,0x400 > /dev/null
	echo "Output: $game.dsk (RUN\"SA21 / RUN\"SA22)"
fi
