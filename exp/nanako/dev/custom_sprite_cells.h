// Nanako: 19 cells.
// 0-7 Nanako (MK1 sideview: right 0-3, left 4-7), 8-11 karakasa's head,
// 12-15 karakasa's body (left 0, 1, right 0, 1), 16-18 puff.

#ifdef CPC
	extern unsigned char *sprite_cells [0];
	#asm
		._sprite_cells
			defw SPRITE_00, SPRITE_01, SPRITE_02, SPRITE_03, SPRITE_04, SPRITE_05, SPRITE_06, SPRITE_07
			defw SPRITE_08, SPRITE_09, SPRITE_0A, SPRITE_0B, SPRITE_0C, SPRITE_0D, SPRITE_0E, SPRITE_0F
			defw SPRITE_10, SPRITE_11, SPRITE_12
	#endasm
#else
	unsigned char *sprite_cells [] = {
		sprite_1_a, sprite_2_a, sprite_3_a, sprite_4_a,
		sprite_5_a, sprite_6_a, sprite_7_a, sprite_8_a,
		sprite_9_a, sprite_10_a, sprite_11_a, sprite_12_a,
		sprite_13_a, sprite_14_a, sprite_15_a, sprite_16_a,
		sprite_17_a, sprite_18_a, sprite_19_a
	};
#endif
