@echo off
rem Rebuilds the MK1 tools Nin Nin needs patched (FreeBASIC for Windows, fbc in PATH):
rem rle44map_sp (keepempty, maxlocks) and ene2h (bin, screens).
cd /d %~dp0
fbc rle44map_sp.bas cmdlineparser.bas mtparser.bas || goto :error
fbc ene2h.bas cmdlineparser.bas mtparser.bas || goto :error
echo Done: rle44map_sp.exe ene2h.exe
goto :eof
:error
echo ERROR
exit /b 1
