# orig/zx

`ingame.scr`: the screen of the original ZX version (2006) at level 1, taken
from `../ncjmc-zx.z80` running in the headless emulator (zxheadless with a 48K
ROM: `-z80 ncjmc-zx.z80`, S at frame 1000, SPACE at frame 3000, memory dump at
frame 3500). `tools/nanako_conv.py` takes the ZX placeholders from it.

`title.scr` (frame 1500, no keys) and `levelbox.scr` (S at frame 1000, frame
1900) are taken the same way. `tools/nanako_zxgfx.py` makes
`gfx/zx_original/screen_*.png` from them.
