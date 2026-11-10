// Nin Nin - the enemies (ENABLE_CUSTOM_ENEMS). Hand-written, from the NES
// game (engine/enengine.h and enemmods/).
//
// The type's low nibble is the movement, the high nibble what it shoots:
//   1-3  linear (the engine's), 4 moving platform (the engine's)
//   6    fanty: flies after Nin Nin (3 hits)
//   7    pursuer: walks after Nin Nin along its row
//   9    drop: falls from (x1, y1) to (x2, y2), again and again; can't die
//   0x1X shoots left / right when Nin Nin is in its row (and it faces him)
//   0x2X shoots up / down when Nin Nin is in its column
//   0x3X-0x6X shoots up, right, down, left every so often
//   0x7X turns while it shoots: the 8 directions in turn
// The engine moves types 1-4 and draws them all (frames from their base
// type: 6 and 7 use type 3's, the NES FANTIES_CELL / PURSUERS_CELL); here
// the rest moves, shoots, and dies when it touches Nin Nin, as on the NES.
//
// Speeds: the .ene speeds are NES pixels per frame at 60 Hz; custom_level_setup
// doubles mx, my for 25 Hz.

#define NN_COCO_FREQ 		62		// NES 150 frames
unsigned char nn_en_ct [MAX_ENEMS], nn_en_cd [MAX_ENEMS];
unsigned char nn_base, nn_touched;
signed int nn_fvx [MAX_ENEMS], nn_fvy [MAX_ENEMS];

#asm
	// nn_fvx / nn_fvy [enit] (HL ->) towards Nin Nin if carry, else away:
	// if (v < 192) v += 16; / if (v > -192) v -= 16;
	.nn_fan_acc
		ld  e, (hl)
		inc hl
		ld  d, (hl)
		jr  nc, nn_fa_dec
		push hl
		ld  hl, -192
		add hl, de
		bit 7, h
		pop hl
		ret z
		ld  a, e
		add a, 16
		ld  e, a
		jr  nc, nn_fa_st
		inc d
		jr  nn_fa_st
	.nn_fa_dec
		push hl
		ld  hl, 191
		add hl, de
		bit 7, h
		pop hl
		ret nz
		ld  a, e
		sub 16
		ld  e, a
		jr  nc, nn_fa_st
		dec d
	.nn_fa_st
		ld  (hl), d
		dec hl
		ld  (hl), e
		ret

	// A = low byte of (HL) >> 6 (signed int)
	.nn_ld_sr6
		ld  e, (hl)
		inc hl
		ld  d, (hl)
		ld  b, 6
	.nn_ls6
		sra d
		rr  e
		djnz nn_ls6
		ld  a, e
		ret

	// en_an_next_frame [enit] = sprite_cells [NN_CELL_DROP]
	.nn_drop_frame
		ld  de, (_sprite_cells + 2 * NN_CELL_DROP)
		ld  hl, _en_an_next_frame
		ld  a, (_enit)
		add a, a
		call nn_add_a
		ld  (hl), e
		inc hl
		ld  (hl), d
		ret
#endasm

void extra_enems_init (void) {
	// nn_en_ct [enit] = NN_COCO_FREQ;
	// nn_en_cd [enit] = 0;
	// nn_fvx [enit] = nn_fvy [enit] = 0;
	// if (_en_t <= 4) return;
	// nn_base = _en_t & 15;
	// if (nn_base <= 4) enems_en_an_calc (nn_base - 1);
	// else if (nn_base == 6 || nn_base == 7) enems_en_an_calc (2);
	// else if (nn_base == 9) en_an_next_frame [enit] = sprite_cells [NN_CELL_DROP];
	#asm
			ld  hl, _nn_en_ct
			ld  a, (_enit)
			call nn_add_a
			ld  (hl), NN_COCO_FREQ
			ld  hl, _nn_en_cd
			ld  a, (_enit)
			call nn_add_a
			ld  (hl), 0
			ld  a, (_enit)
			add a, a
			ld  c, a
			ld  hl, _nn_fvx
			call nn_add_a
			xor a
			ld  (hl), a
			inc hl
			ld  (hl), a
			ld  hl, _nn_fvy
			ld  a, c
			call nn_add_a
			xor a
			ld  (hl), a
			inc hl
			ld  (hl), a
			ld  a, (__en_t)
			cp  5
			ret c
			and 15
			ld  (_nn_base), a
			cp  5
			jr  nc, nn_ei_1
			dec a
			ld  l, a
			ld  h, 0
			jp  _enems_en_an_calc
		.nn_ei_1
			cp  6
			jr  z, nn_ei_2
			cp  7
			jr  nz, nn_ei_3
		.nn_ei_2
			ld  hl, 2
			jp  _enems_en_an_calc
		.nn_ei_3
			cp  9
			ret nz
			jp  nn_drop_frame
	#endasm
}

