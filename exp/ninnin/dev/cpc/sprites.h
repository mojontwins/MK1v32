// Nin Nin CPC - the fixed sprite cells (gfx/cpc/sprites.png). Hand-written.
// cpc/sprites.bin is mkts_om mode=sprites output: 64 bytes per cell (4 bytes x
// 16 lines, mode 1, pen 0 transparent). Cells 0-7 Nin Nin (drawn for the CPC),
// 8 explosion, 9 coco, 10 punch facing right, 11 punch facing left. The level's cells are at NN_SPR_BASE
// (custom_sprite_cells.h).

extern unsigned char spr_fixed [];
#asm
	._spr_fixed
		BINARY "cpc/sprites.bin"
#endasm

#define NN_FIXED(n)		(spr_fixed + (n) * 64)
#define NN_LCELL(n)		((unsigned char *) (NN_SPR_BASE + (n) * 64))
