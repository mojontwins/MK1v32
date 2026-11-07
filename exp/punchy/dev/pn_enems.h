// Punchy - enemies: punchies, shooties, plain patrollers, warpers; cocos.
// From MK1_NES (enem_punchy.h, enem_shooty.h, enems_recoiling.h).
//
// MK1 types (enems/level0.ene, converted from the tester):
//   1 punchy (NES 0x41): linear; punches when the player is in front, close
//   2 shooty (NES 0x81): linear; shoots cocos at the player in line
//   3 linear (NES 0x01)
//   32 + n warper (NES 0xff) to screen n: FIRE while touching it; the player
//   appears at its x2, y2
// Every one takes ENEMS_LIFE_GAUGE (3) hits, is pushed back and stunned for
// a moment on each, and comes back when the screen is entered again.

#define PN_EN_ATTACK 	PN_T16	// NES 16: punching / shooting
#define PN_EN_TOUCHED 	PN_T8	// NES ENEMS_TOUCHED_FRAMES 8
#define PN_EN_RECOIL 	PN_SPD2	// px per frame (NES 2 at 60 Hz)
#define PN_COCO_V 		PN_SPD2	// px per frame (NES 128 / 64 at 60 Hz)
#define TYPE_WARPER 	32

unsigned char en_face [MAX_ENEMS], en_ct [MAX_ENEMS], en_hitct [MAX_ENEMS];
unsigned char en_pc [MAX_ENEMS], en_ox [MAX_ENEMS];
signed char en_recd [MAX_ENEMS];
signed int pn_ex, pn_ey;

#asm
	// HL = HL + enit (an enemy's entry in a byte array)
	.pn_ei
		ld  a, (_enit)
		jp  pn_add_a

	// NC if HL >= DE (signed, |HL - DE| < 32768), C otherwise
	.pn_ge
		or  a
		sbc hl, de
		ld  a, h
		rlca 						// carry = sign of HL - DE
		ret

	// NC if the player is in line (gpy + 23 >= _en_y && gpy <= _en_y + 23)
	// and in front of enemy enit ((en_face && _en_x > gpx) ||
	// (!en_face && _en_x < gpx)), C otherwise
	.pn_em_sees
		ld  a, (__en_y)
		ld  b, a
		ld  a, (_gpy)
		add a, 23
		jr  c, pn_ems_1
		cp  b
		ret c
	.pn_ems_1
		ld  a, (_gpy)
		ld  c, a
		ld  a, b
		add a, 23
		jr  c, pn_ems_2
		cp  c
		ret c
	.pn_ems_2
		ld  hl, _en_face
		call pn_ei
		ld  a, (_gpx)
		ld  c, a
		ld  a, (hl)
		or  a
		ld  a, (__en_x)
		jr  z, pn_ems_right
		cp  c
		ret c 						// _en_x < gpx
		jr  z, pn_ems_no 			// _en_x == gpx
		or  a
		ret
	.pn_ems_right
		cp  c
		ccf
		ret 						// C if _en_x >= gpx
	.pn_ems_no
		scf
		ret
#endasm

// ----------------------------------------------------------------------------
// Cocos: 8x8, straight, killed by walls and the edges of the screen

void pn_coco_shoot (void) {
	// for (pn_j = 0; pn_j < PN_COCOS; pn_j ++) if (co_on [pn_j] == 0) {
	//	co_on [pn_j] = 1;
	//	co_x [pn_j] = en_face [enit] ? _en_x - 8 : _en_x + 16;
	//	co_y [pn_j] = _en_y - 2;
	//	co_mx [pn_j] = en_face [enit] ? -PN_COCO_V : PN_COCO_V;
	//	return;
	// }
	#asm
			ld  hl, _co_on
			ld  c, 0
		.pn_cs_find
			ld  a, (hl)
			or  a
			jr  z, pn_cs_found
			inc hl
			inc c
			ld  a, c
			cp  PN_COCOS
			jr  c, pn_cs_find
			ld  (_pn_j), a
			ret
		.pn_cs_found
			ld  (hl), 1
			ld  a, c
			ld  (_pn_j), a
			ld  hl, _en_face
			call pn_ei
			ld  a, (hl)
			or  a
			ld  a, (__en_x)
			ld  b, PN_COCO_V
			jr  z, pn_cs_right
			sub 8
			ld  b, 256 - PN_COCO_V
			jr  pn_cs_x
		.pn_cs_right
			add a, 16
		.pn_cs_x
			ld  hl, _co_x
			ld  e, a
			ld  a, c
			call pn_add_a
			ld  (hl), e
			ld  hl, _co_mx
			ld  a, c
			call pn_add_a
			ld  (hl), b
			ld  hl, _co_y
			ld  a, c
			call pn_add_a
			ld  a, (__en_y)
			sub 2
			ld  (hl), a
	#endasm
}

