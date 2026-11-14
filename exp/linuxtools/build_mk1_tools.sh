#!/bin/sh
# Build the MTE MK1 v3.2 resource tools for Linux x86-64.
#
#   ./build_mk1_tools.sh FREEBASIC_DIR MK1_UTILS_DIR OUT_DIR
#
#   FREEBASIC_DIR  FreeBASIC-1.10.1-linux-x86_64 (unpacked)
#   MK1_UTILS_DIR  mk1v32/src/utils (the .bas/.bi sources)
#   OUT_DIR        where the programs go (ts2bin, sprcnv2, png2scr, mapcnv, ene2h, mkts_om,
#                  rle44map_sp, rle53map_sp, rle62map_sp, cpctape, cpcdsk)
#
# Two shims are built first:
#  * libtinfo.so.5: this fbc binary wants ncurses 5 symbols; modern systems
#    have libtinfo.so.6. The shim forwards the five calls fbc uses.
#  * libfbpng.a: the fbpng in utils/ is a Windows library. This one implements
#    png_load / png_save / png_dimensions / png_destroy on top of libpng and
#    returns FB "new style" images created with fb_GfxImageCreate (so the
#    tools can ImageDestroy them), which is all the MK1 tools use.
# Needs gcc, libpng-dev, and libX11/libXext/libXpm/libXrandr/libXrender
# (fbgfx links them; the tools never open a window: screenres ... -1 = GFX_NULL).

set -e
FB="$1"; SRC="$2"; OUT="$3"
[ -d "$FB" ] && [ -d "$SRC" ] && [ -n "$OUT" ] || { echo "usage: $0 FREEBASIC_DIR MK1_UTILS_DIR OUT_DIR"; exit 1; }
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$OUT/lib" "$OUT/src"
LIB="$(cd "$OUT/lib" && pwd)"

gcc -shared -fPIC -o "$LIB/libtinfo.so.5" "$HERE/tinfo5.c" \
	-Wl,--version-script="$HERE/tinfo5.map" -Wl,-soname,libtinfo.so.5 -ldl
gcc -O2 -c "$HERE/fbpng.c" -o "$LIB/fbpng.o"
ar rcs "$LIB/libfbpng.a" "$LIB/fbpng.o"
cp "$LIB/libfbpng.a" "$LIB/libfbpngs.a"
for l in Xpm Xrandr; do
	[ -e "/usr/lib/x86_64-linux-gnu/lib$l.so" ] || ln -sf "$(ls /usr/lib/x86_64-linux-gnu/lib$l.so.* | head -1)" "$LIB/lib$l.so"
done

cp "$SRC"/*.bas "$SRC"/*.bi "$OUT/src/"
cd "$OUT/src"
FBC="env LD_LIBRARY_PATH=$LIB $FB/bin/fbc -p $LIB"
$FBC -l png16 -x ../ts2bin  ts2bin.bas
$FBC -l png16 -x ../sprcnv2 sprcnv2.bas
$FBC -l png16 -x ../png2scr png2scr.bas
$FBC -x ../mapcnv mapcnv.bas
$FBC -x ../ene2h  ene2h.bas mtparser.bas cmdlineparser.bas
for t in cpctape cpcdsk; do
	if [ -f $t.bas ]; then $FBC -x ../$t $t.bas cmdlineparser.bas; fi
done
$FBC -l png16 -x ../mkts_om mkts_om.bas cmdlineparser.bas mtparser.bas
for r in rle44map_sp rle53map_sp rle62map_sp; do
	[ -f $r.bas ] && $FBC -x ../$r $r.bas cmdlineparser.bas mtparser.bas
done
echo "Built: $(cd .. && ls ts2bin sprcnv2 png2scr mapcnv ene2h mkts_om rle*map_sp cpctape cpcdsk 2>/dev/null | tr '\n' ' ')"
# Note: Linux is case sensitive and comp.bat files were written on Windows:
# check the case of map/enems/gfx file names (games/papaya/map has mapa.MAP).
