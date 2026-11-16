@echo off
rem Writes wav\<name>.wav for each effect in sfx.txt, with the player's timing.
rem preview.sh does the same.
rem
rem   preview.bat [idle] [slice]
rem
rem idle: % of the frame the game leaves idle (100 = the sound as it is; Uwol
rem leaves ~10% in the emulator). slice: SFX_MIN_SLICE in config.h.

cd /d %~dp0
if not exist wav mkdir wav
set IDLE=%1
if [%1]==[] set IDLE=100
set SLICE=%2
if [%2]==[] set SLICE=0
..\utils\sfxcnv.exe in=sfx.txt out=wav\sfx.h wav=wav\ idle=%IDLE% slice=%SLICE%
