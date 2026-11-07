#!/bin/sh
# Builds the MK1 tools the Nin Nin build needs patched, for Linux (FreeBASIC:
# fbc in PATH; if it needs libtinfo.so.5 etc, LD_LIBRARY_PATH to them). Output
# in ${MK1_TOOLS:-.}: rle44map_sp (keepempty, maxlocks), ene2h (bin, screens).
# mkts_om is the stock one.
set -e
cd "$(dirname "$0")"
out=${MK1_TOOLS:-.}
fbc rle44map_sp.bas cmdlineparser.bas mtparser.bas -x $out/rle44map_sp
fbc ene2h.bas cmdlineparser.bas mtparser.bas -x $out/ene2h
echo "Done: $out/rle44map_sp $out/ene2h"
