// Nin Nin - sprite cell tables (CUSTOM_SPRITE_CELLS). Hand-written.
//
// sprite_cells: frames the engine (and custom.h) pick by number:
//   0-7    Nin Nin (fixed cells 0-7): 0-2 walk right, 3 jump right,
//          4-6 walk left, 7 jump left (the NES order)
//   8-15   enemies, types 1-4, 2 frames each (level cells 0-7)
//   16-21  swimming: 16-18 right, 19-21 left (level cells 8-13)
//   22     explosion (fixed 8), 23 coco (fixed 9), 24 punch (fixed 10; 11 facing left),
//   25     drop (level cell 14)

extern unsigned char *sprite_cells [0];

#asm
	._sprite_cells
		defw _spr_fixed + 0, _spr_fixed + 64, _spr_fixed + 128, _spr_fixed + 192
		defw _spr_fixed + 256, _spr_fixed + 320, _spr_fixed + 384, _spr_fixed + 448
		defw NN_SPR_BASE + 0, NN_SPR_BASE + 64, NN_SPR_BASE + 128, NN_SPR_BASE + 192
		defw NN_SPR_BASE + 256, NN_SPR_BASE + 320, NN_SPR_BASE + 384, NN_SPR_BASE + 448
		defw NN_SPR_BASE + 512, NN_SPR_BASE + 576, NN_SPR_BASE + 640
		defw NN_SPR_BASE + 704, NN_SPR_BASE + 768, NN_SPR_BASE + 832
		defw _spr_fixed + 512, _spr_fixed + 576, _spr_fixed + 640, NN_SPR_BASE + 896
#endasm

#define NN_CELL_SWIM 		16
#define NN_CELL_BOOM 		22
#define NN_CELL_COCO 		23
#define NN_CELL_PUNCH 		24
#define NN_CELL_DROP 		25
