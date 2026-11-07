// Punchy - CPC sprite frames (mkts_om mode=sprites, mode 0, sprite pens 1-3)
//
// 16x32 frames: 4 bytes x 32 lines = 128 bytes. 8x16 pieces: 2 x 16 = 32.
// 8x8 cocos: 2 x 8 = 16.

extern unsigned char spr_tall [];
extern unsigned char spr_piece [];
extern unsigned char spr_coco [];
extern unsigned char spr_boom [];
extern unsigned char spr_empty [];

#asm
	._spr_tall
		BINARY "cpc/spr_tall.bin"
	._spr_piece
		BINARY "cpc/spr_piece.bin"
	._spr_coco
		BINARY "cpc/spr_coco.bin"
	._spr_boom
		BINARY "cpc/spr_boom.bin"
	._spr_empty
		defs 16, 0
#endasm

#define PN_TALL_SZ 		128
#define PN_PIECE_SZ 	32
