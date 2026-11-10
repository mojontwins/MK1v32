#!/bin/sh
# How the Linux z88dk used by dev/comp.sh was made (z88dk 1.10 era):
#
#  1. Compilers from z88dk's git history, at the commit matching the
#     binaries in mk1v32/env/z88dk10-orig.zip (sccz80.exe of 2012-12-16):
git clone --filter=blob:none --no-checkout https://github.com/z88dk/z88dk z88dk-git
cd z88dk-git
git sparse-checkout init --no-cone
printf '/src/\n/Makefile\n/lib/config/\n/include/\n' > .git/info/sparse-checkout
git checkout 49ee3c92f
mkdir -p bin
export CFLAGS="-O2 -fcommon -std=gnu89 -w -DUNIX" CC=gcc
for t in setup appmake copt zcpp sccz80 z80asm zcc; do make -e $t; done
cd ..
#  2. Libraries, includes and the custom crt0 (spec_crt0.asm) from the repo:
unzip mk1v32/env/z88dk10-orig.zip -d "$HOME"
Z=$HOME/z88dk10-orig
mkdir -p $Z/linbin && cp z88dk-git/bin/* $Z/linbin/
cp mk1v32/lib/splib2/splib2f.lib $Z/lib/clibs/
cp mk1v32/lib/splib2/spritepack.h $Z/include/
#  3. Point the .cfg files at the new place:
for f in $Z/lib/config/*.cfg; do
	sed -i -e "s#c:\\\\z88dk10-orig#$Z#gI" -e 's#\\#/#g' -e 's/^COPYCMD.*/COPYCMD\t\tcp/' -e 's/\r$//' $f
done
#  Then: cd dev && Z88DK=$Z ./comp.sh
