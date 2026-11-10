// Nin Nin - EL CASTLER's puzzles (load 2, NN_CASTLE). Hand-written, from the
// NES game's my/hooks.h (level 11). The coordinates are the NES ones moved to
// the 15x10 screens (tools/nn_assets.py's reduction: the rows and columns the
// converter deleted in each screen); where a NES tile fell in a deleted line,
// the next one is used.
//
//  * Screens 1, 3: entering them is not a safe place to come back to.
//  * Screens 5, 7, 23 and 15: spring traps (behaviour 48: the air over a floor
//    turns into spikes when stepped on); 5 and 7 also have a hidden way
//    through (behaviour 0) at the right.
//  * Screen 13: the moving platform (enemy 0) is off until FIRE on the face
//    at the bottom left.
//  * Screen 17: walking into the column at x 5 (rows 1-4) breaks the wall
//    in the middle (row 5, x 6-9), once.
//  * Screen 18: FIRE on the face at the top (8, 3) builds a column of
//    breakable blocks at x 9, rows 4-6.
//  * Screen 20: when its rays are off, two bricks (44) appear instead of the
//    fences opening.

#define NN_CASTLE_LEVEL 	9
unsigned char nn_cf0, nn_fzone;

// behaviour changes: x, y, behaviour; 0xff ends
unsigned char nn_alm5 [] = { 4, 2, 48, 8, 2, 48, 10, 2, 48, 13, 7, 0, 13, 8, 0, 0xff };
unsigned char nn_alm7 [] = { 4, 3, 48, 8, 3, 48, 10, 3, 48, 13, 7, 0, 13, 8, 0, 0xff };
unsigned char nn_alm15 [] = { 2, 3, 48, 0xff };
unsigned char nn_alm23 [] = { 6, 4, 48, 4, 4, 48, 0xff };

void nn_attr_list (void) {
	// nn_p: x, y, behaviour...
	// while (*nn_p != 0xff) {
	//	map_attr [nn_p [0] + (nn_p [1] << 4) - nn_p [1]] = nn_p [2];
	//	nn_p += 3; }
	#asm
			ld  hl, (_nn_p)
		.nn_al_loop
			ld  a, (hl)
			cp  0xff
			jr  z, nn_al_end
			ld  c, a
			inc hl
			ld  a, (hl)
			ld  b, a
			add a, a
			add a, a
			add a, a
			add a, a
			sub b
			add a, c
			inc hl
			ld  e, (hl)
			inc hl
			push hl
			ld  hl, _map_attr
			call nn_add_a
			ld  (hl), e
			pop hl
			jr  nn_al_loop
		.nn_al_end
			ld  (_nn_p), hl
	#endasm
}

#asm
	// Z if Nin Nin's centre is on tile (D, E): ((gpx + 8) >> 4) == D && ((gpy + 8) >> 4) == E
	.nn_player_at
		ld  c, 8
		ld  a, (_gpx)
		call nn_cell
		cp  d
		ret nz
		ld  a, (_gpy)
		call nn_cell
		cp  e
		ret
#endasm

void nn_castle_screen (void) {
	// nn_cf0 = 0; nn_fzone = 0;
	// if (nn_l != NN_CASTLE_LEVEL) return;
	// nn_p = 0;
	// switch (n_pant) {
	//	case 5: nn_p = nn_alm5; break;
	//	case 7: nn_p = nn_alm7; break;
	//	case 15: nn_p = nn_alm15; break;
	//	case 23: nn_p = nn_alm23; break;
	//	case 13: // the platform is off
	//		malotes [enoffs].t = 0; en_an_next_frame [0] = spr_empty; break;
	//	case 17: nn_fzone = 1; break;
	// }
	// if (nn_p) nn_attr_list ();
	#asm
			xor a
			ld  (_nn_cf0), a
			ld  (_nn_fzone), a
			ld  a, (_nn_l)
			cp  NN_CASTLE_LEVEL
			ret nz
			ld  hl, 0
			ld  (_nn_p), hl
			ld  a, (_n_pant)
			ld  hl, _nn_alm5
			cp  5
			jr  z, nn_cs_list
			ld  hl, _nn_alm7
			cp  7
			jr  z, nn_cs_list
			ld  hl, _nn_alm15
			cp  15
			jr  z, nn_cs_list
			ld  hl, _nn_alm23
			cp  23
			jr  z, nn_cs_list
			cp  13
			jr  z, nn_cs_13
			cp  17
			ret nz
			ld  a, 1
			ld  (_nn_fzone), a
			ret
		.nn_cs_13
			call nn_cs_plat
			ld  (hl), 0
			ld  hl, _spr_empty
			ld  (_en_an_next_frame), hl
			ret
		.nn_cs_list
			ld  (_nn_p), hl
			jp  _nn_attr_list

			// HL -> malotes [enoffs].t
		.nn_cs_plat
			ld  hl, (_enoffs)
			ld  d, h
			ld  e, l
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, de
			ld  de, _malotes + 8
			add hl, de
			ret
	#endasm
}

