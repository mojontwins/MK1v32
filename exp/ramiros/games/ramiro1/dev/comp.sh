#!/bin/sh
# MTE MK1 v3.2 - Ramiro el Vampiro en "No tiene un respiro" - Linux build
# (Python ports of the tools in ../utils/py + z88dk 1.10 built for Linux).
#
#   ./comp.sh              export the .apor, convert everything, compile, make the .tap
#   ./comp.sh justcompile  only recompile the script and the C code
#
# Set Z88DK to the z88dk10 directory (with linbin/, lib/, include/).

set -e
cd "$(dirname "$0")"

game=ramiro1
PY="python3 -I"
U=../utils/py
Z88DK=${Z88DK:-$HOME/z88dk10-orig}
export PATH="$Z88DK/linbin:$Z88DK/bin:$PATH"
export ZCCCFG="$Z88DK/lib/config/"
export Z80_OZFILES="$Z88DK/lib/"

echo "Making $game"

if [ "$1" != "justcompile" ]; then
	# Map, enemies and tile behaviours come from the Aportador project.
	$PY $U/apor2mk1.py ../apor/$game.apor
	# Copy the behaviours exported by Aportador into config.h
	$PY - <<'EOF'
import re
behs = open('../map/behs.txt').read().strip().split(',')[:48]
behs = [b.strip() for b in behs]
rows = [', '.join(behs[i:i + 16]) for i in range(0, 48, 16)]
new = 'unsigned char comportamiento_tiles [] = {\n\t' + ',\n\t'.join(rows) + '\n};'
s = open('config.h').read()
s2 = re.sub(r'unsigned char comportamiento_tiles \[\] = \{.*?\};', new, s, flags=re.S)
if s2 != s:
	open('config.h', 'w').write(s2)
	print('config.h: comportamiento_tiles updated from behs.txt')
EOF

	$PY $U/mk1data.py rle53map in=../map/mapa.MAP out=mapa.bin size=15,2 scrsize=15,10 mk1h=mapa.h
	$PY $U/mk1data.py ene2h ../enems/enems.ene enems.h 2bytes

	$PY $U/zxgfx.py ts2bin ../gfx/speccy/font.png ../gfx/speccy/work.png tileset.bin 7
	$PY $U/zxgfx.py sprcnv ../gfx/speccy/sprites.png speccy/sprites.h
	$PY $U/zxgfx.py png2scr ../gfx/speccy/title.png ../gfx/speccy/title.scr
	$PY $U/zxgfx.py png2scr ../gfx/speccy/ending.png ../gfx/speccy/ending.scr
	$PY $U/zxgfx.py png2scr ../gfx/speccy/loading.png loading.bin
	$PY $U/zx0.py ../gfx/speccy/title.scr titlec.bin
	$PY $U/zx0.py ../gfx/speccy/ending.scr endingc.bin
fi

echo "Making script"
cd ../script
$PY ../utils/py/msc4.py in=$game.spt v=3 target=zx interpreter=msc4i.asm text=text.bin debug > ../dev/msc.txt || { tail -20 ../dev/msc.txt; exit 1; }
cp $game.spt.bin ../dev/script.spt.bin
cp text.bin ../dev/
mv msc4i.asm ../dev/
cd ../dev

rm -f zcc_opt.def
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o $game.bin msc4i.asm churromain.c -DSPECCY
$PY $U/mk1data.py printsize $game.bin
$PY $U/mk1data.py mktap $game.tap $game system/loader.bas loading.bin:32768 $game.bin:24200
mkdir -p ../show
cp $game.tap ../show/
echo "Output: ../show/$game.tap"