void nn_shoot (void) {
	// if (nn_en_ct [enit]) { nn_en_ct [enit] --; return; }
	// _x = _en_x + 4; _y = _en_y + 4; _t = 0xff;
	// switch (_en_t & 0xf0) {
	//	case 0x10:
	//		if (gpy + 8 >= _en_y && gpy <= _en_y + 8) {
	//			if (_en_mx == 0 || (_en_mx > 0 && gpx > _en_x) || (_en_mx < 0 && gpx < _en_x))
	//				_t = (gpx < _en_x) ? 6 : 2; }
	//		break;
	//	case 0x20:
	//		if (gpx + 8 >= _en_x && gpx <= _en_x + 8) {
	//			if (_en_my == 0 || (_en_my > 0 && gpy > _en_y) || (_en_my < 0 && gpy < _en_y))
	//				_t = (gpy < _en_y) ? 0 : 4; }
	//		break;
	//	case 0x30: case 0x40: case 0x50: case 0x60:
	//		_t = ((_en_t >> 4) - 3) << 1;
	//		break;
	//	case 0x70:
	//		nn_en_cd [enit] = (nn_en_cd [enit] + 1) & 7;
	//		_t = nn_en_cd [enit];
	//		break;
	// }
	// if (_t != 0xff) { cocos_add (); nn_en_ct [enit] = NN_COCO_FREQ; }
	#asm
			ld  hl, _nn_en_ct
			ld  a, (_enit)
			call nn_add_a
			ld  a, (hl)
			or  a
			jr  z, nn_sh_0
			dec (hl)
			ret
		.nn_sh_0
			ld  a, (__en_x)
			add a, 4
			ld  (__x), a
			ld  a, (__en_y)
			add a, 4
			ld  (__y), a
			ld  a, 0xff
			ld  (__t), a
			ld  a, (__en_t)
			and 0xf0
			cp  0x10
			jr  z, nn_sh_h
			cp  0x20
			jr  z, nn_sh_v
			cp  0x70
			jr  z, nn_sh_rot
			cp  0x30
			jr  c, nn_sh_done
			cp  0x70
			jr  nc, nn_sh_done
			rrca
			rrca
			rrca
			rrca
			sub 3
			add a, a
			jr  nn_sh_t

		.nn_sh_h
			// in its row: gpy + 8 >= _en_y && _en_y + 8 >= gpy
			ld  a, (_gpy)
			ld  b, a
			ld  a, (__en_y)
			ld  c, a
			call nn_sh_near
			jr  c, nn_sh_done
			ld  a, (_gpx)
			ld  b, a 					// B = gpx
			ld  a, (__en_x)
			ld  c, a 					// C = _en_x
			ld  a, (__en_mx)
			ld  de, 6 * 256 + 2 		// D: gpx < _en_x, E: else
			jr  nn_sh_dir
		.nn_sh_v
			ld  a, (_gpx)
			ld  b, a
			ld  a, (__en_x)
			ld  c, a
			call nn_sh_near
			jr  c, nn_sh_done
			ld  a, (_gpy)
			ld  b, a
			ld  a, (__en_y)
			ld  c, a
			ld  a, (__en_my)
			ld  de, 0 * 256 + 4
		.nn_sh_dir
			// facing it: m == 0 || (m > 0 && B > C) || (m < 0 && B < C)
			or  a
			jr  z, nn_sh_face
			rlca
			jr  c, nn_sh_mneg
			ld  a, c
			cp  b
			jr  nc, nn_sh_done 			// not C < B
			jr  nn_sh_face
		.nn_sh_mneg
			ld  a, b
			cp  c
			jr  nc, nn_sh_done 			// not B < C
		.nn_sh_face
			ld  a, b
			cp  c
			ld  a, d
			jr  c, nn_sh_t
			ld  a, e
			jr  nn_sh_t

		.nn_sh_rot
			ld  hl, _nn_en_cd
			ld  a, (_enit)
			call nn_add_a
			ld  a, (hl)
			inc a
			and 7
			ld  (hl), a
		.nn_sh_t
			ld  (__t), a
		.nn_sh_done
			ld  a, (__t)
			cp  0xff
			ret z
			call _cocos_add
			ld  hl, _nn_en_ct
			ld  a, (_enit)
			call nn_add_a
			ld  (hl), NN_COCO_FREQ
			ret

			// NC if B + 8 >= C && C + 8 >= B (as 9 bit sums), C otherwise
		.nn_sh_near
			ld  a, b
			add a, 8
			jr  c, nn_sn_1
			cp  c
			ret c
		.nn_sn_1
			ld  a, c
			add a, 8
			ccf
			ret nc 						// (carry: >= 256, true)
			cp  b
			ret
	#endasm
}

