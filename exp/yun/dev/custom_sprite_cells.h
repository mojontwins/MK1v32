// Yun - sprite cell tables (CUSTOM_SPRITE_CELLS). Hand-written.
//
// Fixed cells, in the binary (sprites.png): 0-7 Yun, 8-9 bubble, 10-11
// platform, 12-13 fanty, 14-15 trapped fanty, 16-19 monococo (bush, coming
// out, out, trapped), 20 coco, 21 explosion (CPC spr_boom).
// Level cells, at SPR_VAR_BASE (spritesN.png): 0-5 linear enemies
// (types 1-3, 2 frames each), 6-11 the same trapped (upside down).
//
// sprite_cells: player frames 0-7, then enemy types 1-4 (2 frames each).
// ud_cells: the trapped frames of types 1-6 ((type - 1) * 2 + frame); type 4,
// the platform, is never trapped. With YUN_EXTRA_ENEMS (loads 2 and 3) also
// types 7 (pezon) and 8 (monococo in the wall), and fixed cells 22-31: 22-23
// that monococo hidden / peeking, 24-25 pezon (and trapped), 26-27 gyrosaw,
// 28-31 cannon facing left, up, right, down.
// ZX: a cell pair every 144 bytes (sprcnv2 nomask), CPC: 64 bytes per cell.

extern unsigned char *sprite_cells [0];
extern unsigned char *ud_cells [0];

#ifdef CPC
	#asm
		._sprite_cells
			defw _spr_fixed + 0, _spr_fixed + 64, _spr_fixed + 128, _spr_fixed + 192
			defw _spr_fixed + 256, _spr_fixed + 320, _spr_fixed + 384, _spr_fixed + 448
			defw SPR_VAR_BASE + 0, SPR_VAR_BASE + 64, SPR_VAR_BASE + 128, SPR_VAR_BASE + 192
			defw SPR_VAR_BASE + 256, SPR_VAR_BASE + 320, _spr_fixed + 640, _spr_fixed + 704
		._ud_cells
			defw SPR_VAR_BASE + 384, SPR_VAR_BASE + 448, SPR_VAR_BASE + 512, SPR_VAR_BASE + 576
			defw SPR_VAR_BASE + 640, SPR_VAR_BASE + 704, _spr_fixed + 640, _spr_fixed + 704
			defw _spr_fixed + 896, _spr_fixed + 960, _spr_fixed + 1216, _spr_fixed + 1216
		#ifdef YUN_EXTRA_ENEMS
			defw _spr_fixed + 1600, _spr_fixed + 1600, _spr_fixed + 1216, _spr_fixed + 1216
		#endif
	#endasm
#else
	#asm
		._sprite_cells
		#ifdef PLAYER_TALL
			// 16x24 Yun (speccy/player.h from gfx/speccy/player.png; sprites.png cells 0-7 aren't converted)
			defw _player_1_a, _player_2_a, _player_3_a, _player_4_a
			defw _player_5_a, _player_6_a, _player_7_a, _player_8_a
		#else
			defw _sprite_1_a, _sprite_2_a, _sprite_3_a, _sprite_4_a
			defw _sprite_5_a, _sprite_6_a, _sprite_7_a, _sprite_8_a
		#endif
			defw SPR_VAR_BASE + 0, SPR_VAR_BASE + 1, SPR_VAR_BASE + 144, SPR_VAR_BASE + 145
			defw SPR_VAR_BASE + 288, SPR_VAR_BASE + 289, _sprite_11_a, _sprite_12_a
		._ud_cells
			defw SPR_VAR_BASE + 432, SPR_VAR_BASE + 433, SPR_VAR_BASE + 576, SPR_VAR_BASE + 577
			defw SPR_VAR_BASE + 720, SPR_VAR_BASE + 721, _sprite_11_a, _sprite_12_a
			defw _sprite_15_a, _sprite_16_a, _sprite_20_a, _sprite_20_a
		#ifdef YUN_EXTRA_ENEMS
			defw _sprite_26_a, _sprite_26_a, _sprite_20_a, _sprite_20_a
		#endif
	#endasm
#endif
