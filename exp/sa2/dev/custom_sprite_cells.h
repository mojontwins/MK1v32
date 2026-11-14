// Sir Ababol 2 - sprite cell table (CUSTOM_SPRITE_CELLS). Hand-written.
// sprite_cells [n] is frame n of cpc/sprites.h.

extern unsigned char *sprite_cells [0];

#asm
	._sprite_cells
		defw _spr_fixed + 0, _spr_fixed + 64, _spr_fixed + 128, _spr_fixed + 192
		defw _spr_fixed + 256, _spr_fixed + 320, _spr_fixed + 384, _spr_fixed + 448
		defw _spr_fixed + 512, _spr_fixed + 576, _spr_fixed + 640, _spr_fixed + 704
		defw _spr_fixed + 768, _spr_fixed + 832, _spr_fixed + 896, _spr_fixed + 960
		defw _spr_fixed + 1024, _spr_fixed + 1088, _spr_fixed + 1152, _spr_fixed + 1216
		defw _spr_fixed + 1280, _spr_fixed + 1344
#endasm

#define SA2_CELL_ICE 		16
#define SA2_CELL_BOSS_DEAD 	17
#define SA2_CELL_SWORD 		19
