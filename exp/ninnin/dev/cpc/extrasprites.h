// Nin Nin CPC - extra frames. Hand-written.

#define spr_boom	NN_FIXED (8)
#define spr_coco	NN_FIXED (9)
#define spr_punch	NN_FIXED (10)

extern unsigned char spr_empty [];
#asm
	._spr_empty
		defs 64, 0
#endasm
