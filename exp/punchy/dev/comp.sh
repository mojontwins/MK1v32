#!/bin/sh
# Punchy (MK1 v3.2) - build script (Linux). comp.bat does the same on Windows.
#
#   ./comp.sh zx|cpc              convert resources + compile + tape / snapshot
#   ./comp.sh zx|cpc justcompile
#   PNFLAGS="-DPN_TEST_SCR=1 -DPN_TEST_X=4" ./comp.sh cpc justcompile   (start elsewhere, to test)
#
# Needs: the MK1 z88dk in PATH and the MK1 tools (mkts_om, mapcnv, ene2h,
# png2scr) in $MK1_TOOLS (default ../utils). zx0, bas2tap/bin2tap and
# cpctbin2sna have Python stand-ins in ../tools (set ZX0 to use a native zx0).

set -e
game=punchy
T=${MK1_TOOLS:-../utils}
PY=${PYTHON:-python3}
ZX0=${ZX0:-"$PY ../tools/zx0.py"}
PF=$1
[ "$PF" = zx ] || [ "$PF" = cpc ] || { echo "usage: $0 zx|cpc [justcompile]"; exit 1; }

cd "$(dirname "$0")"

if [ "$2" != "justcompile" ]; then
	echo "Converting resources ($PF)"
	# Map (4 x 1 screens, no locks) and enemies (dsall: warpers keep their
	# destination in x2, y2)
	$T/mapcnv ../map/level0.map mapa.h 4 1 15 10 99 packed > /dev/null
	$T/ene2h ../enems/level0.ene enems.h dsall > /dev/null
	if [ "$PF" = zx ]; then
		G=../gfx/speccy
		$T/mkts_om platform=zx mode=scripted in=$G/tiles.spt > /dev/null
		# Sprites: 16x32 (metasize 2,4), pieces 8x16 (1,2), cocos 8x8 (1,1)
		M="platform=zx mode=sprites silent"
		$T/mkts_om $M in=$G/spr_tall.png out=speccy/spr_tall metasize=2,4 size=8,4 max=27 > /dev/null
		$T/mkts_om $M in=$G/spr_boom.png out=speccy/spr_boom metasize=2,4 size=1,1 > /dev/null
		$T/mkts_om $M in=$G/spr_empty.png out=speccy/spr_empty metasize=2,4 size=1,1 > /dev/null
		$T/mkts_om $M in=$G/spr_piece.png out=speccy/spr_piece metasize=1,2 size=3,1 > /dev/null
		$T/mkts_om $M in=$G/spr_piece_empty.png out=speccy/spr_piece_empty metasize=1,2 size=1,1 > /dev/null
		$T/mkts_om $M in=$G/spr_coco.png out=speccy/spr_coco metasize=1,1 size=1,1 > /dev/null
		for s in title marco ending; do
			$T/png2scr $G/$s.png $s.scr > /dev/null
			$ZX0 $s.scr ${s}c.bin > /dev/null
			rm $s.scr
		done
		$T/png2scr $G/loading.png loading.bin > /dev/null
	else
		G=../gfx/cpc
		$T/mkts_om platform=cpc mode=scripted in=$G/tiles.spt > /dev/null
		$ZX0 cpc/patterns.bin cpc/patterns.zx0 > /dev/null
		M="platform=cpc cpcmode=0 pal=$G/pal_cnvspr.png mode=sprites silent"
		$T/mkts_om $M in=$G/spr_tall.png out=cpc/spr_tall metasize=2,4 size=8,4 max=27 > /dev/null
		$T/mkts_om $M in=$G/spr_boom.png out=cpc/spr_boom metasize=2,4 size=1,1 > /dev/null
		$T/mkts_om $M in=$G/spr_piece.png out=cpc/spr_piece metasize=1,2 size=3,1 > /dev/null
		$T/mkts_om $M in=$G/spr_coco.png out=cpc/spr_coco metasize=1,1 size=1,1 > /dev/null
		# Palettes: pal.png (my_inks: titles, frame), background and sprite inks
		$T/mkts_om platform=cpc mode=palsasassembly in=$G/pal.png prefix=my_inks out=cpc/pal silent > /dev/null
		$T/mkts_om platform=cpc mode=pals in=$G/pal_cnvtiles.png prefix=bg_pal out=cpc/bg_pal silent > /dev/null
		$T/mkts_om platform=cpc mode=pals in=$G/pal_cnvspr.png prefix=spr_pal out=cpc/spr_pal silent > /dev/null
		for s in title marco ending; do
			$T/mkts_om platform=cpc cpcmode=0 pal=$G/pal.png mode=superbuffer in=$G/$s.png out=cpc/$s silent > /dev/null
			$ZX0 cpc/$s.bin cpc/${s}c.bin > /dev/null
			rm cpc/$s.bin
		done
	fi
fi

echo "Compiling ($PF)"
if [ "$PF" = zx ]; then
	zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o $game.bin churromain.c -DSPECCY $PNFLAGS
	$PY ../tools/mktap.py $game.tap $game system/loader.bas loading.bin $game.bin 24200
	end=$((24200 + $(wc -c < $game.bin)))
	echo "Output: $game.tap ($(wc -c < $game.bin) bytes, ends at $end; must end below 59392)"
	if [ "$end" -gt 59392 ]; then echo "ERROR: the binary runs into the flip table (E800)"; exit 1; fi
else
	zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o ${game}c.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY $PNFLAGS
	$PY ../tools/mksna.py ${game}c.bin 0x400 -pc 0x400 -o $game.sna > /dev/null
	size=$(wc -c < ${game}c.bin)
	echo "Output: $game.sna ($size bytes, 0400-$(printf %04X $((1024 + size))), must end below 8800)"
	if [ $((1024 + size)) -gt $((0x8800)) ]; then echo "ERROR: the binary runs into 8800"; exit 1; fi
fi