void extra_enems_move (void) {
	// nn_base = _en_t & 15;
	// if (_en_t > 4) {
	//	if (nn_base <= 4) en_lineal_do ();
	//	else if (nn_base == 7) {
	//		// pursuer: along its row, at its speed
	//		rda = _en_mx < 0 ? -_en_mx : _en_mx; if (rda == 0) rda = 2;
	//		if (_en_x > gpx + rda) _en_x -= rda;
	//		else if (_en_x + rda < gpx) _en_x += rda;
	//	} else if (nn_base == 6) {
	//		// fanty: accelerates towards Nin Nin (1/64 px, 8 bit steps)
	//		if (_en_x < gpx) { if (nn_fvx [enit] < 192) nn_fvx [enit] += 16; }
	//		else { if (nn_fvx [enit] > -192) nn_fvx [enit] -= 16; }
	//		if (_en_y < gpy) { if (nn_fvy [enit] < 192) nn_fvy [enit] += 16; }
	//		else { if (nn_fvy [enit] > -192) nn_fvy [enit] -= 16; }
	//		_en_x += nn_fvx [enit] >> 6; _en_y += nn_fvy [enit] >> 6;
	//	} else if (nn_base == 9) {
	//		// drop
	//		_en_x += _en_mx; _en_y += _en_my;
	//		if (_en_y >= _en_y2) { _en_x = _en_x1; _en_y = _en_y1; }
	//		en_an_next_frame [enit] = sprite_cells [NN_CELL_DROP];
	//	}
	// }
	// if (_en_t & 0x70 && _en_t < 0x80) nn_shoot ();
	#asm
			ld  a, (__en_t)
			and 15
			ld  (_nn_base), a
			ld  c, a
			ld  a, (__en_t)
			cp  5
			jp  c, nn_em_shoot
			ld  a, c
			cp  5
			jr  nc, nn_em_1
			call en_lineal_do
			jp  nn_em_shoot
		.nn_em_1
			cp  7
			jr  nz, nn_em_2
			// pursuer
			ld  a, (__en_mx)
			bit 7, a
			jr  z, nn_em_p1
			neg
		.nn_em_p1
			or  a
			jr  nz, nn_em_p2
			ld  a, 2
		.nn_em_p2
			ld  (_rda), a
			ld  b, a
			ld  a, (_gpx)
			add a, b
			jr  c, nn_em_p3 			// gpx + rda > 255
			ld  c, a
			ld  a, (__en_x)
			cp  c
			jr  z, nn_em_p3
			jr  c, nn_em_p3
			sub b
			ld  (__en_x), a
			jp  nn_em_shoot
		.nn_em_p3
			ld  a, (__en_x)
			add a, b
			jp  c, nn_em_shoot
			ld  c, a
			ld  a, (_gpx)
			cp  c
			jp  c, nn_em_shoot
			jp  z, nn_em_shoot
			ld  a, c
			ld  (__en_x), a
			jp  nn_em_shoot
		.nn_em_2
			cp  6
			jr  nz, nn_em_3
			// fanty
			ld  a, (_enit)
			add a, a
			ld  hl, _nn_fvx
			call nn_add_a
			ld  a, (_gpx)
			ld  b, a
			ld  a, (__en_x)
			cp  b
			call nn_fan_acc
			ld  a, (_enit)
			add a, a
			ld  hl, _nn_fvy
			call nn_add_a
			ld  a, (_gpy)
			ld  b, a
			ld  a, (__en_y)
			cp  b
			call nn_fan_acc
			ld  a, (_enit)
			add a, a
			ld  hl, _nn_fvx
			call nn_add_a
			call nn_ld_sr6
			ld  b, a
			ld  a, (__en_x)
			add a, b
			ld  (__en_x), a
			ld  a, (_enit)
			add a, a
			ld  hl, _nn_fvy
			call nn_add_a
			call nn_ld_sr6
			ld  b, a
			ld  a, (__en_y)
			add a, b
			ld  (__en_y), a
			jr  nn_em_shoot
		.nn_em_3
			cp  9
			jr  nz, nn_em_shoot
			// drop
			ld  a, (__en_mx)
			ld  b, a
			ld  a, (__en_x)
			add a, b
			ld  (__en_x), a
			ld  a, (__en_my)
			ld  b, a
			ld  a, (__en_y)
			add a, b
			ld  (__en_y), a
			ld  b, a
			ld  a, (__en_y2)
			ld  c, a
			ld  a, b
			cp  c
			jr  c, nn_em_d1
			ld  a, (__en_x1)
			ld  (__en_x), a
			ld  a, (__en_y1)
			ld  (__en_y), a
		.nn_em_d1
			call nn_drop_frame
		.nn_em_shoot
			ld  a, (__en_t)
			cp  0x80
			ret nc
			and 0x70
			ret z
			jp  _nn_shoot
	#endasm
}

