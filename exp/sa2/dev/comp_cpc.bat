@echo off
rem Sir Ababol 2 CPC - build script (Windows). comp_cpc.sh does the same.
rem MK1 v3.2 with the CPC_DIRTY backend (lib\cpcdirty, -lcpcdirty): nibble
rem mode 0 (16 pens), MK1's 64 x 192 screen.
rem
rem Needs: MK1's z88dk (env\setenv.bat) with cpcdirty installed
rem (lib\updatelib.bat) and the MK1 tools in ..\utils. mapcnv and ene2h must be
rem the versions with the `bin` option (utils\*.bas: compile them with fbc).
rem
rem   comp_cpc.bat [1|2]              convert resources + compile + snapshot of that load (1)
rem   comp_cpc.bat [1|2] justcompile  (justcompile: with the resources last converted)
rem   set SA2FLAGS=-DSA2_DEBUG_LEVEL=2 then comp_cpc.bat 1 justcompile   (start at the load's level 3)

setlocal enabledelayedexpansion
set game=sa2
set SA2_LOAD=1
set JC=%1
if "%1"=="1" (set SA2_LOAD=1& set JC=%2)
if "%1"=="2" (set SA2_LOAD=2& set JC=%2)
rem Each load: levels, title of the part, cutscene windows
if "%SA2_LOAD%"=="1" (set LEVELS=0 1 2& set TITLE=A& set CUTS=1 2) else (set LEVELS=3 4 5& set TITLE=B& set CUTS=3 4)
set SCREENS=20
cd /d %~dp0
if not exist cpc mkdir cpc
set BADLEVEL=

if [%JC%]==[justcompile] goto :compile

echo Converting resources
rem Patterns + tile maps: cpc\patterns.bin, cpc\lvlN_tmaps.bin (gfx\tilesN.spt)
..\utils\mkts_om.exe platform=cpc mode=scripted in=..\gfx\tiles%SA2_LOAD%.spt > nul
call :zx0 cpc\patterns.bin cpc\patterns.zx0
rem The 22 sprite frames, in the binary
..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\pal.png mode=sprites in=..\gfx\sprites.png out=cpc\sprites.bin max=22 silent > nul
rem Palette (my_inks: the original's 16 inks)
..\utils\mkts_om.exe platform=cpc mode=palsasassembly in=..\gfx\pal.png prefix=my_inks out=cpc\pal.h silent > nul
rem Levels: cpc\lvlN_area.zx0 (map + lock, enemies + hotspots, tile map)
for %%n in (%LEVELS%) do call :level %%n
if defined BADLEVEL goto :error
rem Screens: mkts_om superbuffers (64 x 192) in zx0. The title is the frame
rem (DIRECT_TO_PLAY).
..\utils\mkts_om.exe platform=cpc cpcmode=0 pal=..\gfx\pal.png mode=superbuffer in=..\gfx\title%TITLE%.png out=cpc\title silent > nul
call :zx0 cpc\title.bin cpc\titlec.bin
del cpc\title.bin
rem Cutscenes (mode 1): the frame, the load's two picture windows (16 x 9
rem cells of 2 bytes x 8 lines) and the font
..\utils\mkts_om.exe platform=cpc cpcmode=1 pal=..\gfx\cut_pal.png mode=superbuffer in=..\gfx\cut_frame.png out=cpc\cut_frame silent > nul
call :zx0 cpc\cut_frame.bin cpc\cut_frame.zx0
for %%w in (%CUTS%) do (
	..\utils\mkts_om.exe platform=cpc cpcmode=1 pal=..\gfx\cut_pal.png mode=chars in=..\gfx\cut_win%%w.png out=cpc\cut_win%%w size=16,9 silent > nul
	call :zx0 cpc\cut_win%%w.bin cpc\cut_win%%w.zx0
)
..\utils\mkts_om.exe platform=cpc cpcmode=1 pal=..\gfx\cut_pal.png mode=chars in=..\gfx\cut_font.png out=cpc\cut_font size=64,1 silent > nul
call :zx0 cpc\cut_font.bin cpc\cut_font.zx0
del cpc\cut_frame.bin cpc\cut_win*.bin cpc\cut_font.bin
rem Songs (zx0 of the player's format, unpacked to WYZ_SONG_BUFFER)
for %%m in (..\ogt\*.mus) do call :zx0 %%m %%m.bin

:compile
echo Compiling
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcdirty -o %game%.bin churromain.c -DCPC -DMODE_0 -DCPC_DIRTY -DSA2_LOAD=%SA2_LOAD% %SA2FLAGS% || goto :error
..\utils\printsize.exe %game%.bin
if exist %game%%SA2_LOAD%.sna del %game%%SA2_LOAD%.sna
..\utils\cpctbin2sna.exe %game%.bin 0x400 -pc 0x400 -o %game%%SA2_LOAD%.sna > nul
rem Songs are unpacked to WYZ_SONG_BUFFER (sa2_level.h), which must leave room
rem up to 9000 for the load's biggest .mus; the binary must end below it.
for /f "tokens=3" %%a in ('findstr /c:"#define SA2_SONG_BUFFER_%SA2_LOAD%" sa2_level.h') do set BUF=%%a
for /f "tokens=3" %%a in ('findstr /c:"#define SA2_SONG_MAX_%SA2_LOAD%" sa2_level.h') do set MUS=%%a
for %%I in (%game%.bin) do set SIZE=%%~zI
set /a END=1024+SIZE
set /a LIMIT=%BUF%
set /a FREE=LIMIT-END
echo Output: %game%%SA2_LOAD%.sna (%SIZE% bytes, ends at %END%, must end below %LIMIT% = %BUF%: %FREE% bytes free; biggest .mus %MUS% bytes)
if %END% GTR %LIMIT% (
	echo ERROR: the binary runs into the song buffer
	exit /b 1
)
set /a TOP=LIMIT+MUS
if %TOP% GTR 36864 (
	echo ERROR: the biggest .mus does not fit between %BUF% and 9000
	exit /b 1
)

rem Tape: system\sa2pre.asm (firmware file, RUN"") loads system\sa2loader.asm
rem (raw) at BA00, which loads the game (zx0, raw) and unpacks it to 0400.
rem cpctape.exe and cpcdsk.exe are MK1 tools (utils\*.bas: compile them with
rem fbc cpctape.bas cmdlineparser.bas); the loaders are z80asm's.
copy /b %game%.bin %game%%SA2_LOAD%.bin > nul
call :zx0 %game%.bin cpc\main.zx0
for %%I in (cpc\main.zx0) do set MAIN=%%~zI
(echo 	defc MAIN_SIZE = %MAIN%& echo 	defc LOADER_SIZE = 0) > system\sizes.inc
pushd system
z80asm -b sa2loader.asm || (popd & goto :error)
popd
for %%I in (system\sa2loader.bin) do set LDR=%%~zI
(echo 	defc MAIN_SIZE = %MAIN%& echo 	defc LOADER_SIZE = %LDR%) > system\sizes.inc
pushd system
z80asm -b sa2pre.asm || (popd & goto :error)
popd
..\utils\cpctape.exe in=system\sa2pre.bin out=%game%%SA2_LOAD%.cdt mode=fw name=SIR-ABABOL-2-%SA2_LOAD% new > nul
..\utils\cpctape.exe in=system\sa2loader.bin out=%game%%SA2_LOAD%.cdt mode=raw pause=100 > nul
..\utils\cpctape.exe in=cpc\main.zx0 out=%game%%SA2_LOAD%.cdt mode=raw > nul
del system\*.obj system\*.map system\*.sym system\*.err 2> nul
echo Output: %game%%SA2_LOAD%.cdt (RUN"")

rem Disk: both loads (RUN"SA21, RUN"SA22), once both have been built
if exist %game%1.bin if exist %game%2.bin (
	..\utils\cpcdsk.exe out=%game%.dsk file1=%game%1.bin,SA21.BIN,0x400,0x400 file2=%game%2.bin,SA22.BIN,0x400,0x400 > nul
	echo Output: %game%.dsk ^(RUN"SA21 / RUN"SA22^)
)
goto :end

:zx0
if exist %2 del %2
..\utils\zx0.exe %1 %2 > nul
goto :eof

:level
..\utils\mapcnv.exe ..\map\level%1.map cpc\lvl%1_map.bin 1 20 15 10 99 packed bin maxlocks=1 screens=%SCREENS% > nul
..\utils\ene2h.exe ..\enems\level%1.ene cpc\lvl%1_ene.h bin=cpc\lvl%1_ene.bin screens=%SCREENS% > nul
copy /b cpc\lvl%1_map.bin + cpc\lvl%1_ene.bin + cpc\lvl%1_tmaps.bin cpc\lvl%1_area.bin > nul
rem The level area must come out exactly 20 * 105 + 4 + 128 bytes (old mapcnv /
rem ene2h, without `bin`, write C text here: the game would hang)
for %%I in (cpc\lvl%1_area.bin) do set ASIZE=%%~zI
set /a AWANT=SCREENS*105+4+128
if not "%ASIZE%"=="%AWANT%" (
	echo ERROR: cpc\lvl%1_area.bin is %ASIZE% bytes, should be %AWANT%: are mapcnv.exe and ene2h.exe the versions with "bin" ^(utils\*.bas^)?
	set BADLEVEL=1
)
call :zx0 cpc\lvl%1_area.bin cpc\lvl%1_area.zx0
del cpc\lvl%1_map.bin cpc\lvl%1_ene.bin cpc\lvl%1_ene.h cpc\lvl%1_area.bin
goto :eof

:error
echo ERROR
exit /b 1

:end
