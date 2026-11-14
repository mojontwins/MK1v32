// Sir Ababol 2 - the game: the original's engine.h, printer.h and
// levelmanager.h on top of MK1. Hand-written assembly; the original C is in
// the comments. Included from custom.h.
//
// Tiles: 0 background (backdrop mountains 19-21 in the lower rows, sparkle
// 18 at random), 11 ice block (beh 24: solid, slippery, breakable), 12 spikes
// (beh 1), 13 gate (opened with a key, flood fill), 14 key, 15 coin, 16 life,
// 17 a breaking block. The map is 4 bits per tile: what is picked, broken or
// opened is written back to it (it stays so for the rest of the level).

#define SA2_ST_NORMAL 		0
#define SA2_ST_ICE 			1		// frigoababol
#define SA2_ST_STOP 		2		// a boss is dying

#define SA2_EN_NORMAL 		0
#define SA2_EN_PARP 		2		// boss hit
#define SA2_EN_MUR 			4		// boss dying
#define SA2_EN_PARALYZED 	16		// stunned

#define SA2_SW_RIGHT 		1		// sprite frames 19, 20, 21
#define SA2_SW_LEFT 		2
#define SA2_SW_UP 			3

#define SA2_COUNTER 		32		// frigoababol, stunned enemies, flickering
#define SA2_AX_ICE 			4		// PLAYER_AXF, PLAYER_RXF
#define SA2_BRK_MAX 		2
#define SA2_BRK_FRAMES 		6
#define SA2_TILE_BREAKING 	17
#define SA2_TILE_SPARKLE 	18

// Effects (the original's numbers)
#define SA2_SFX_BREAK_END 	0
#define SA2_SFX_SWORD 		4
#define SA2_SFX_LIFE 		5
#define SA2_SFX_HIT 		6
#define SA2_SFX_FROZEN 		7
#define SA2_SFX_GET 		9
#define SA2_SFX_SHAKE 		10
#define SA2_SFX_JUMP 		11

// printer.h: the tiled background of the 48K and CPC versions (rows 5-9)
unsigned char sa2_backdrop [] = {
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,19,21, 0,
	 0, 0, 0,19,21, 0, 0, 0, 0, 0, 0,19,20,20,21,
	 0, 0,19,20,20,21, 0, 0, 0, 0,19,20,20,20,20,
	 0,19,20,20,20,20,21, 0, 0,19,20,20,20,20,20,
	19,20,20,20,20,20,20,21,19,20,20,20,20,20,20
};

// The sword: 6 frames (the original had 9: { 8, 10, 12, 14, 15, 15, 14, 13, 10 },
// { 2, 2, 2, 3, 4, 4, 5, 6, 7 }, breaking on 3-5, hitting on 3-6)
#define SA2_SW_FRAMES 		6
#define SA2_SW_BREAK 		2		// frames 2-3 break blocks
#define SA2_SW_BREAK_N 		2
#define SA2_SW_HIT 			2		// frames 2-4 hit enemies (checked a frame later)
#define SA2_SW_HIT_N 		3
unsigned char sa2_swoffs_x [] = { 8, 11, 14, 15, 14, 11 };
unsigned char sa2_swoffs_y [] = { 2,  2,  3,  4,  5,  7 };
unsigned char sa2_walk [] = { 0, 1, 2, 1 };

unsigned char sa2_state, sa2_ice_ct, sa2_ax, sa2_rx, sa2_pax, sa2_prx;
unsigned char sa2_ptx1, sa2_ptx2, sa2_ptx3, sa2_pty1, sa2_pty2;
unsigned char sa2_coins, sa2_swording, sa2_carry, sa2_sv_life, sa2_sv_keys;
unsigned char sa2_o_coins, sa2_o_keys;
unsigned char sw_on, sw_type, sw_f, sw_x, sw_y;
unsigned char brk_x [SA2_BRK_MAX], brk_y [SA2_BRK_MAX], brk_f [SA2_BRK_MAX];
unsigned char sa2_en_st [3], sa2_en_ct [3];
unsigned char sa2_ccx, sa2_ccy;

