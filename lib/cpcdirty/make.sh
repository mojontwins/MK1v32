#!/bin/sh
# cpcdirty: builds cpcdirty.lib (z80asm of the MK1 z88dk 1.10, in PATH).
#   ./make.sh            build
#   ./make.sh install    build and copy cpcdirty.lib / cpcdirty.h into z88dk
#                        ($Z80_OZFILES/clibs and $Z80_OZFILES/../include)
# make.bat does the same on Windows.
set -e
cd "$(dirname "$0")"
rm -f *.obj *.err *.sym
z80asm -xcpcdirty.lib @cpcdirty.lst
rm -f *.obj *.sym
echo "cpcdirty.lib: $(wc -c < cpcdirty.lib) bytes"
if [ "$1" = "install" ]; then
	cp cpcdirty.lib "${Z80_OZFILES%/}/clibs/"
	cp cpcdirty.h "${Z80_OZFILES%/}/../include/"
	echo "installed in ${Z80_OZFILES%/}"
fi