void pn_cocos_do (void) {
	// for each coco on:
	//	pn_ex = co_x + co_mx; if (pn_ex < 0 || pn_ex > 232) off, next;
	//	co_x = pn_ex;
	//	pn_cx = (co_x + 4) >> 4; pn_cy = (co_y + 4) >> 4; if (pn_attr () & 8) off;
	//	the player's box, 8 px wide at gpx + 4, from gpy - VSTRETCH to gpy + 15:
	//	if (co_x + 7 >= gpx + 4 && co_x <= gpx + 11 &&
	//	    co_y + 7 + PLAYER_VSTRETCH_FG >= gpy && co_y <= gpy + 15) {
	//		off; pn_player_hurt ();
	//	}
	#asm
			xor a
			ld  (_pn_j), a
		.pn_cd_loop
			ld  hl, _co_on
			ld  a, (_pn_j)
			call pn_add_a
			ld  a, (hl)
			or  a
			jp  z, pn_cd_next
			// pn_ex = co_x + co_mx
			ld  hl, _co_mx
			ld  a, (_pn_j)
			call pn_add_a
			ld  e, (hl)
			ld  d, 0
			bit 7, e
			jr  z, pn_cd_mxp
			dec d
		.pn_cd_mxp
			ld  hl, _co_x
			ld  a, (_pn_j)
			call pn_add_a
			push hl
			ld  a, (hl)
			ld  l, a
			ld  h, 0
			add hl, de
			ld  (_pn_ex), hl
			pop de 						// DE -> co_x
			ld  a, h
			or  a
			jr  nz, pn_cd_off 			// < 0 or > 255
			ld  a, l
			cp  233
			jr  nc, pn_cd_off
			ld  (de), a
			// (co_x + 4) >> 4, (co_y + 4) >> 4 (9 bits)
			add a, 4
			rra
			srl a
			srl a
			srl a
			ld  (_pn_cx), a
			ld  hl, _co_y
			ld  a, (_pn_j)
			call pn_add_a
			ld  a, (hl)
			add a, 4
			rra
			srl a
			srl a
			srl a
			ld  (_pn_cy), a
			call _pn_attr
			and 8
			jr  nz, pn_cd_off
			// collision with the player
			ld  hl, _co_x
			ld  a, (_pn_j)
			call pn_add_a
			ld  c, (hl) 				// co_x
			ld  a, (_gpx)
			ld  b, a 					// gpx
			ld  a, c
			add a, 3 					// co_x + 3 >= gpx
			jr  c, pn_cd_c1
			cp  b
			jr  c, pn_cd_next
		.pn_cd_c1
			ld  a, b
			add a, 11 					// gpx + 11 >= co_x
			jr  c, pn_cd_c2
			cp  c
			jr  c, pn_cd_next
		.pn_cd_c2
			ld  hl, _co_y
			ld  a, (_pn_j)
			call pn_add_a
			ld  c, (hl) 				// co_y
			ld  a, (_gpy)
			ld  b, a 					// gpy
			ld  a, c
			add a, 7 + PLAYER_VSTRETCH_FG 	// co_y + 15 >= gpy
			jr  c, pn_cd_c3
			cp  b
			jr  c, pn_cd_next
		.pn_cd_c3
			ld  a, b
			add a, 15 					// gpy + 15 >= co_y
			jr  c, pn_cd_c4
			cp  c
			jr  c, pn_cd_next
		.pn_cd_c4
			ld  hl, _co_on
			ld  a, (_pn_j)
			call pn_add_a
			ld  (hl), 0
			call _pn_player_hurt
			jr  pn_cd_next
		.pn_cd_off
			ld  hl, _co_on
			ld  a, (_pn_j)
			call pn_add_a
			ld  (hl), 0
		.pn_cd_next
			ld  a, (_pn_j)
			inc a
			ld  (_pn_j), a
			cp  PN_COCOS
			jp  c, pn_cd_loop
	#endasm
}