#asm
	// HL += A
	.sa2_add_a
		add a, l
		ld  l, a
		ret nc
		inc h
		ret

	// A = C * 15 + B (B = x, C = y). Keeps BC, DE, HL.
	.sa2_idx
		ld  a, c
		add a, a
		add a, a
		add a, a
		add a, a
		sub c
		add a, b
		ret

	// A = map_attr at tile (B, C); 0 outside the screen. Keeps BC, DE.
	.sa2_attr
		ld  hl, _map_attr
	.sa2_attr_hl
		ld  a, c
		cp  10
		jr  nc, sa2_attr_0
		ld  a, b
		cp  15
		jr  nc, sa2_attr_0
		call sa2_idx
		call sa2_add_a
		ld  a, (hl)
		ret
	.sa2_attr_0
		xor a
		ret

	// A = map_buff at tile (B, C); 0 outside the screen. Keeps BC, DE.
	.sa2_tile
		ld  hl, _map_buff
		jr  sa2_attr_hl

	// A = (A >> 4)
	.sa2_shr4
		rrca
		rrca
		rrca
		rrca
		and 15
		ret

	// A = A * 16
	.sa2_shl4
		add a, a
		add a, a
		add a, a
		add a, a
		ret

	// gpy = A, player.y = A << 6
	.sa2_set_gpy
		ld  (_gpy), a
		call Ashl16_HL
		ld  (_player + 2), hl
		ret

	// gpx = A, player.x = A << 6
	.sa2_set_gpx
		ld  (_gpx), a
		call Ashl16_HL
		ld  (_player), hl
		ret

	// The original's xorshift (engine.h rand). A = random. Trashes HL, DE.
	.sa2_rand
		ld  hl, 0xA280
		ld  de, 0xC0DE
		ld  a, h
		add a, a
		xor h
		ld  h, l
		ld  l, d
		ld  d, e
		ld  e, a
		rra
		xor e
		ld  e, a
		ld  a, d
		add a, a
		add a, a
		add a, a
		xor d
		xor e
		ld  e, a
		ld  (sa2_rand + 1), hl
		ld  (sa2_rand + 4), de
		ret

	// What tile A at (B, C) looks like (draw_map_tile): tile 0 is the
	// backdrop in rows 5-9, and a sparkle once in 16. Keeps BC.
	.sa2_subst
		or  a
		ret nz
		ld  a, c
		sub 5
		jr  c, sa2_sb_spark
		ld  e, a
		call sa2_shl4
		sub e
		add a, b
		ld  hl, _sa2_backdrop
		call sa2_add_a
		ld  a, (hl)
		or  a
		ret nz
	.sa2_sb_spark
		push bc
		call sa2_rand
		pop bc
		and 15
		cp  1
		ld  a, 0
		ret nz
		ld  a, SA2_TILE_SPARKLE
		ret

	// Draw tile A at (B, C) as it looks. Keeps BC.
	.sa2_draw_s
		call sa2_subst
	.sa2_draw
		push bc
		ld  (__t), a
		ld  a, b
		add a, a
		add a, VIEWPORT_X
		ld  (__x), a
		ld  a, c
		add a, a
		add a, VIEWPORT_Y
		ld  (__y), a
		call _draw_coloured_tile_do
		pop bc
		ret

	// Tile A at (B, C): map_buff, map_attr (its behaviour), drawn. Keeps BC.
	.sa2_set_tile
		ld  d, a
		call sa2_idx
		ld  e, a
		ld  hl, _map_buff
		call sa2_add_a
		ld  (hl), d
		ld  hl, _comportamiento_tiles
		ld  a, d
		call sa2_add_a
		ld  a, (hl)
		push af
		ld  hl, _map_attr
		ld  a, e
		call sa2_add_a
		pop af
		ld  (hl), a
		ld  a, d
		jr  sa2_draw_s

	// set_map_tile: tile A (0-15) at (B, C) of this screen into the packed
	// map. Keeps BC (not A).
	.sa2_map_put
		push bc
		and 15
		ld  d, a
		call sa2_idx
		ld  e, a
		ld  hl, 0
		ld  bc, 150
		ld  a, (_n_pant)
		or  a
		jr  z, sa2_mp_1
	.sa2_mp_0
		add hl, bc
		dec a
		jr  nz, sa2_mp_0
	.sa2_mp_1
		ld  a, e
		call sa2_add_a
		ld  bc, _mapa
		srl h
		rr  l
		jr  c, sa2_mp_odd
		add hl, bc
		ld  a, (hl)
		and 0x0f
		ld  c, a
		ld  a, d
		call sa2_shl4
		or  c
		ld  (hl), a
		pop bc
		ret
	.sa2_mp_odd
		add hl, bc
		ld  a, (hl)
		and 0xf0
		or  d
		ld  (hl), a
		pop bc
		ret

	// Lose a life (decrease_life). 0 is game over (MK1: life < 0).
	.sa2_hurt
		ld  hl, (_player + 29)
		ld  a, h
		or  l
		ret z
		dec hl
		ld  a, h
		or  l
		jr  nz, sa2_hurt_1
		dec hl
	.sa2_hurt_1
		ld  (_player + 29), hl
		ret

	// One more life (99 max)
	.sa2_life_up
		ld  hl, (_player + 29)
		ld  a, l
		cp  99
		jr  nc, sa2_lu_1
		inc hl
		ld  (_player + 29), hl
	.sa2_lu_1
		ld  l, SA2_SFX_LIFE
		jp  _peta_el_beeper

	// Flicker (EST_PARP, counted down by the main loop)
	.sa2_flicker
		ld  a, EST_PARP
		ld  (_player + 23), a
		ld  a, SA2_COUNTER
		ld  (_player + 24), a
		ret

	// open_gate (B, C): a gate and a key: the gate (every 13 touching it) goes.
	.sa2_open_gate
		call sa2_tile
		cp  13
		ret nz
		ld  a, (_player + 28) 		// keys
		or  a
		ret z
		dec a
		ld  (_player + 28), a
	.sa2_gate_rec
		call sa2_tile
		cp  13
		ret nz
		xor a
		call sa2_map_put 		// (A is the map byte now)
		xor a
		call sa2_set_tile 		// 0: behaviour 0
		dec b
		call sa2_gate_rec
		inc b
		inc b
		call sa2_gate_rec
		dec b
		dec c
		call sa2_gate_rec
		inc c
		inc c
		call sa2_gate_rec
		dec c
		ret

	// get_from (B, C): tile 14 key, 15 coin, 16 life
	.sa2_get_from
		call sa2_tile
		cp  14
		jr  nz, sa2_gf_coin
		ld  hl, _player + 28
		inc (hl)
		ld  l, SA2_SFX_GET
		jr  sa2_gf_done
	.sa2_gf_coin
		cp  15
		jr  nz, sa2_gf_life
		ld  a, (_sa2_coins)
		inc a
		jr  z, sa2_gf_c1 		// 255 max
		ld  (_sa2_coins), a
	.sa2_gf_c1
		push bc
		ld  l, SA2_SFX_GET
		call _peta_el_beeper
		pop bc
		ld  a, (_sa2_coins)
		cp  25
		jr  nz, sa2_gf_clear
		xor a
		ld  (_sa2_coins), a
	.sa2_gf_life
		push bc
		call sa2_life_up
		pop bc
		jr  sa2_gf_clear
	.sa2_gf_done
		push bc
		call _peta_el_beeper
		pop bc
	.sa2_gf_clear
		xor a
		call sa2_map_put 		// (A is the map byte now)
		xor a
		jp  sa2_set_tile

	// delete_tile (B, C): the sword breaks an ice block (beh & 16)
	.sa2_delete_tile
		call sa2_attr
		and 16
		ret z
		ld  hl, _brk_f
		ld  e, 0
	.sa2_dt_find
		ld  a, (hl)
		or  a
		jr  z, sa2_dt_found
		inc hl
		inc e
		ld  a, e
		cp  SA2_BRK_MAX
		jr  c, sa2_dt_find
		ret
	.sa2_dt_found
		ld  (hl), 1
		ld  hl, _brk_x
		ld  a, e
		call sa2_add_a
		ld  (hl), b
		ld  hl, _brk_y
		ld  a, e
		call sa2_add_a
		ld  (hl), c
		xor a
		call sa2_map_put
		ld  hl, _map_attr
		call sa2_idx
		call sa2_add_a
		ld  (hl), 8
		ld  a, SA2_TILE_BREAKING
		call sa2_draw
		ld  l, SA2_SFX_HIT
		jp  _peta_el_beeper
#endasm

