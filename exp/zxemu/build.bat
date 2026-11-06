@echo off
rem zxheadless / cpcheadless - build with gcc (MinGW / MSYS2). Same as the Makefile.
cd /d %~dp0
gcc -O2 -Wall -o zxheadless.exe zxheadless.c z80.c || goto :error
gcc -O2 -Wall -o cpcheadless.exe cpcheadless.c z80.c || goto :error
echo OK
goto :eof
:error
echo ERROR
exit /b 1