// ----------------------------------------------------------------------------
// Engine hooks

void extra_enems_init (void) {
	// enemies come back to life when the screen is entered again:
	// if ((_en_t & 128) && (_en_t & 127) < TYPE_WARPER) {
	//	_en_t &= 127; _en_life = ENEMS_LIFE_GAUGE; }
	// en_face [enit] = _en_mx < 0; en_ct = en_hitct = 0; en_pc = 0xff;
	// en_ox [enit] = _en_x;
	#asm
			ld  a, (__en_t)
			bit 7, a
			jr  z, pn_ei_alive
			and 127
			cp  TYPE_WARPER
			jr  nc, pn_ei_alive
			ld  (__en_t), a
			ld  a, ENEMS_LIFE_GAUGE
			ld  (__en_life), a
		.pn_ei_alive
			ld  a, (__en_mx)
			rlca
			and 1
			ld  c, a
			ld  hl, _en_face
			call pn_ei
			ld  (hl), c
			ld  hl, _en_ct
			call pn_ei
			ld  (hl), 0
			ld  hl, _en_hitct
			call pn_ei
			ld  (hl), 0
			ld  hl, _en_pc
			call pn_ei
			ld  (hl), 0xff
			ld  a, (__en_x)
			ld  c, a
			ld  hl, _en_ox
			call pn_ei
			ld  (hl), c
	#endasm
}

void pn_release_piece (void) {
	// if (en_pc [enit] != 0xff) { pc_on [en_pc [enit]] = 0; en_pc [enit] = 0xff; }
	#asm
			ld  hl, _en_pc
			call pn_ei
			ld  a, (hl)
			cp  0xff
			ret z
			ld  (hl), 0xff
			ld  hl, _pc_on
			call pn_add_a
			ld  (hl), 0
	#endasm
}