// ----------------------------------------------------------------------------
// The screen renderer (CUSTOM_RENDERER): the original's draw_map_tile, the
// random sequence seeded per screen (y_pant, level) as in render_screen.

void custom_renderer (void) {
	#asm
			ld  a, (_rdi)
			or  a
			jr  nz, sa2_cr_1
			ld  a, (_n_pant)
			ld  l, a
			ld  h, 0
			ld  (sa2_rand + 1), hl
			ld  a, (_level)
			ld  l, a
			ld  (sa2_rand + 4), hl
		.sa2_cr_1
			ld  a, (_rdx)
			srl a
			ld  b, a
			ld  a, (_rdy)
			srl a
			ld  c, a
			ld  a, (__t)
			call sa2_subst
			ld  (__t), a
	#endasm
}

// ----------------------------------------------------------------------------
// Breaking blocks (process_breakable): after 6 frames, nothing, a coin (rand
// & 7 == 1) or a life (rand & 31 == 1); only in map_buff.

void sa2_breakables_do (void) {
	#asm
			ld  e, 0
		.sa2_bd_loop
			ld  hl, _brk_f
			ld  a, e
			call sa2_add_a
			ld  a, (hl)
			or  a
			jr  z, sa2_bd_next
			inc a
			ld  (hl), a
			cp  SA2_BRK_FRAMES
			jr  nz, sa2_bd_next
			ld  (hl), 0
			push de
			ld  hl, _brk_x
			ld  a, e
			call sa2_add_a
			ld  b, (hl)
			ld  hl, _brk_y
			ld  a, e
			call sa2_add_a
			ld  c, (hl)
			push bc
			call sa2_rand
			and 7
			ld  c, 0
			cp  1
			jr  nz, sa2_bd_1
			ld  c, 15
		.sa2_bd_1
			push bc
			call sa2_rand
			pop bc
			and 31
			cp  1
			jr  nz, sa2_bd_2
			ld  c, 16
		.sa2_bd_2
			ld  a, c
			pop bc
			call sa2_set_tile
			call sa2_idx
			ld  hl, _map_attr
			call sa2_add_a
			ld  (hl), 0
			ld  l, SA2_SFX_BREAK_END
			call _peta_el_beeper
			pop de
		.sa2_bd_next
			inc e
			ld  a, e
			cp  SA2_BRK_MAX
			jr  c, sa2_bd_loop
	#endasm
}

// ----------------------------------------------------------------------------
// The sword (render_sword). SA2_SW_FRAMES frames; some break blocks, some hit
// enemies (sa2_enems_checks). sw_x, sw_y: the sprite, in pixels.

