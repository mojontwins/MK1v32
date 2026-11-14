@echo off
rem cpcdirty test bench (Windows): builds test_cd.c in the three modes and the
rem two screen widths against ..\cpcdirty.lib (installed with
rem ..\make.bat install). Running and checking them needs cpcheadless and
rem python: see test.sh.
cd /d "%~dp0"
call ..\make.bat install
for %%m in (N0 M1 P0) do (
	zcc +cpc -vn -unsigned -zorg=1024 -lcpcdirty -o test_%%m_64.bin test_cd.c -DCD_MODE_%%m -DCD_SCRW=64 -DCD_VPX=1 -DCD_VPY=2
	python mksna.py test_%%m_64.bin 0x400 -pc 0x400 -sp 0x8f00 -o test_%%m_64.sna
	zcc +cpc -vn -unsigned -zorg=1024 -lcpcdirty -o test_%%m_80.bin test_cd.c -DCD_MODE_%%m -DCD_SCRW=80 -DCD_VPX=5 -DCD_VPY=1
	python mksna.py test_%%m_80.bin 0x400 -pc 0x400 -sp 0x8f00 -o test_%%m_80.sna
)
