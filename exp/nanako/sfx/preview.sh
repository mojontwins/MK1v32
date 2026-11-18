#!/bin/sh
# Writes wav/<name>.wav for each effect in sfx.txt, with the player's timing.
# preview.bat does the same.
#
#   ./preview.sh [idle] [slice]
#
# idle: % of the frame the game leaves idle (100 = the sound as it is; use what
# the game leaves idle, measured in an emulator). slice: SFX_MIN_SLICE in config.h.

T=${MK1_TOOLS:-../utils}
cd "$(dirname "$0")"
mkdir -p wav
$T/sfxcnv in=sfx.txt out=wav/sfx.h wav=wav/ idle=${1:-100} slice=${2:-0}