void sa2_sword_do (void) {
	#asm
			ld  a, (_sw_f)
			ld  c, a
			ld  b, 0
			ld  hl, _sa2_swoffs_x
			add hl, bc
			ld  d, (hl) 			// D = swoffs_x [f]
			ld  hl, _sa2_swoffs_y
			add hl, bc
			ld  e, (hl) 			// E = swoffs_y [f]

			ld  a, (_sw_type)
			cp  SA2_SW_UP
			jr  nz, sa2_sw_side

			// s_x = px + swoffs_y [f]; s_y = py + 7 - swoffs_x [f];
			ld  a, (_gpx)
			add a, e
			ld  (_sw_x), a
			ld  a, (_gpy)
			add a, 7
			sub d
			ld  (_sw_y), a
			jr  nc, sa2_sw_up_1
			xor a
			ld  (_sw_y), a 			// (s_y < 0: 0, no tile)
			jr  sa2_sw_next
		.sa2_sw_up_1
			call sa2_sw_breaks
			jr  nz, sa2_sw_next
			// delete_tile ((s_x >> 4) + ((s_x & 15) > 11), s_y >> 4);
			ld  a, (_sw_y)
			call sa2_shr4
			ld  c, a
			ld  a, (_sw_x)
			call sa2_round
			ld  b, a
			call sa2_delete_tile
			jr  sa2_sw_next

		.sa2_sw_side
			// s_y = py + swoffs_y [f];
			ld  a, (_gpy)
			add a, e
			ld  (_sw_y), a
			ld  a, (_sw_type)
			cp  SA2_SW_LEFT
			jr  nz, sa2_sw_right
			// s_x = px + 6 - swoffs_x [f];
			ld  a, (_gpx)
			add a, 6
			sub d
			ld  (_sw_x), a
			ld  b, a
			jr  nc, sa2_sw_side_1
			xor a
			ld  (_sw_x), a 			// (s_x < 0: 0, no tile)
			jr  sa2_sw_next
		.sa2_sw_right
			// s_x = px + swoffs_x [f]; tile x = (s_x + 6) >> 4
			ld  a, (_gpx)
			add a, d
			ld  (_sw_x), a
			add a, 6
			ld  b, a
		.sa2_sw_side_1
			call sa2_sw_breaks
			jr  nz, sa2_sw_next
			ld  a, b
			call sa2_shr4
			ld  b, a
			ld  a, (_sw_y)
			call sa2_round
			ld  c, a
			call sa2_delete_tile

		.sa2_sw_next
			// if (++ s_frame == 9) s_on = p_swording = 0;
			ld  a, (_sw_x)
			cp  225
			jr  c, sa2_sw_n1
			ld  a, 224
			ld  (_sw_x), a
		.sa2_sw_n1
			ld  hl, _sw_f
			inc (hl)
			ld  a, (hl)
			cp  SA2_SW_FRAMES
			ret nz
			xor a
			ld  (_sw_on), a
			ld  (_sa2_swording), a
			ret

			// Z if a breaking frame (keeps B)
		.sa2_sw_breaks
			ld  a, (_sw_f)
			sub SA2_SW_BREAK
			cp  SA2_SW_BREAK_N
			ld  a, 0
			jr  c, sa2_sw_b1
			inc a
		.sa2_sw_b1
			or  a
			ret

			// A = (A >> 4) + ((A & 15) > 11)
		.sa2_round
			ld  e, a
			call sa2_shr4
			ld  l, a
			ld  a, e
			and 15
			cp  12
			ld  a, l
			ret c
			inc a
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// The player (PLAYER_CUSTOM_MOVE): the original's move (), 1:1. Collision box
// x + 4..11, y + 8..15. Jump: BUTTON_B (FIRE2), the sword: BUTTON_A (FIRE).

void player_custom_move (void) {
	#asm
			ld  a, (_sa2_state)
			cp  SA2_ST_STOP
			ret z

			// ---- Vertical
			// if (p_vy < MAX_VY_CAYENDO) p_vy += p_g; else p_vy = MAX_VY_CAYENDO;
			ld  hl, (_player + 8)
			ld  de, PLAYER_MAX_VY_CAYENDO
			or  a
			sbc hl, de
			add hl, de
			jp  p, sa2_g_cap
			ld  de, PLAYER_G
			add hl, de
			ex  de, hl
		.sa2_g_cap
			ex  de, hl
			ld  (_player + 8), hl

			// p_y += p_vy; 0 <= p_y <= 9216
			ex  de, hl
			ld  hl, (_player + 2)
			add hl, de
			bit 7, h
			jr  z, sa2_y_1
			ld  hl, 0
			jr  sa2_y_w
		.sa2_y_1
			ld  de, 144 * 64
			or  a
			sbc hl, de
			add hl, de
			jp  m, sa2_y_w
			ex  de, hl
		.sa2_y_w
			ld  (_player + 2), hl
			call HLshr6_A
			ld  (_gpy), a

			call sa2_points
			xor a
			ld  (_hit_v), a
			ld  (_hit_h), a

			ld  hl, (_player + 8)
			ld  a, h
			or  l
			jp  z, sa2_vc_done
			bit 7, h
			jr  z, sa2_vc_down

			// Up: ceilings (beh 8) at (ptx1, pty1), (ptx2, pty1)
			ld  a, (_sa2_pty1)
			ld  c, a
			call sa2_two_points 	// A = at1 | at2, BC = (ptx2, pty1)
			ld  l, a
			and 8
			jr  z, sa2_vc_evil
			ld  hl, 0
			ld  (_player + 8), hl
			ld  a, (_sa2_pty1)
			inc a
			call sa2_shl4
			sub 8
			call sa2_set_gpy
			ld  a, (_sa2_pty1)
			jr  sa2_vc_gate

		.sa2_vc_down
			// Down: only while the feet are in the upper half of a tile
			ld  a, (_gpy)
			dec a
			and 15
			cp  8
			jr  nc, sa2_vc_done
			ld  a, (_sa2_pty2)
			ld  c, a
			call sa2_two_points
			ld  l, a
			and 12
			jr  z, sa2_vc_evil
			// ... but not into a wall: the tiles above the feet
			dec c
			call sa2_attr 			// (ptx2, pty2 - 1)
			ld  e, a
			ld  a, (_sa2_ptx1)
			ld  b, a
			call sa2_attr
			or  e
			and 8
			jr  nz, sa2_vc_done
			ld  hl, 0
			ld  (_player + 8), hl
			ld  a, (_sa2_pty2)
			dec a
			call sa2_shl4
			call sa2_set_gpy
			ld  a, (_sa2_pty2)

		.sa2_vc_gate
			ld  c, a
			ld  a, (_sa2_ptx3)
			ld  b, a
			call sa2_open_gate
			jr  sa2_vc_done

		.sa2_vc_evil
			ld  a, l
			and 1
			ld  (_hit_v), a
		.sa2_vc_done

			// possee: the tiles under the feet (gpy + 16)
			ld  a, (_gpy)
			add a, 16
			call sa2_shr4
			ld  c, a
			call sa2_two_points
			ld  e, a
			and 12
			ld  (_player + 26), a 	// possee
			// Slippery (beh & 16 under the feet)
			ld  a, (_sa2_ax)
			ld  b, a
			ld  a, (_sa2_rx)
			ld  c, a
			ld  a, e
			and 16
			jr  z, sa2_sl_1
			ld  bc, SA2_AX_ICE * 256 + SA2_AX_ICE
		.sa2_sl_1
			ld  a, b
			ld  (_sa2_pax), a
			ld  a, c
			ld  (_sa2_prx), a

			// ---- Jump (started by a fresh press, thrust while held)
			ld  a, (_pad0)
			and sp_FIRE2
			jr  nz, sa2_j_release
			ld  a, (_player + 19) 	// saltando
			or  a
			jr  nz, sa2_j_go
			ld  a, (_pad_this_frame)
			and sp_FIRE2
			jr  nz, sa2_j_done
			ld  a, (_player + 26) 	// possee
			or  a
			jr  z, sa2_j_done
			ld  a, 1
			ld  (_player + 19), a
			xor a
			ld  (_player + 14), a 	// cont_salto
			ld  l, SA2_SFX_JUMP
			call _peta_el_beeper
		.sa2_j_go
			// p_vy -= (VY_INICIAL_SALTO + INCR_SALTO - (p_cont_salto >> 1)), -MAX_VY_SALTANDO max
			ld  a, (_player + 14)
			srl a
			ld  e, a
			ld  d, 0
			ld  hl, PLAYER_VY_INICIAL_SALTO + PLAYER_INCR_SALTO
			or  a
			sbc hl, de
			ex  de, hl
			ld  hl, (_player + 8)
			or  a
			sbc hl, de
			ld  de, -PLAYER_MAX_VY_SALTANDO
			or  a
			sbc hl, de
			add hl, de
			jp  p, sa2_j_w
			ex  de, hl
		.sa2_j_w
			ld  (_player + 8), hl
			ld  hl, _player + 14
			inc (hl)
			ld  a, (hl)
			cp  PLAYER_JUMP_FRAMES
			jr  c, sa2_j_done
		.sa2_j_release
			xor a
			ld  (_player + 19), a
		.sa2_j_done

			// ---- Horizontal
			ld  a, (_pad0)
			and sp_LEFT | sp_RIGHT
			cp  sp_LEFT | sp_RIGHT
			jr  nz, sa2_h_keys
			// Neither: friction
			ld  a, (_sa2_prx)
			ld  e, a
			ld  d, 0
			ld  hl, (_player + 6)
			ld  a, h
			or  l
			jr  z, sa2_h_keys
			bit 7, h
			jr  nz, sa2_h_fneg
			or  a
			sbc hl, de
			jp  p, sa2_h_fw
			jr  sa2_h_f0
		.sa2_h_fneg
			add hl, de
			bit 7, h
			jr  nz, sa2_h_fw
		.sa2_h_f0
			ld  hl, 0
		.sa2_h_fw
			ld  (_player + 6), hl

		.sa2_h_keys
			ld  a, (_sa2_pax)
			ld  c, a
			ld  b, 0
			ld  a, (_pad0)
			and sp_LEFT
			jr  nz, sa2_h_right
			// if (p_vx > -maxx) { p_facing = left; p_vx -= pax; }
			ld  hl, (_player + 6)
			ld  de, -PLAYER_MAX_VX
			or  a
			sbc hl, de
			add hl, de
			jp  m, sa2_h_right
			jr  z, sa2_h_right
			or  a
			sbc hl, bc
			ld  (_player + 6), hl
			ld  a, SIDEVIEW_FACING_LEFT
			ld  (_player + 22), a
		.sa2_h_right
			ld  a, (_pad0)
			and sp_RIGHT
			jr  nz, sa2_h_move
			// if (p_vx < maxx) { p_vx += pax; p_facing = right; }
			ld  hl, (_player + 6)
			ld  de, PLAYER_MAX_VX
			or  a
			sbc hl, de
			add hl, de
			jp  p, sa2_h_move
			add hl, bc
			ld  (_player + 6), hl
			xor a 					// SIDEVIEW_FACING_RIGHT
			ld  (_player + 22), a

		.sa2_h_move
			// p_x += p_vx; 0 <= p_x <= 14336
			ld  hl, (_player + 6)
			ex  de, hl
			ld  hl, (_player)
			add hl, de
			bit 7, h
			jr  z, sa2_x_1
			ld  hl, 0
			jr  sa2_x_w
		.sa2_x_1
			ld  de, 224 * 64
			or  a
			sbc hl, de
			add hl, de
			jp  m, sa2_x_w
			ex  de, hl
		.sa2_x_w
			ld  (_player), hl
			call HLshr6_A
			ld  (_gpx), a

			call sa2_points
			ld  hl, (_player + 6)
			ld  a, h
			or  l
			jr  z, sa2_hc_done
			bit 7, h
			jr  z, sa2_hc_right
			// Left: (ptx1, pty1), (ptx1, pty2)
			ld  a, (_sa2_ptx1)
			call sa2_two_v 			// A = at1 | at2, BC = (ptx1, pty1)
			ld  l, a
			and 8
			jr  z, sa2_hc_evil
			ld  a, b
			inc a
			call sa2_shl4
			sub 4
			jr  sa2_hc_wall
		.sa2_hc_right
			ld  a, (_sa2_ptx2)
			call sa2_two_v
			ld  l, a
			and 8
			jr  z, sa2_hc_evil
			ld  a, b
			dec a
			call sa2_shl4
			add a, 4
		.sa2_hc_wall
			push bc
			call sa2_set_gpx
			ld  hl, 0
			ld  (_player + 6), hl
			pop bc
			call sa2_open_gate
			jr  sa2_hc_done
		.sa2_hc_evil
			ld  a, l
			and 1
			ld  (_hit_h), a
		.sa2_hc_done

			// ---- Objects: tiles 14-16 at the centre
			ld  a, (_gpx)
			add a, 8
			call sa2_shr4
			ld  b, a
			ld  a, (_gpy)
			add a, 8
			call sa2_shr4
			ld  c, a
			call sa2_tile
			sub 14
			cp  3
			call c, sa2_get_from

			// ---- Evil tile
			ld  a, (_sa2_state)
			ld  c, a
			ld  a, (_player + 23) 	// estado (flickering)
			or  c
			jr  nz, sa2_ev_done
			ld  a, (_hit_v)
			ld  c, a
			ld  a, (_hit_h)
			or  c
			jr  z, sa2_ev_done
			// p_vy = p_vy >= 0 ? -MAX_VX : MAX_VX
			ld  de, PLAYER_MAX_VX
			ld  a, (_player + 9)
			bit 7, a
			jr  nz, sa2_ev_1
			ld  de, -PLAYER_MAX_VX
		.sa2_ev_1
			ld  (_player + 8), de
			call sa2_hurt
			call sa2_flicker
			ld  a, (_gpy)
			and 0xf0
			call sa2_set_gpy
		.sa2_ev_done

			// ---- Frigoababol / sword, frame
			ld  a, (_sa2_state)
			cp  SA2_ST_ICE
			jr  nz, sa2_fr_normal

			ld  c, SA2_CELL_ICE
			ld  a, (_pad_this_frame) 	// FIRE pressed this frame: holding it does not thaw
			and sp_FIRE
			jr  nz, sa2_ice_1
			// Shake: vy, vx += -64, 0 or 64; one less
			ld  l, SA2_SFX_SHAKE
			call _peta_el_beeper
			ld  hl, _player + 8
			call sa2_shake
			ld  hl, _player + 6
			call sa2_shake
			ld  hl, _sa2_ice_ct
			ld  a, (hl)
			or  a
			jr  z, sa2_ice_1
			dec (hl)
		.sa2_ice_1
			ld  a, (_sa2_ice_ct)
			or  a
			jr  nz, sa2_ice_2
			// Thawed: flickering, normal movement
			ld  (_sa2_state), a
			ld  a, PLAYER_AX
			ld  (_sa2_ax), a
			ld  a, PLAYER_RX
			ld  (_sa2_rx), a
			call sa2_flicker
		.sa2_ice_2
			ld  c, SA2_CELL_ICE
			jr  sa2_fr_set

		.sa2_fr_normal
			// The sword (a fresh press of FIRE)
			ld  a, (_pad_this_frame)
			and sp_FIRE
			jr  nz, sa2_fr_1
			ld  a, (_sw_on)
			or  a
			jr  nz, sa2_fr_1
			ld  b, SA2_SW_UP
			ld  a, (_pad0)
			and sp_UP | sp_LEFT | sp_RIGHT
			cp  sp_LEFT | sp_RIGHT 	// UP, neither LEFT nor RIGHT
			jr  z, sa2_fr_sw
			ld  b, SA2_SW_LEFT
			ld  a, (_player + 22)
			or  a
			jr  nz, sa2_fr_sw
			ld  b, SA2_SW_RIGHT
		.sa2_fr_sw
			ld  a, b
			ld  (_sw_type), a
			ld  a, 1
			ld  (_sw_on), a
			ld  (_sa2_swording), a
			xor a
			ld  (_sw_f), a
			ld  l, SA2_SFX_SWORD
			call _peta_el_beeper
		.sa2_fr_1
			// in the air or swinging: the jump frame
			ld  a, (_player + 22) 	// facing: 0 / 4
			ld  c, a
			ld  a, (_player + 26)
			or  a
			jr  z, sa2_fr_air
			ld  a, (_sa2_swording)
			or  a
			jr  nz, sa2_fr_air
			ld  hl, (_player + 6)
			ld  a, h
			or  l
			jr  nz, sa2_fr_walk
			inc c 					// still: frame 1
			jr  sa2_fr_set
		.sa2_fr_walk
			ld  hl, _player + 21 	// subframe
			inc (hl)
			ld  a, (hl)
			cp  4
			jr  nz, sa2_fr_w1
			ld  (hl), 0
			ld  a, (_player + 20) 	// frame (0-3)
			inc a
			and 3
			ld  (_player + 20), a
		.sa2_fr_w1
			ld  a, (_player + 20)
			ld  hl, _sa2_walk
			call sa2_add_a
			ld  a, (hl)
			add a, c
			ld  c, a
			jr  sa2_fr_set
		.sa2_fr_air
			inc c
			inc c
			inc c
		.sa2_fr_set
			// player.next_frame = sprite_cells [C]
			ld  a, c
			add a, a
			ld  hl, _sprite_cells
			call sa2_add_a
			ld  a, (hl)
			inc hl
			ld  h, (hl)
			ld  l, a
			ld  (_player + 17), hl

			ld  a, (_sw_on)
			or  a
			call nz, _sa2_sword_do
			ret

			// ptx1, ptx2, ptx3, pty1, pty2 from gpx, gpy
		.sa2_points
			ld  a, (_gpx)
			ld  c, a
			add a, 4
			call sa2_shr4
			ld  (_sa2_ptx1), a
			ld  a, c
			add a, 11
			call sa2_shr4
			ld  (_sa2_ptx2), a
			ld  a, c
			add a, 8
			call sa2_shr4
			ld  (_sa2_ptx3), a
			ld  a, (_gpy)
			ld  c, a
			add a, 8
			call sa2_shr4
			ld  (_sa2_pty1), a
			ld  a, c
			add a, 15
			call sa2_shr4
			ld  (_sa2_pty2), a
			ret

			// A = attr (ptx1, C) | attr (ptx2, C); BC = (ptx2, C)
		.sa2_two_points
			ld  a, (_sa2_ptx1)
			ld  b, a
			call sa2_attr
			ld  e, a
			ld  a, (_sa2_ptx2)
			ld  b, a
			call sa2_attr
			or  e
			ret

			// A = attr (A, pty1) | attr (A, pty2); BC = (A, pty1)
		.sa2_two_v
			ld  b, a
			ld  a, (_sa2_pty2)
			ld  c, a
			call sa2_attr
			ld  e, a
			ld  a, (_sa2_pty1)
			ld  c, a
			call sa2_attr
			or  e
			ret

			// (HL) (16 bits) += ((rand () % 3) << 6) - 64
		.sa2_shake
			push hl
			call sa2_rand
		.sa2_sh_mod
			cp  3
			jr  c, sa2_sh_1
			sub 3
			jr  sa2_sh_mod
		.sa2_sh_1
			ld  de, -64
			or  a
			jr  z, sa2_sh_2
			ld  de, 0
			dec a
			jr  z, sa2_sh_2
			ld  de, 64
		.sa2_sh_2
			pop hl
			ld  a, (hl)
			add a, e
			ld  (hl), a
			inc hl
			ld  a, (hl)
			adc a, d
			ld  (hl), a
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// Enemies (ENABLE_CUSTOM_ENEMS). Types 1-4 (frames 8 + 2 (t - 1)) become
// 17-20 when their screen is first entered, out of MK1's way; 6 is the boss
// (frames 14-15): x1 its life, x2, y2 its home (sa2_level_enems).

// A level starts: bosses get 3 lives and remember where they start
void sa2_level_enems (void) {
	#asm
			ld  hl, _malotes
			ld  b, TOTAL_EXISTING_ENEMS
		.sa2_le_loop
			push hl
			ld  de, 8
			add hl, de
			ld  a, (hl) 			// t
			pop hl
			cp  6
			jr  nz, sa2_le_next
			ld  a, (hl) 			// x
			inc hl
			ld  c, (hl) 			// y
			inc hl
			ld  (hl), 3 			// x1: life
			inc hl
			inc hl
			ld  (hl), a 			// x2
			inc hl
			ld  (hl), c 			// y2
			dec hl
			dec hl
			dec hl
			dec hl
			dec hl
		.sa2_le_next
			ld  de, 9
			add hl, de
			djnz sa2_le_loop
	#endasm
}

void extra_enems_init (void) {
	#asm
			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  (hl), 0
			ld  a, (__en_t)
			or  a
			ret z
			cp  6
			jr  z, sa2_ei_boss
			cp  5
			jr  nc, sa2_ei_1
			add a, 16
			ld  (__en_t), a
		.sa2_ei_1
			// base frame 8 + 2 (t - 17)
			sub 17
			add a, a
			add a, 8
			jr  sa2_ei_base
		.sa2_ei_boss
			ld  a, (__en_x2)
			ld  (__en_x), a
			call Ashl16_HL
			ex  de, hl
			ld  hl, _en_an_x
			call sa2_en_w2
			ld  (hl), e
			inc hl
			ld  (hl), d
			ld  a, (__en_y2)
			ld  (__en_y), a
			call Ashl16_HL
			ex  de, hl
			ld  hl, _en_an_y
			call sa2_en_w2
			ld  (hl), e
			inc hl
			ld  (hl), d
			ld  hl, _en_an_vx
			call sa2_en_w2
			xor a
			ld  (hl), a
			inc hl
			ld  (hl), a
			ld  hl, _en_an_vy
			call sa2_en_w2
			xor a
			ld  (hl), a
			inc hl
			ld  (hl), a
			ld  a, 14
		.sa2_ei_base
			ld  c, a
			ld  hl, _en_an_base_frame
			call sa2_en_b2
			ld  (hl), c
			ret

		.sa2_en_b2
			ld  a, (_enit)
			jp  sa2_add_a
		.sa2_en_w2
			ld  a, (_enit)
			add a, a
			jp  sa2_add_a
	#endasm
}

// HL (16 bits) = limit (HL, -256, 256)
#asm
	.sa2_lim256
		ld  de, -256
		or  a
		sbc hl, de
		add hl, de
		jp  p, sa2_lim_1
		ex  de, hl
		ret
	.sa2_lim_1
		ld  de, 256
		or  a
		sbc hl, de
		add hl, de
		ret m
		ex  de, hl
		ret

	// (HL) += 8 or -8 so that it goes from (DE) towards BC; then +-256 max
	// HL -> en_an_v*, DE = en_an_* value, BC = player value
	.sa2_home
		push hl
		ld  h, b
		ld  l, c
		or  a
		sbc hl, de 				// player - enemy
		pop hl
		ret z
		ld  de, 8
		jp  p, sa2_ho_1
		ld  de, -8
	.sa2_ho_1
		push hl
		ld  a, (hl)
		inc hl
		ld  h, (hl)
		ld  l, a
		add hl, de
		call sa2_lim256
		ex  de, hl
		pop hl
		ld  (hl), e
		inc hl
		ld  (hl), d
		ret

	// (HL) += (DE); 0 .. BC; A = the result >> 6
	.sa2_pos
		push hl
		ld  a, (hl)
		inc hl
		ld  h, (hl)
		ld  l, a
		add hl, de
		bit 7, h
		jr  z, sa2_pos_1
		ld  hl, 0
		jr  sa2_pos_w
	.sa2_pos_1
		push hl
		or  a
		sbc hl, bc
		pop hl
		jr  c, sa2_pos_w
		ld  h, b
		ld  l, c
	.sa2_pos_w
		ex  de, hl
		pop hl
		ld  (hl), e
		inc hl
		ld  (hl), d
		ex  de, hl
		jp  HLshr6_A
#endasm

void extra_enems_move (void) {
	#asm
			ld  a, (__en_t)
			cp  6
			jr  nz, sa2_em_normal

			// The boss: towards the player while normal
			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  a, (hl)
			or  a
			jr  nz, sa2_em_boss_xy

			ld  hl, _en_an_x
			call sa2_en_w2
			ld  e, (hl)
			inc hl
			ld  d, (hl)
			ld  bc, (_player)
			ld  hl, _en_an_vx
			call sa2_en_w2
			call sa2_home
			ld  hl, _en_an_y
			call sa2_en_w2
			ld  e, (hl)
			inc hl
			ld  d, (hl)
			ld  bc, (_player + 2)
			ld  hl, _en_an_vy
			call sa2_en_w2
			call sa2_home

			ld  hl, _en_an_vx
			call sa2_en_w2
			ld  e, (hl)
			inc hl
			ld  d, (hl)
			ld  hl, _en_an_x
			call sa2_en_w2
			ld  bc, 224 * 64
			call sa2_pos
			ld  (__en_x), a
			ld  hl, _en_an_vy
			call sa2_en_w2
			ld  e, (hl)
			inc hl
			ld  d, (hl)
			ld  hl, _en_an_y
			call sa2_en_w2
			ld  bc, 144 * 64
			call sa2_pos
			ld  (__en_y), a
		.sa2_em_boss_xy
			jr  sa2_em_state

		.sa2_em_normal
			// Linear, not while stunned
			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  a, (hl)
			cp  SA2_EN_PARALYZED
			jr  z, sa2_em_bounce
			ld  a, (__en_mx)
			ld  c, a
			ld  a, (__en_x)
			add a, c
			ld  (__en_x), a
			ld  a, (__en_my)
			ld  c, a
			ld  a, (__en_y)
			add a, c
			ld  (__en_y), a
		.sa2_em_bounce
			ld  a, (__en_x)
			ld  c, a
			ld  a, (__en_x1)
			cp  c
			jr  z, sa2_em_bx
			ld  a, (__en_x2)
			cp  c
			jr  nz, sa2_em_by
		.sa2_em_bx
			ld  a, (__en_mx)
			neg
			ld  (__en_mx), a
		.sa2_em_by
			ld  a, (__en_y)
			ld  c, a
			ld  a, (__en_y1)
			cp  c
			jr  z, sa2_em_byy
			ld  a, (__en_y2)
			cp  c
			jr  nz, sa2_em_state
		.sa2_em_byy
			ld  a, (__en_my)
			neg
			ld  (__en_my), a

		.sa2_em_state
			// if (state != NORMAL && -- counter == 0) ...
			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  a, (hl)
			or  a
			jr  z, sa2_em_frame
			ld  b, a
			push hl
			ld  hl, _sa2_en_ct
			call sa2_en_b2
			dec (hl)
			pop hl
			jr  nz, sa2_em_frame
			ld  (hl), SA2_EN_NORMAL
			ld  a, b
			cp  SA2_EN_MUR
			jr  nz, sa2_em_frame
			// The boss is gone: the level is won
			xor a
			ld  (__en_t), a
			ld  (_sa2_state), a
			ld  hl, _player + 27 	// objs
			inc (hl)
			ld  hl, _en_an_next_frame
			call sa2_en_w2
			ld  de, _spr_empty
			ld  (hl), e
			inc hl
			ld  (hl), d
			ret

		.sa2_em_frame
			// stunned: still; flickering: off every other frame
			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  a, (hl)
			cp  SA2_EN_PARP
			jr  nz, sa2_em_f1
			ld  a, (_half_life)
			or  a
			jr  z, sa2_em_f1
			ld  de, _spr_empty
			jr  sa2_em_fw
		.sa2_em_f1
			ld  hl, _en_an_frame
			call sa2_en_b2
			ld  c, (hl)
			ld  hl, _en_an_base_frame
			call sa2_en_b2
			ld  a, (hl)
			add a, c
			add a, a
			ld  hl, _sprite_cells
			call sa2_add_a
			ld  e, (hl)
			inc hl
			ld  d, (hl)
		.sa2_em_fw
			ld  hl, _en_an_next_frame
			call sa2_en_w2
			ld  (hl), e
			inc hl
			ld  (hl), d
	#endasm
}

void extra_enems_checks (void) {
	#asm
			ld  a, (__en_t)
			or  a
			ret z

			// The sword, its hitting frames
			ld  a, (_sw_on)
			or  a
			jp  z, sa2_ec_player
			ld  a, (_sw_f)
			sub SA2_SW_HIT
			cp  SA2_SW_HIT_N
			jp  nc, sa2_ec_player
			// s_x + 7 >= ccx && s_x <= ccx + 15 (and y)
			ld  a, (__en_x)
			ld  b, a
			ld  a, (_sw_x)
			call sa2_overlap_7_15
			jp  nc, sa2_ec_player
			ld  a, (__en_y)
			ld  b, a
			ld  a, (_sw_y)
			call sa2_overlap_7_15
			jp  nc, sa2_ec_player

			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  a, (__en_t)
			cp  6
			jr  z, sa2_ec_boss
			// Stunned, turned away from the player (speed 1)
			ld  (hl), SA2_EN_PARALYZED
			ld  a, (__en_mx)
			or  a
			jr  z, sa2_ec_s1
			ld  a, (_gpx)
			ld  c, a
			ld  a, (__en_x)
			call sa2_sgn
			ld  (__en_mx), a
		.sa2_ec_s1
			ld  a, (__en_my)
			or  a
			jr  z, sa2_ec_s2
			ld  a, (_gpy)
			ld  c, a
			ld  a, (__en_y)
			call sa2_sgn
			ld  (__en_my), a
		.sa2_ec_s2
			jr  sa2_ec_count
		.sa2_ec_boss
			ld  a, (hl)
			or  a
			jr  nz, sa2_ec_sfx
			ld  a, (__en_x1)
			dec a
			ld  (__en_x1), a
			ld  (hl), SA2_EN_PARP
			jr  nz, sa2_ec_count
			// Killed
			ld  (hl), SA2_EN_MUR
			ld  a, SA2_ST_STOP
			ld  (_sa2_state), a
			ld  hl, _en_an_base_frame
			call sa2_en_b2
			ld  (hl), SA2_CELL_BOSS_DEAD
		.sa2_ec_count
			ld  hl, _sa2_en_ct
			call sa2_en_b2
			ld  (hl), SA2_COUNTER
		.sa2_ec_sfx
			ld  l, SA2_SFX_HIT
			call _peta_el_beeper

		.sa2_ec_player
			// Touching the player: frozen (not by a flickering boss)
			ld  a, (_sa2_state)
			ld  c, a
			ld  a, (_player + 23)
			or  c
			ret nz
			ld  hl, _sa2_en_st
			call sa2_en_b2
			ld  a, (hl)
			cp  SA2_EN_PARP
			ret z
			// p_px + 12 >= ccx && p_px <= ccx + 12 (and y)
			ld  a, (__en_x)
			ld  b, a
			ld  a, (_gpx)
			call sa2_overlap_12
			ret nc
			ld  a, (__en_y)
			ld  b, a
			ld  a, (_gpy)
			call sa2_overlap_12
			ret nc
			ld  l, SA2_SFX_FROZEN
			call _peta_el_beeper
			call sa2_hurt
			ld  a, SA2_ST_ICE
			ld  (_sa2_state), a
			ld  a, SA2_COUNTER
			ld  (_sa2_ice_ct), a
			ld  a, SA2_AX_ICE
			ld  (_sa2_ax), a
			ld  (_sa2_rx), a
			ret

			// C if A + 7 >= B && A <= B + 15
		.sa2_overlap_7_15
			ld  c, a
			add a, 7
			cp  b
			ccf
			ret nc
			ld  a, b
			add a, 15
			cp  c
			ccf
			ret
			// C if A + 12 >= B && A <= B + 12
		.sa2_overlap_12
			ld  c, a
			add a, 12
			cp  b
			ccf
			ret nc
			ld  a, b
			add a, 12
			cp  c
			ccf
			ret

			// A = A >= C ? 1 : -1 (sgni (A - C))
		.sa2_sgn
			cp  c
			ld  a, 1
			ret nc
			ld  a, -1
			ret
	#endasm
}

unsigned char this_enemy_kills (void) {
	return 0;
}

// ----------------------------------------------------------------------------
// The sword sprite (ENABLE_CUSTOM_SPRITES, 1 slot)

#define SA2_SW_SPR BASE_SPRITES + SP_CUSTOM_BASE * 16 		// (no brackets: ld ix, (nn) is a load)

void custom_sprites_create (void) {
	#asm
			jp  _custom_sprites_hide
	#endasm
}

void custom_sprites_hide (void) {
	#asm
			ld  hl, _spr_empty
			ld  (SA2_SW_SPR), hl
	#endasm
}

void custom_sprites_render (void) {
	#asm
			ld  a, (_sw_on)
			or  a
			jp  z, _custom_sprites_hide
			ld  ix, SA2_SW_SPR
			ld  a, (_sw_x)
			add a, VIEWPORT_X * 8
			srl a
			ld  (ix + 8), a
			ld  a, (_sw_y)
			add a, VIEWPORT_Y * 8
			ld  (ix + 9), a
			// frame 19 right, 20 left, 21 up
			ld  a, (_sw_type)
			add a, SA2_CELL_SWORD - SA2_SW_RIGHT
			add a, a
			ld  hl, _sprite_cells
			call sa2_add_a
			ld  a, (hl)
			inc hl
			ld  h, (hl)
			ld  l, a
			ld  (ix + 0), l
			ld  (ix + 1), h
	#endasm
}

// ----------------------------------------------------------------------------
// HUD: life (MK1, LIFE_X), coins (3 digits), keys, height (each screen)

#define SA2_COINS_X 	13
#define SA2_KEYS_X 		19
#define SA2_HEIGHT_X 	24
#define SA2_HUD_Y 		22

void sa2_hud (void) {
	#asm
			ld  a, (_sa2_o_coins)
			ld  c, a
			ld  a, (_sa2_coins)
			cp  c
			jr  z, sa2_hud_keys
			ld  (_sa2_o_coins), a
			// hundreds, then two digits
			ld  b, DIGIT_BASE - 1
		.sa2_hud_100
			inc b
			sub 100
			jr  nc, sa2_hud_100
			add a, 100
			push af
			ld  a, b
			ld  (__t), a
			ld  a, SA2_COINS_X
			ld  (__x), a
			ld  a, SA2_HUD_Y
			ld  (__y), a
			call _cd_put_char
			ld  a, SA2_COINS_X + 1
			ld  (__x), a
			pop af
			call draw_2_digits_shortcut
		.sa2_hud_keys
			ld  a, (_sa2_o_keys)
			ld  c, a
			ld  a, (_player + 28)
			cp  c
			ret z
			ld  (_sa2_o_keys), a
			ld  c, a
			ld  a, SA2_KEYS_X
			ld  (__x), a
			ld  a, SA2_HUD_Y
			ld  (__y), a
			ld  a, c
			jp  draw_2_digits_shortcut
	#endasm
}
