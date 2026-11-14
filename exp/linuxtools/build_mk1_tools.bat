@echo off
rem Windows equivalent of build_mk1_tools.sh: rebuild the MK1 v3.2 resource
rem tools from their .bas sources with FreeBASIC for Windows.
rem On Windows no shims are needed: utils\ already has libfbpng.a and zlib1.dll.
rem
rem   build_mk1_tools.bat MK1_UTILS_DIR OUT_DIR
rem   (fbc must be in the PATH)

if [%2]==[] (
	echo Usage: build_mk1_tools.bat MK1_UTILS_DIR OUT_DIR
	exit /b 1
)
if not exist %2 mkdir %2
pushd %1
fbc -x %2\ts2bin.exe  ts2bin.bas
fbc -x %2\sprcnv2.exe sprcnv2.bas
fbc -x %2\png2scr.exe png2scr.bas
fbc -x %2\mapcnv.exe  mapcnv.bas
fbc -x %2\ene2h.exe   ene2h.bas mtparser.bas cmdlineparser.bas
for %%t in (cpctape cpcdsk) do if exist %%t.bas fbc -x %2\%%t.exe %%t.bas cmdlineparser.bas
fbc -x %2\mkts_om.exe mkts_om.bas cmdlineparser.bas mtparser.bas
for %%r in (rle44map_sp rle53map_sp rle62map_sp) do if exist %%r.bas fbc -x %2\%%r.exe %%r.bas cmdlineparser.bas mtparser.bas
popd
