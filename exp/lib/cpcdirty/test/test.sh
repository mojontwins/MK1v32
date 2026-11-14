#!/bin/sh
# cpcdirty test bench (Linux): builds test_cd.c in the three modes and the
# two screen widths (64: viewport at 1, 2; 80: at 5, 1) against
# ../cpcdirty.lib, runs each in cpcheadless and checks the screen after every
# step with check.py. Needs the MK1 z88dk in PATH, python3 and cpcheadless
# (CPCHEADLESS=path). test.bat only builds (no emulator on Windows).
set -e
cd "$(dirname "$0")"
E=${CPCHEADLESS:-cpcheadless}
PY=${PYTHON:-python3}
(cd .. && ./make.sh install > /dev/null)
for cfg in "64 1 2" "80 5 1"; do
	set -- $cfg; w=$1; vx=$2; vy=$3
	for m in N0 M1 P0; do
		l=$(echo $m | tr A-Z a-z)_$w
		zcc +cpc -vn -unsigned -zorg=1024 -lcpcdirty -o test_$l.bin test_cd.c -DCD_MODE_$m -DCD_SCRW=$w -DCD_VPX=$vx -DCD_VPY=$vy
		$PY mksna.py test_$l.bin 0x400 -pc 0x400 -sp 0x8f00 -o test_$l.sna > /dev/null
		# when do the steps happen? (the step counter at 8600, every frame)
		args=""; f=0; while [ $f -lt 1300 ]; do args="$args -dump 0x8600:1@$f"; f=$((f + 1)); done
		$E -sna test_$l.sna -f 1300 $args 2> /dev/null > steps_$l.txt
		args=$($PY -c "
prev = 0; out = []
for l in open('steps_$l.txt'):
	if not l.startswith('frame '): continue
	f = int(l.split()[1]); v = int(l.split(':')[1], 16)
	if v != prev: out.append('-dump 0:65536@%d' % (f + 20)); prev = v
print(' '.join(out))")
		$E -sna test_$l.sna -f 1300 $args 2> /dev/null > dump_$l.txt
		printf "%s: " "$l"; $PY check.py $(echo $m | tr A-Z a-z) dump_$l.txt $w $vx $vy | tail -1
	done
done
rm -f test_*.bin test_*.sna steps_*.txt dump_*.txt zcc_opt.def
