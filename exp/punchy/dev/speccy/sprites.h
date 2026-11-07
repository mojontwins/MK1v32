// Punchy - ZX sprite frames (mkts_om mode=sprites, masked: graphic, mask)
//
// 16x32 frames (metasize 2,4): 3 columns (the last one empty) of 32 + 8
// lines x 2 bytes = 240 bytes. 8x16 pieces (1,2): 2 columns of 16 + 8 lines
// = 96 bytes. 8x8 cocos (1,1): 2 columns of 8 + 8 = 64 bytes. splib2 needs
// 8 empty lines before each column: the block before (or the 16 bytes of
// pn_blank) gives them.

extern unsigned char spr_tall [];
extern unsigned char spr_piece [];
extern unsigned char spr_coco [];
extern unsigned char spr_boom [];
extern unsigned char spr_empty [];
extern unsigned char spr_piece_empty [];

#asm
	.pn_blank16
		defb 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255
	._spr_tall
		BINARY "speccy/spr_tall.bin"
	._spr_boom
		BINARY "speccy/spr_boom.bin"
	._spr_empty
		BINARY "speccy/spr_empty.bin"
	.pn_blank16b
		defb 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255
	._spr_piece
		BINARY "speccy/spr_piece.bin"
	._spr_piece_empty
		BINARY "speccy/spr_piece_empty.bin"
	.pn_blank16c
		defb 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255
	._spr_coco
		BINARY "speccy/spr_coco.bin"
#endasm

#define PN_TALL_SZ 		240
#define PN_PIECE_SZ 	96