void extra_enems_move (void) {
	// pn_t1 = _en_t;
	// warper (>= TYPE_WARPER): pn_f = F_WA_0 + ((maincounter >> 1) & 3), pn_i = 0
	// else:
	//	if (_en_mx > 0) en_face = 0; else if (_en_mx < 0) en_face = 1;
	//	stunned (en_hitct): -- en_hitct; _en_x = en_ox; pn_ex = _en_x + en_recd;
	//	  if (pn_ex >= _en_x1 && pn_ex <= _en_x2) { pn_cy = (_en_y + 8) >> 4;
	//	  pn_cx = en_recd < 0 ? pn_ex >> 4 : (pn_ex + 15) >> 4;
	//	  if (!(pn_attr () & 8)) _en_x = pn_ex; }
	//	pn_f = ((_en_x + 4) >> 3) & 1;
	//	punchy: if (en_ct) { -- en_ct; _en_x = en_ox; pn_f = F_PU_PUNCH;
	//	    if (!en_ct) pn_release_piece (); }
	//	  else if (!en_hitct && sees && |_en_x - gpx| < 16) { en_ct = ATTACK;
	//	    _en_x = en_ox; pn_ex = face ? _en_x - 8 : _en_x + 16; pn_ey = _en_y - 7;
	//	    if (pn_ex + 7 >= gpx + 4 && pn_ex <= gpx + 11 &&
	//	      pn_ey + 7 + VSTRETCH >= gpy && pn_ey <= gpy + 13) pn_player_hurt ();
	//	    peta_el_beeper (0); a free piece 1-2: en_pc, pc_on, pc_f = PC_PU_PUNCH,
	//	    pc_left = face, pc_x = pn_ex, pc_y = _en_y - 16 + PD_PU_PUNCH;
	//	    pn_f = F_PU_PUNCH; }
	//	  else pn_f += F_PU_WALK1;  if (en_hitct) pn_f = F_PU_HIT;
	//	shooty: the same with (rand () & PN_SHOOT_MASK) == 0 instead of the
	//	  distance, pn_coco_shoot (), F_SH_*
	//	linear: pn_f += F_LI_WALK1
	//	pn_i = en_face;
	// en_ox = _en_x; pn_a = 1 + enit; pn_frame (); en_an_next_frame = pn_fp;
	#asm
			ld  a, (__en_t)
			ld  (_pn_t1), a
			cp  TYPE_WARPER
			jr  c, pn_em_enemy
			ld  a, (_maincounter)
			srl a
			and 3
			add a, F_WA_0
			ld  (_pn_f), a
			xor a
			ld  (_pn_i), a
			jp  pn_em_store

		.pn_em_enemy
			ld  a, (__en_mx)
			or  a
			jr  z, pn_em_faced
			rlca
			and 1
			ld  c, a
			ld  hl, _en_face
			call pn_ei
			ld  (hl), c
		.pn_em_faced

			// stunned and pushed back
			ld  hl, _en_hitct
			call pn_ei
			ld  a, (hl)
			or  a
			jp  z, pn_em_walk
			dec (hl)
			call pn_em_back
			ld  hl, _en_recd
			call pn_ei
			ld  e, (hl)
			ld  d, 0
			bit 7, e
			jr  z, pn_em_rp
			dec d
		.pn_em_rp
			ld  a, (__en_x)
			ld  l, a
			ld  h, 0
			add hl, de
			ld  (_pn_ex), hl
			ld  a, h
			or  a
			jr  nz, pn_em_walk 			// < 0 (< _en_x1) or > 255 (> _en_x2)
			ld  a, (__en_x1)
			ld  c, a
			ld  a, l
			cp  c
			jr  c, pn_em_walk
			ld  a, (__en_x2)
			cp  l
			jr  c, pn_em_walk
			ld  a, (__en_y)
			add a, 8
			rra
			srl a
			srl a
			srl a
			ld  (_pn_cy), a
			ld  a, l
			bit 7, d 					// en_recd < 0
			jr  nz, pn_em_rl
			add a, 15
			rra
			jr  pn_em_rx
		.pn_em_rl
			srl a
		.pn_em_rx
			srl a
			srl a
			srl a
			ld  (_pn_cx), a
			call _pn_attr
			and 8
			jr  nz, pn_em_walk
			ld  a, (_pn_ex)
			ld  (__en_x), a

		.pn_em_walk
			ld  a, (__en_x)
			add a, 4
			rrca
			rrca
			rrca
			and 1
			ld  (_pn_f), a
			ld  a, (_pn_t1)
			cp  1
			jr  z, pn_em_punchy
			cp  2
			jp  z, pn_em_shooty
			ld  a, (_pn_f)
			add a, F_LI_WALK1
			ld  (_pn_f), a
			jp  pn_em_face

			// ---- punchy
		.pn_em_punchy
			ld  hl, _en_ct
			call pn_ei
			ld  a, (hl)
			or  a
			jr  z, pn_em_pu_try
			dec (hl)
			push af
			call pn_em_back
			ld  a, F_PU_PUNCH
			ld  (_pn_f), a
			pop af
			call z, _pn_release_piece
			jp  pn_em_pu_hit
		.pn_em_pu_try
			ld  hl, _en_hitct
			call pn_ei
			ld  a, (hl)
			or  a
			jp  nz, pn_em_pu_walk
			call pn_em_sees
			jp  c, pn_em_pu_walk
			ld  a, (_gpx)
			ld  c, a
			ld  a, (__en_x)
			sub c
			jr  nc, pn_em_pu_d
			neg
		.pn_em_pu_d
			cp  16
			jp  nc, pn_em_pu_walk
			// punch
			ld  hl, _en_ct
			call pn_ei
			ld  (hl), PN_EN_ATTACK
			call pn_em_back
			ld  hl, _en_face
			call pn_ei
			ld  a, (hl)
			ld  (pn_em_fc), a
			ld  a, (__en_x)
			ld  l, a
			ld  h, 0
			ld  a, (pn_em_fc)
			or  a
			ld  de, 16
			jr  z, pn_em_pu_x
			ld  de, -8
		.pn_em_pu_x
			add hl, de
			ld  (_pn_ex), hl
			ld  a, (__en_y)
			ld  l, a
			ld  h, 0
			ld  de, -7
			add hl, de
			ld  (_pn_ey), hl
			// the fist against the player's box
			ld  hl, (_pn_ex)
			ld  de, 3
			add hl, de
			ld  a, (_gpx)
			ld  e, a
			ld  d, 0
			call pn_ge 					// pn_ex + 3 >= gpx
			jr  c, pn_em_pu_miss
			ld  a, (_gpx)
			ld  l, a
			ld  h, 0
			ld  de, 11
			add hl, de
			ld  de, (_pn_ex)
			call pn_ge 					// gpx + 11 >= pn_ex
			jr  c, pn_em_pu_miss
			ld  hl, (_pn_ey)
			ld  de, 7 + PLAYER_VSTRETCH_FG
			add hl, de
			ld  a, (_gpy)
			ld  e, a
			ld  d, 0
			call pn_ge 					// pn_ey + 15 >= gpy
			jr  c, pn_em_pu_miss
			ld  a, (_gpy)
			ld  l, a
			ld  h, 0
			ld  de, 13
			add hl, de
			ld  de, (_pn_ey)
			call pn_ge 					// gpy + 13 >= pn_ey
			jr  c, pn_em_pu_miss
			call _pn_player_hurt
		.pn_em_pu_miss
			ld  l, 0
			call _peta_el_beeper
			// a free piece (1, 2) for the fist
			ld  a, 1
		.pn_em_pu_pc
			ld  (_pn_j), a
			ld  hl, _pc_on
			call pn_add_a
			ld  a, (hl)
			or  a
			jr  z, pn_em_pu_pcfree
			ld  a, (_pn_j)
			inc a
			cp  PN_PIECES
			jr  c, pn_em_pu_pc
			ld  (_pn_j), a
			jr  pn_em_pu_pcdone
		.pn_em_pu_pcfree
			ld  (hl), 1
			ld  a, (_pn_j)
			ld  c, a
			ld  hl, _en_pc
			call pn_ei
			ld  (hl), c
			ld  hl, _pc_f
			ld  a, c
			call pn_add_a
			ld  (hl), PC_PU_PUNCH
			ld  hl, _pc_left
			ld  a, c
			call pn_add_a
			ld  a, (pn_em_fc)
			ld  (hl), a
			ld  hl, _pc_x
			ld  a, c
			add a, a
			call pn_add_a
			ld  de, (_pn_ex)
			ld  (hl), e
			inc hl
			ld  (hl), d
			ld  hl, _pc_y
			ld  a, c
			add a, a
			call pn_add_a
			push hl
			ld  a, (__en_y)
			ld  l, a
			ld  h, 0
			ld  de, -16 + PD_PU_PUNCH
			add hl, de
			ex  de, hl
			pop hl
			ld  (hl), e
			inc hl
			ld  (hl), d
		.pn_em_pu_pcdone
			ld  a, F_PU_PUNCH
			ld  (_pn_f), a
			jr  pn_em_pu_hit
		.pn_em_pu_walk
			ld  a, (_pn_f)
			add a, F_PU_WALK1
			ld  (_pn_f), a
		.pn_em_pu_hit
			ld  hl, _en_hitct
			call pn_ei
			ld  a, (hl)
			or  a
			jr  z, pn_em_face
			ld  a, F_PU_HIT
			ld  (_pn_f), a
			jr  pn_em_face

			// ---- shooty
		.pn_em_shooty
			ld  hl, _en_ct
			call pn_ei
			ld  a, (hl)
			or  a
			jr  z, pn_em_sh_try
			dec (hl)
			call pn_em_back
			ld  a, F_SH_SHOOT
			ld  (_pn_f), a
			jr  pn_em_sh_hit
		.pn_em_sh_try
			ld  hl, _en_hitct
			call pn_ei
			ld  a, (hl)
			or  a
			jr  nz, pn_em_sh_walk
			call pn_em_sees
			jr  c, pn_em_sh_walk
			call _rand
			ld  a, l
			and PN_SHOOT_MASK
			jr  nz, pn_em_sh_walk
			ld  hl, _en_ct
			call pn_ei
			ld  (hl), PN_EN_ATTACK
			call pn_em_back
			call _pn_coco_shoot
			ld  a, F_SH_SHOOT
			ld  (_pn_f), a
			jr  pn_em_sh_hit
		.pn_em_sh_walk
			ld  a, (_pn_f)
			add a, F_SH_WALK1
			ld  (_pn_f), a
		.pn_em_sh_hit
			ld  hl, _en_hitct
			call pn_ei
			ld  a, (hl)
			or  a
			jr  z, pn_em_face
			ld  a, F_SH_HIT
			ld  (_pn_f), a

		.pn_em_face
			ld  hl, _en_face
			call pn_ei
			ld  a, (hl)
			ld  (_pn_i), a

		.pn_em_store
			ld  a, (__en_x)
			ld  c, a
			ld  hl, _en_ox
			call pn_ei
			ld  (hl), c
			ld  a, (_enit)
			inc a
			ld  (_pn_a), a
			call _pn_frame
			ld  hl, _en_an_next_frame
			ld  a, (_enit)
			add a, a
			call pn_add_a
			ld  de, (_pn_fp)
			ld  (hl), e
			inc hl
			ld  (hl), d
			ret

			// _en_x = en_ox [enit] (back where it was: it doesn't walk)
		.pn_em_back
			ld  hl, _en_ox
			call pn_ei
			ld  a, (hl)
			ld  (__en_x), a
			ret
		.pn_em_fc
			defb 0
	#endasm
}