void extra_enems_checks (void) {
	// touched Nin Nin (this_enemy_kills) or punched: dies
	// if (_en_t == 4 || _en_t >= 128) { nn_touched = 0; return; }
	// if (nn_touched == 0 && hit_on && hit_hy) {
	//	if (hit_hx + 1 >= _en_x && hit_hx <= _en_x + 15 && hit_hy + 1 >= _en_y && hit_hy <= _en_y + 15) nn_touched = 1; }
	// if (nn_touched) { nn_touched = 0; if ((_en_t & 15) != 9) enems_kill (1); }
	#asm
			ld  a, (__en_t)
			cp  4
			jr  z, nn_ec_clr
			cp  128
			jr  c, nn_ec_1
		.nn_ec_clr
			xor a
			ld  (_nn_touched), a
			ret
		.nn_ec_1
			ld  a, (_nn_touched)
			or  a
			jr  nz, nn_ec_kill
			ld  a, (_hit_on)
			or  a
			ret z
			ld  a, (_hit_hy)
			or  a
			ret z
			ld  a, (_hit_hx)
			ld  b, a
			ld  a, (__en_x)
			ld  c, a
			call nn_ec_in
			ret c
			ld  a, (_hit_hy)
			ld  b, a
			ld  a, (__en_y)
			ld  c, a
			call nn_ec_in
			ret c
		.nn_ec_kill
			xor a
			ld  (_nn_touched), a
			ld  a, (__en_t)
			and 15
			cp  9
			ret z
			ld  hl, 1
			push hl
			call _enems_kill
			pop hl
			ret
			// NC if B + 1 >= C && C + 15 >= B (9 bit sums), C otherwise
		.nn_ec_in
			ld  a, b
			inc a
			jr  z, nn_ei_in1
			cp  c
			ret c
		.nn_ei_in1
			ld  a, c
			add a, 15
			ccf
			ret nc
			cp  b
			ret
	#endasm
}

void extra_enems_killed (void) {
}

unsigned char this_enemy_kills (void) {
	// nn_touched = 1; return 1;
	#asm
			ld  a, 1
			ld  (_nn_touched), a
			ld  hl, 1
	#endasm
}
