// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Custom flick (ENABLE_CUSTOM_FLICK). The engine calls these right before
// flicking; returning 0 cancels the flick. Results in L (HL).
//
// Yun: in YUN_CYCLIC_LEVEL (yun_level.h) every row of the map wraps around:
// going left from its first screen takes you to its last one and vice versa
// (12x2: left of 0 is 11, left of 12 is 23, right of 11 is 0, right of 23 is
// 12). We move n_pant one map width along and the engine's n_pant -- / ++
// does the rest. Every other level flicks as usual.

#ifdef YUN_CYCLIC_LEVEL
#asm
	// A = n_pant % map_w, HL -> map_w; A = 255 (no column) if not in the cyclic level
	.yfl_col
		ld  hl, _map_w
		ld  a, (_level)
		cp  YUN_CYCLIC_LEVEL
		ld  a, 255
		ret nz
		ld  a, (_n_pant)
	.yfl_col_l
		sub (hl)
		jr  nc, yfl_col_l
		add (hl)
		ret
#endasm
#endif

unsigned char flick_left (void) {
	#asm
		#ifdef YUN_CYCLIC_LEVEL
			// if (level == YUN_CYCLIC_LEVEL && n_pant % map_w == 0) n_pant += map_w;
			call yfl_col
			or  a
			jr  nz, yfl_l_done
			ld  a, (_n_pant)
			add (hl)
			ld  (_n_pant), a
		.yfl_l_done
		#endif
			// return 1;
			ld  hl, 1
	#endasm
}

unsigned char flick_right (void) {
	#asm
		#ifdef YUN_CYCLIC_LEVEL
			// if (level == YUN_CYCLIC_LEVEL && n_pant % map_w == map_w - 1) n_pant -= map_w;
			call yfl_col
			inc a
			cp  (hl)
			jr  nz, yfl_r_done
			ld  a, (_n_pant)
			sub (hl)
			ld  (_n_pant), a
		.yfl_r_done
		#endif
			// return 1;
			ld  hl, 1
	#endasm
}

unsigned char flick_up (void) {
	#asm
			ld  hl, 1
	#endasm
}

unsigned char flick_down (void) {
	#asm
			ld  hl, 1
	#endasm
}