void extra_enems_checks (void) {
	// if (_en_t & 128) return;
	// warper: if (collide_enem ()) { pn_warp_t = _en_t - TYPE_WARPER + 1;
	//	pn_warp_x = _en_x2; pn_warp_y = _en_y2; } return;
	// the player's punch / kick:
	// if (pn_hitact && pn_hx + 6 >= _en_x && pn_hx <= _en_x + 14 &&
	//	pn_hy + 7 + ENEMS_VSTRETCH_FG >= _en_y && pn_hy <= _en_y + 12) {
	//	pn_hitact = 0; pn_frozen = PN_FROZEN_FRAMES; en_hitct = PN_EN_TOUCHED;
	//	en_recd = player.facing ? -PN_EN_RECOIL : PN_EN_RECOIL;
	//	en_ct = 0; pn_release_piece (); enems_kill (1);
	// }
	#asm
			ld  a, (__en_t)
			bit 7, a
			ret nz
			cp  TYPE_WARPER
			jr  c, pn_ec_enemy
			call _collide_enem
			ld  a, l
			or  a
			ret z
			ld  a, (__en_t)
			sub TYPE_WARPER - 1
			ld  (_pn_warp_t), a
			ld  a, (__en_x2)
			ld  (_pn_warp_x), a
			ld  a, (__en_y2)
			ld  (_pn_warp_y), a
			ret

		.pn_ec_enemy
			ld  a, (_pn_hitact)
			or  a
			ret z
			ld  hl, (_pn_hx)
			ld  de, 6
			add hl, de
			ld  a, (__en_x)
			ld  e, a
			ld  d, 0
			call pn_ge 					// pn_hx + 6 >= _en_x
			ret c
			ld  a, (__en_x)
			ld  l, a
			ld  h, 0
			ld  de, 14
			add hl, de
			ld  de, (_pn_hx)
			call pn_ge 					// _en_x + 14 >= pn_hx
			ret c
			ld  hl, (_pn_hy)
			ld  de, 7 + ENEMS_VSTRETCH_FG
			add hl, de
			ld  a, (__en_y)
			ld  e, a
			ld  d, 0
			call pn_ge 					// pn_hy + 15 >= _en_y
			ret c
			ld  a, (__en_y)
			ld  l, a
			ld  h, 0
			ld  de, 12
			add hl, de
			ld  de, (_pn_hy)
			call pn_ge 					// _en_y + 12 >= pn_hy
			ret c

			xor a
			ld  (_pn_hitact), a
			ld  a, PN_FROZEN_FRAMES
			ld  (_pn_frozen), a
			ld  hl, _en_hitct
			call pn_ei
			ld  (hl), PN_EN_TOUCHED
			ld  a, (_player + 22) 		// facing
			or  a
			ld  c, PN_EN_RECOIL
			jr  z, pn_ec_rec
			ld  c, 256 - PN_EN_RECOIL
		.pn_ec_rec
			ld  hl, _en_recd
			call pn_ei
			ld  (hl), c
			ld  hl, _en_ct
			call pn_ei
			ld  (hl), 0
			call _pn_release_piece
			ld  hl, 1
			push hl
			call _enems_kill
			pop hl
	#endasm
}

void extra_enems_killed (void) {
	// pn_release_piece ();
	#asm
			call _pn_release_piece
	#endasm
}

unsigned char this_enemy_kills (void) {
	// return _en_t < TYPE_WARPER;
	#asm
			ld  hl, 0
			ld  a, (__en_t)
			cp  TYPE_WARPER
			ret nc
			inc l
	#endasm
}