unsigned char nn_fire_zoned (void) {
	// screen 17: x 80-95, y 16-79 fires by itself
	// if (nn_fzone && gpx + 8 >= 80 && gpx + 8 <= 95 && gpy + 8 >= 16 && gpy + 8 <= 79) return 1;
	// return 0;
	#asm
			ld  hl, 0
			ld  a, (_nn_fzone)
			or  a
			ret z
			ld  a, (_gpx)
			add a, 8
			ret c
			cp  80
			ret c
			cp  96
			ret nc
			ld  a, (_gpy)
			add a, 8
			ret c
			cp  16
			ret c
			cp  80
			ret nc
			inc l
	#endasm
}

unsigned char nn_castle_fire (void) {
	// if (nn_l != NN_CASTLE_LEVEL) return 0;
	// if (nn_fzone && nn_fire_zoned ()) {
	//	peta_el_beeper (5);
	//	_y = 5; for (_x = 6; _x < 10; _x ++) { _t = tileset_mappings [0]; nn_set_tile (); }
	//	nn_fzone = 0;
	//	return 1; }
	// if (nn_cf0) return 0;
	// if (n_pant == 18 && NN_PLAYER_AT (8, 2)) {
	//	peta_el_beeper (5);
	//	_x = 9; for (_y = 4; _y < 7; _y ++) { _t = 29; nn_set_tile (); }
	//	nn_cf0 = 1;
	//	return 1; }
	// if (n_pant == 13 && NN_PLAYER_AT (3, 8)) {
	//	// the platform comes back
	//	peta_el_beeper (8);
	//	malotes [enoffs].t = 4;
	//	enit = 0; enems_en_an_calc (3);
	//	nn_cf0 = 1;
	//	return 0; 		// (the face's own puzzle goes on, as on the NES)
	// }
	// return 0;
	#asm
			ld  hl, 0
			ld  a, (_nn_l)
			cp  NN_CASTLE_LEVEL
			ret nz
			ld  a, (_nn_fzone)
			or  a
			jr  z, nn_cf_1
			call _nn_fire_zoned
			ld  a, l
			or  a
			jr  z, nn_cf_1
			ld  l, 5
			call _peta_el_beeper
			ld  a, 5
			ld  (__y), a
			ld  a, 6
		.nn_cf_wall
			ld  (__x), a
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
			ld  (__t), a
			call _nn_set_tile
			ld  a, (__x)
			inc a
			cp  10
			jr  c, nn_cf_wall
			ld  (__x), a
			xor a
			ld  (_nn_fzone), a
			ld  hl, 1
			ret
		.nn_cf_1
			ld  hl, 0
			ld  a, (_nn_cf0)
			or  a
			ret nz
			ld  a, (_n_pant)
			cp  18
			jr  nz, nn_cf_2
			ld  de, 8 * 256 + 2
			call nn_player_at
			ld  hl, 0
			ret nz
			ld  l, 5
			call _peta_el_beeper
			ld  a, 9
			ld  (__x), a
			ld  a, 4
		.nn_cf_col
			ld  (__y), a
			ld  a, 29
			ld  (__t), a
			call _nn_set_tile
			ld  a, (__y)
			inc a
			cp  7
			jr  c, nn_cf_col
			ld  (__y), a
			ld  a, 1
			ld  (_nn_cf0), a
			ld  hl, 1
			ret
		.nn_cf_2
			cp  13
			ret nz
			ld  de, 3 * 256 + 8
			call nn_player_at
			ld  hl, 0
			ret nz
			ld  l, 8
			call _peta_el_beeper
			call nn_cs_plat
			ld  (hl), 4
			xor a
			ld  (_enit), a
			ld  hl, 3
			call _enems_en_an_calc
			ld  a, 1
			ld  (_nn_cf0), a
			ld  hl, 0
	#endasm
}

unsigned char nn_castle_rays (void) {
	// all the rays of the screen are off: 1 if handled here
	// if (nn_l == NN_CASTLE_LEVEL && n_pant == 20) {
	//	_x = 8; _y = 3; _t = 44; nn_set_tile ();
	//	_x = 5; _y = 1; _t = 44; nn_set_tile ();
	//	return 1; }
	// return 0;
	#asm
			ld  hl, 0
			ld  a, (_nn_l)
			cp  NN_CASTLE_LEVEL
			ret nz
			ld  a, (_n_pant)
			cp  20
			ret nz
			ld  a, 44
			ld  (__t), a
			ld  a, 8
			ld  (__x), a
			ld  a, 3
			ld  (__y), a
			call _nn_set_tile
			ld  a, 5
			ld  (__x), a
			ld  a, 1
			ld  (__y), a
			ld  a, 44
			ld  (__t), a
			call _nn_set_tile
			ld  hl, 1
	#endasm
}

unsigned char nn_screen_is_safe (void) {
	// return !(nn_l == NN_CASTLE_LEVEL && (n_pant == 1 || n_pant == 3));
	#asm
			ld  hl, 1
			ld  a, (_nn_l)
			cp  NN_CASTLE_LEVEL
			ret nz
			ld  a, (_n_pant)
			cp  1
			jr  z, nn_si_no
			cp  3
			ret nz
		.nn_si_no
			dec l
	#endasm
}
