// Sir Ababol 2 CPC - the sprite frames (gfx/sprites.png). Hand-written.
// cpc/sprites.bin is mkts_om mode=sprites output: 64 bytes per frame (4 bytes
// x 16 lines, nibble mode 0, pen 12 transparent), the original's 22 frames:
//   0-2 walk right, 3 jump right, 4-6 walk left, 7 jump left (as MK1's)
//   8-15 enemies, types 1-4, 2 frames each (the boss uses 14-15)
//   16 the ice block, 17 the dead boss, 18 blank (unused), 19-21 the sword
//   (right, left, up)

extern unsigned char spr_fixed [];
#asm
	._spr_fixed
		BINARY "cpc/sprites.bin"
#endasm

#define SA2_FRAME(n)	(spr_fixed + (n) * 64)
