@echo off
rem cpcdirty: builds cpcdirty.lib (z80asm of the MK1 z88dk 1.10, in PATH).
rem   make.bat            build
rem   make.bat install    build and copy cpcdirty.lib / cpcdirty.h into
rem                       c:\z88dk10-orig (as ..\updatelib.bat does)
cd /d "%~dp0"
del /q *.obj *.err *.sym 2>nul
z80asm -xcpcdirty.lib @cpcdirty.lst
if errorlevel 1 goto :eof
del /q *.obj *.sym 2>nul
if not "%1"=="install" goto :eof
copy cpcdirty.lib c:\z88dk10-orig\lib\clibs\
copy cpcdirty.h c:\z88dk10-orig\include\
