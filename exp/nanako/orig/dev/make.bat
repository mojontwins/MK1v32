@echo off
cls
echo **************
echo * COMPILANDO *
echo **************
rem zcc +cpc TileMapConf.asm -create-app -O3 -unsigned -o %1.bin %1.c -lcpcrslib -lcpcwyzlib -zorg=8900
zcc +cpc TileMapConf.asm -O3 -unsigned -o %1.bin %1.c -lcpcrslib -lcpcwyzlib -zorg=8192

echo.
echo ************
echo * COPIANDO *
echo ************
copy %1.bin ..\..\WinAPE
echo OK!


