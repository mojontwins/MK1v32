// Nin Nin - the player's world: special tiles, the punch, breakable blocks,
// coins, cocos, respawning. Hand-written, from the NES game (engine/player.h,
// hitter.h, breakable.h, silly_cocos.h, my/hooks.h). Assembly; the C is in
// the comments.

// ----------------------------------------------------------------------------
// Persistence: broken blocks and coins taken stay so. One bit per tile, 19
// bytes per screen, NN_CLEAR (nn_load.h). Cleared at every level.

unsigned char nn_bit [] = { 1, 2, 4, 8, 16, 32, 64, 128 };
unsigned char *nn_clr;
unsigned char nn_m;

#asm
	// HL = NN_CLEAR + n_pant * 19
	.nn_scr_clr
		ld  a, (_n_pant)
		ld  l, a
		ld  h, 0
		ld  d, h
		ld  e, l
		add hl, hl
		ld  b, h
		ld  c, l
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, bc
		add hl, de
		ld  de, NN_CLEAR
		add hl, de
		ret

	// A = nn_bit [A & 7]
	.nn_bitof
		and 7
		ld  hl, _nn_bit
		call nn_add_a
		ld  a, (hl)
		ret
#endasm

void nn_clear_mark (void) {
	// tile nn_m of this screen is gone
	// nn_clr = (unsigned char *) (NN_CLEAR + n_pant * 19 + (nn_m >> 3));
	// *nn_clr |= nn_bit [nn_m & 7];
	#asm
			call nn_scr_clr
			ld  a, (_nn_m)
			rrca
			rrca
			rrca
			and 31
			call nn_add_a
			ld  (_nn_clr), hl
			ex  de, hl
			ld  a, (_nn_m)
			call nn_bitof
			ex  de, hl
			or  (hl)
			ld  (hl), a
	#endasm
}

void nn_clear_apply (void) {
	// redraw what is gone in this screen as the background
	// nn_clr = (unsigned char *) (NN_CLEAR + n_pant * 19);
	// rdx = rdy = 0;
	// for (nn_m = 0; nn_m < 150; nn_m ++) {
	//	if (nn_clr [nn_m >> 3] & nn_bit [nn_m & 7]) {
	//		_t = tileset_mappings [0];
	//		map_buff [nn_m] = _t;
	//		map_attr [nn_m] = comportamiento_tiles [_t];
	//		draw_coloured_tile (VIEWPORT_X + rdx, VIEWPORT_Y + rdy, _t);
	//	}
	//	rdx += 2; if (rdx == 30) { rdx = 0; rdy += 2; }
	// }
	#asm
			call nn_scr_clr
			ld  (_nn_clr), hl
			xor a
			ld  (_rdx), a
			ld  (_rdy), a
		.nn_ca_loop
			ld  (_nn_m), a
			call nn_bitof
			ld  c, a
			ld  a, (_nn_m)
			rrca
			rrca
			rrca
			and 31
			ld  hl, (_nn_clr)
			call nn_add_a
			ld  a, (hl)
			and c
			jr  z, nn_ca_next
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
			ld  (__t), a
			ld  e, a
			ld  hl, _map_buff
			ld  a, (_nn_m)
			call nn_add_a
			ld  (hl), e
			ld  hl, _comportamiento_tiles
			ld  a, e
			call nn_add_a
			ld  c, (hl)
			ld  hl, _map_attr
			ld  a, (_nn_m)
			call nn_add_a
			ld  (hl), c
			ld  a, (_rdx)
			add a, VIEWPORT_X
			ld  (__x), a
			ld  a, (_rdy)
			add a, VIEWPORT_Y
			ld  (__y), a
			call _draw_coloured_tile_do
		.nn_ca_next
			ld  a, (_rdx)
			add a, 2
			cp  30
			jr  nz, nn_ca_x
			ld  a, (_rdy)
			add a, 2
			ld  (_rdy), a
			xor a
		.nn_ca_x
			ld  (_rdx), a
			ld  a, (_nn_m)
			inc a
			cp  150
			jr  c, nn_ca_loop
			ld  (_nn_m), a
	#endasm
}

// A tile of this screen: (_x, _y) tile coordinates, _t. _x and _y are kept.
void nn_set_tile (void) {
	// nn_m = _x + (_y << 4) - _y;
	// map_buff [nn_m] = _t;
	// map_attr [nn_m] = comportamiento_tiles [_t];
	// draw_coloured_tile (VIEWPORT_X + (_x << 1), VIEWPORT_Y + (_y << 1), _t);
	// (draw_coloured_tile leaves its arguments in _x, _y: they are put back)
	#asm
			call nn_xy
			ld  (_nn_m), a
			ld  c, a
			ld  a, (__t)
			ld  e, a
			ld  hl, _map_buff
			ld  a, c
			call nn_add_a
			ld  (hl), e
			ld  hl, _comportamiento_tiles
			ld  a, e
			call nn_add_a
			ld  b, (hl)
			ld  hl, _map_attr
			ld  a, c
			call nn_add_a
			ld  (hl), b
			ld  a, (__x)
			push af
			add a, a
			add a, VIEWPORT_X
			ld  (__x), a
			ld  a, (__y)
			push af
			add a, a
			add a, VIEWPORT_Y
			ld  (__y), a
			call _draw_coloured_tile_do
			pop af
			ld  (__y), a
			pop af
			ld  (__x), a
	#endasm
}

// ----------------------------------------------------------------------------
// Breakable blocks (beh 24): the punch breaks them. They show tile 46 for a
// few frames, then they are background; a star box (14) leaves a coin (47)
// 6 times out of 8.

#define NN_BRK_MAX 			4
#define NN_BRK_FRAMES 		5		// NES 12
#define NN_TILE_BREAKING 	46
#define NN_TILE_COIN 		47
unsigned char brk_x [NN_BRK_MAX], brk_y [NN_BRK_MAX], brk_f [NN_BRK_MAX], brk_t [NN_BRK_MAX];
unsigned char nn_bi;

void nn_break (void) {
	// break the tile at (_x, _y)
	// for (nn_bi = 0; nn_bi < NN_BRK_MAX; nn_bi ++) if (brk_f [nn_bi] == 0) {
	//	brk_x [nn_bi] = _x; brk_y [nn_bi] = _y; brk_f [nn_bi] = 1;
	//	brk_t [nn_bi] = map_buff [_x + (_y << 4) - _y];
	//	_t = NN_TILE_BREAKING; nn_set_tile ();
	//	map_attr [nn_m] = 8;
	//	peta_el_beeper (5);
	//	return;
	// }
	#asm
			ld  hl, _brk_f
			ld  c, 0
		.nn_bk_find
			ld  a, (hl)
			or  a
			jr  z, nn_bk_found
			inc hl
			inc c
			ld  a, c
			cp  NN_BRK_MAX
			jr  c, nn_bk_find
			ld  (_nn_bi), a
			ret
		.nn_bk_found
			ld  (hl), 1
			ld  a, c
			ld  (_nn_bi), a
			ld  hl, _brk_x
			call nn_add_a
			ld  a, (__x)
			ld  (hl), a
			ld  hl, _brk_y
			ld  a, c
			call nn_add_a
			ld  a, (__y)
			ld  (hl), a
			call nn_xy
			ld  hl, _map_buff
			call nn_add_a
			ld  e, (hl)
			ld  hl, _brk_t
			ld  a, c
			call nn_add_a
			ld  (hl), e
			ld  a, NN_TILE_BREAKING
			ld  (__t), a
			call _nn_set_tile
			ld  a, (_nn_m)
			ld  hl, _map_attr
			call nn_add_a
			ld  (hl), 8
			ld  l, 5
			jp  _peta_el_beeper
	#endasm
}

void nn_breakables_do (void) {
	// for (nn_bi = 0; nn_bi < NN_BRK_MAX; nn_bi ++) if (brk_f [nn_bi]) {
	//	brk_f [nn_bi] ++;
	//	if (brk_f [nn_bi] == NN_BRK_FRAMES) {
	//		brk_f [nn_bi] = 0;
	//		_x = brk_x [nn_bi]; _y = brk_y [nn_bi];
	//		if (brk_t [nn_bi] == 14 && (rand () & 7) < 6) _t = NN_TILE_COIN;
	//		else _t = tileset_mappings [0];
	//		nn_set_tile ();
	//		if (_t != NN_TILE_COIN) nn_clear_mark ();
	//	}
	// }
	#asm
			xor a
		.nn_bd_loop
			ld  (_nn_bi), a
			ld  hl, _brk_f
			call nn_add_a
			ld  a, (hl)
			or  a
			jr  z, nn_bd_next
			inc a
			ld  (hl), a
			cp  NN_BRK_FRAMES
			jr  nz, nn_bd_next
			ld  (hl), 0
			ld  a, (_nn_bi)
			ld  c, a
			ld  hl, _brk_x
			call nn_add_a
			ld  a, (hl)
			ld  (__x), a
			ld  hl, _brk_y
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (__y), a
			ld  hl, _brk_t
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			cp  14
			jr  nz, nn_bd_bg
			call _rand
			ld  a, l
			and 7
			cp  6
			jr  nc, nn_bd_bg
			ld  a, NN_TILE_COIN
			jr  nn_bd_t
		.nn_bd_bg
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
		.nn_bd_t
			ld  (__t), a
			call _nn_set_tile
			ld  a, (__t)
			cp  NN_TILE_COIN
			call nz, _nn_clear_mark
		.nn_bd_next
			ld  a, (_nn_bi)
			inc a
			cp  NN_BRK_MAX
			jr  c, nn_bd_loop
			ld  (_nn_bi), a
	#endasm
}

// ----------------------------------------------------------------------------
// The punch (the NES "hitter"): FIRE. 5 frames (advancing every other one, as
// on the NES at half the rate), a fist in front of Nin Nin; frames 2-3 hit.

#define NN_HIT_FRAMES 		5
unsigned char nn_hit_offs [] = { 12, 14, 16, 16, 12 };
unsigned char hit_on, hit_f, hit_x, hit_y, hit_hx, hit_hy;

void nn_hitter_do (void) {
	// hit_y = gpy + 6; hit_hy = hit_y + 4;
	// if (player.facing == SIDEVIEW_FACING_LEFT) {
	//	hit_x = gpx + 8 - nn_hit_offs [hit_f]; hit_hx = hit_x;
	// } else {
	//	hit_x = gpx + nn_hit_offs [hit_f]; hit_hx = hit_x + 7; }
	// if (hit_f < 2 || hit_f > 3) hit_hy = 0;
	// if (hit_hy && hit_hx < 240) {
	//	_x = hit_hx >> 4; _y = hit_hy >> 4;
	//	if (_y < 10 && map_attr [_x + (_y << 4) - _y] == 24) { nn_break (); hit_on = 0; } }
	// hit_f ++; if (hit_f == NN_HIT_FRAMES) hit_on = 0;
	#asm
			ld  a, (_gpy)
			add a, 6
			ld  (_hit_y), a
			add a, 4
			ld  (_hit_hy), a
			ld  hl, _nn_hit_offs
			ld  a, (_hit_f)
			call nn_add_a
			ld  b, (hl)
			ld  a, (_player + 22) 		// facing
			cp  SIDEVIEW_FACING_LEFT
			jr  nz, nn_hd_right
			ld  a, (_gpx)
			add a, 8
			sub b
			ld  (_hit_x), a
			jr  nn_hd_hx
		.nn_hd_right
			ld  a, (_gpx)
			add a, b
			ld  (_hit_x), a
			add a, 7
		.nn_hd_hx
			ld  (_hit_hx), a
			ld  a, (_hit_f)
			cp  2
			jr  c, nn_hd_nohy
			cp  4
			jr  c, nn_hd_1
		.nn_hd_nohy
			xor a
			ld  (_hit_hy), a
		.nn_hd_1
			ld  a, (_hit_hy)
			or  a
			jr  z, nn_hd_2
			ld  a, (_hit_hx)
			cp  240
			jr  nc, nn_hd_2
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  (__x), a
			ld  a, (_hit_hy)
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  (__y), a
			cp  10
			jr  nc, nn_hd_2
			call nn_xy
			ld  hl, _map_attr
			call nn_add_a
			ld  a, (hl)
			cp  24
			jr  nz, nn_hd_2
			call _nn_break
			xor a
			ld  (_hit_on), a
		.nn_hd_2
			ld  a, (_hit_f)
			inc a
			ld  (_hit_f), a
			cp  NN_HIT_FRAMES
			ret nz
			xor a
			ld  (_hit_on), a
	#endasm
}

// ----------------------------------------------------------------------------
// Cocos: 3, 8 directions (0 up, clockwise)

#define NN_COCOS 			3
#define NN_COCO_V 			8		// NES 4 px / frame at 60 Hz
#define NN_COCO_D 			6
signed char nn_coco_mx [] = { 0, NN_COCO_D, NN_COCO_V, NN_COCO_D, 0, -NN_COCO_D, -NN_COCO_V, -NN_COCO_D };
signed char nn_coco_my [] = { -NN_COCO_V, -NN_COCO_D, 0, NN_COCO_D, NN_COCO_V, NN_COCO_D, 0, -NN_COCO_D };
unsigned char coco_on [NN_COCOS], coco_x [NN_COCOS], coco_y [NN_COCOS];
signed char coco_mx [NN_COCOS], coco_my [NN_COCOS];
unsigned char nn_ki;

void nn_hurt (void) {
	// if (player.estado == EST_NORMAL) {
	//	player.is_dead = PLAYER_KILLED_BY_ENEM; player.drain_amount = 1;
	//	peta_el_beeper (2); }
	#asm
			ld  a, (_player + 23) 		// estado
			or  a
			ret nz
			ld  a, PLAYER_KILLED_BY_ENEM
			ld  (_player + 36), a 		// is_dead
			ld  a, 1
			ld  (_player + 46), a 		// drain_amount
			ld  l, 2
			jp  _peta_el_beeper
	#endasm
}

void cocos_add (void) {
	// from (_x, _y) (pixels, the coco's top left) in direction _t
	// for (nn_ki = 0; nn_ki < NN_COCOS; nn_ki ++) if (coco_on [nn_ki] == 0) {
	//	coco_on [nn_ki] = 1;
	//	coco_x [nn_ki] = _x; coco_y [nn_ki] = _y;
	//	coco_mx [nn_ki] = nn_coco_mx [_t]; coco_my [nn_ki] = nn_coco_my [_t];
	//	peta_el_beeper (4);
	//	return;
	// }
	#asm
			ld  hl, _coco_on
			ld  c, 0
		.nn_ka_find
			ld  a, (hl)
			or  a
			jr  z, nn_ka_found
			inc hl
			inc c
			ld  a, c
			cp  NN_COCOS
			jr  c, nn_ka_find
			ld  (_nn_ki), a
			ret
		.nn_ka_found
			ld  (hl), 1
			ld  a, c
			ld  (_nn_ki), a
			ld  hl, _coco_x
			call nn_add_a
			ld  a, (__x)
			ld  (hl), a
			ld  hl, _coco_y
			ld  a, c
			call nn_add_a
			ld  a, (__y)
			ld  (hl), a
			ld  hl, _nn_coco_mx
			ld  a, (__t)
			call nn_add_a
			ld  b, (hl)
			ld  hl, _coco_mx
			ld  a, c
			call nn_add_a
			ld  (hl), b
			ld  hl, _nn_coco_my
			ld  a, (__t)
			call nn_add_a
			ld  b, (hl)
			ld  hl, _coco_my
			ld  a, c
			call nn_add_a
			ld  (hl), b
			ld  l, 4
			jp  _peta_el_beeper
	#endasm
}

void cocos_do (void) {
	// for (nn_ki = 0; nn_ki < NN_COCOS; nn_ki ++) if (coco_on [nn_ki]) {
	//	rda = coco_x [nn_ki] + coco_mx [nn_ki];
	//	rdb = coco_y [nn_ki] + coco_my [nn_ki];
	//	coco_x [nn_ki] = rda; coco_y [nn_ki] = rdb;
	//	// out of the room (unsigned: negative is big)
	//	if (rda > 234 || rdb > 154) { coco_on [nn_ki] = 0; continue; }
	//	// a wall
	//	_x = (rda + 3) >> 4; _y = (rdb + 3) >> 4;
	//	if (map_attr [_x + (_y << 4) - _y] & 8) { coco_on [nn_ki] = 0; continue; }
	//	// Nin Nin
	//	if (rda + 5 >= gpx + 2 && rda <= gpx + 13 && rdb + 5 >= gpy + 2 && rdb <= gpy + 15) {
	//		coco_on [nn_ki] = 0; nn_hurt (); }
	// }
	#asm
			xor a
		.nn_kd_loop
			ld  (_nn_ki), a
			ld  c, a
			ld  hl, _coco_on
			call nn_add_a
			ld  a, (hl)
			or  a
			jp  z, nn_kd_next
			ld  hl, _coco_mx
			ld  a, c
			call nn_add_a
			ld  b, (hl)
			ld  hl, _coco_x
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			add a, b
			ld  (hl), a
			ld  (_rda), a
			ld  hl, _coco_my
			ld  a, c
			call nn_add_a
			ld  b, (hl)
			ld  hl, _coco_y
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			add a, b
			ld  (hl), a
			ld  (_rdb), a
			cp  155
			jr  nc, nn_kd_off
			ld  a, (_rda)
			cp  235
			jr  nc, nn_kd_off
			add a, 3
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  (__x), a
			ld  a, (_rdb)
			add a, 3
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  (__y), a
			call nn_xy
			ld  hl, _map_attr
			call nn_add_a
			ld  a, (hl)
			and 8
			jr  nz, nn_kd_off
			// Nin Nin: rda + 3 >= gpx, gpx + 13 >= rda, rdb + 3 >= gpy, gpy + 15 >= rdb
			ld  a, (_gpx)
			ld  b, a
			ld  a, (_rda)
			ld  e, a
			add a, 3
			cp  b
			jr  c, nn_kd_next
			ld  a, b
			add a, 13
			jr  c, nn_kd_c1
			cp  e
			jr  c, nn_kd_next
		.nn_kd_c1
			ld  a, (_gpy)
			ld  b, a
			ld  a, (_rdb)
			ld  e, a
			add a, 3
			cp  b
			jr  c, nn_kd_next
			ld  a, b
			add a, 15
			jr  c, nn_kd_c2
			cp  e
			jr  c, nn_kd_next
		.nn_kd_c2
			call nn_kd_kill
			call _nn_hurt
			jr  nn_kd_next
		.nn_kd_off
			call nn_kd_kill
		.nn_kd_next
			ld  a, (_nn_ki)
			inc a
			cp  NN_COCOS
			jp  c, nn_kd_loop
			ld  (_nn_ki), a
			ret
		.nn_kd_kill
			ld  hl, _coco_on
			ld  a, (_nn_ki)
			call nn_add_a
			ld  (hl), 0
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// The floor: quicksand (beh 2), ice (beh & 64), conveyors (40 / 41), coins
// (128, at Nin Nin's centre). Run every frame before the player moves.

unsigned char nn_sand, nn_ice, nn_at1, nn_at2;
#define NN_VX_CONVEYOR 		154		// NES 16
#define NN_VY_SINKING 		10		// NES 1

#asm
	// nn_at1 = map_attr [_x + 15 * _y]; nn_at2 = map_attr [_x2 + 15 * _y]
	.nn_at12
		call nn_y15
		ld  e, a
		ld  a, (__x)
		add a, e
		ld  hl, _map_attr
		call nn_add_a
		ld  a, (hl)
		ld  (_nn_at1), a
		ld  a, (__x2)
		add a, e
		ld  hl, _map_attr
		call nn_add_a
		ld  a, (hl)
		ld  (_nn_at2), a
		ret
#endasm

void nn_floor (void) {
	// nn_sand = nn_ice = 0;
	// _x = (gpx + 4) >> 4; _x2 = (gpx + 11) >> 4;
	// // quicksand: at the feet
	// _y = (gpy + 15) >> 4;
	// if (_y < 10) {
	//	nn_at1 = map_attr [_x + (_y << 4) - _y]; nn_at2 = map_attr [_x2 + (_y << 4) - _y];
	//	if (nn_at1 == 2 || nn_at2 == 2) nn_sand = 1; }
	// // what's under the feet
	// _y = (gpy + 16) >> 4;
	// if (_y < 10 && player.possee) {
	//	nn_at1 = ...; nn_at2 = ...;
	//	nn_ice = (nn_at1 & 64) || (nn_at2 & 64);
	//	if ((nn_at1 & 0xFE) == 40) ptgmx += (nn_at1 & 1) ? NN_VX_CONVEYOR : -NN_VX_CONVEYOR;
	//	else if ((nn_at2 & 0xFE) == 40) ptgmx += (nn_at2 & 1) ? NN_VX_CONVEYOR : -NN_VX_CONVEYOR; }
	// // coins
	// _x = (gpx + 8) >> 4; _y = (gpy + 8) >> 4;
	// nn_m = _x + (_y << 4) - _y;
	// if (_y < 10 && map_attr [nn_m] == 128) {
	//	_t = tileset_mappings [0]; nn_set_tile (); nn_clear_mark ();
	//	peta_el_beeper (1);
	//	nn_coins ++;
	//	if (nn_coins == NN_COINS_LIFE) { nn_coins = 0; player.life ++; } }
	#asm
			xor a
			ld  (_nn_sand), a
			ld  (_nn_ice), a
			ld  c, 4
			ld  a, (_gpx)
			call nn_cell
			ld  (__x), a
			ld  c, 11
			ld  a, (_gpx)
			call nn_cell
			ld  (__x2), a

			ld  c, 15
			ld  a, (_gpy)
			call nn_cell
			ld  (__y), a
			cp  10
			jr  nc, nn_fl_2
			call nn_at12
			ld  a, (_nn_at1)
			cp  2
			jr  z, nn_fl_sand
			ld  a, (_nn_at2)
			cp  2
			jr  nz, nn_fl_2
		.nn_fl_sand
			ld  a, 1
			ld  (_nn_sand), a

		.nn_fl_2
			ld  c, 16
			ld  a, (_gpy)
			call nn_cell
			ld  (__y), a
			cp  10
			jr  nc, nn_fl_3
			ld  a, (_player + 26) 		// possee
			or  a
			jr  z, nn_fl_3
			call nn_at12
			ld  a, (_nn_at1)
			ld  b, a
			ld  a, (_nn_at2)
			or  b
			and 64
			jr  z, nn_fl_ice
			ld  a, 1
		.nn_fl_ice
			ld  (_nn_ice), a
			ld  a, (_nn_at1)
			ld  b, a
			and 0xfe
			cp  40
			jr  z, nn_fl_conv
			ld  a, (_nn_at2)
			ld  b, a
			and 0xfe
			cp  40
			jr  nz, nn_fl_3
		.nn_fl_conv
			ld  hl, (_ptgmx)
			ld  de, NN_VX_CONVEYOR
			bit 0, b
			jr  nz, nn_fl_cv
			ld  de, -NN_VX_CONVEYOR
		.nn_fl_cv
			add hl, de
			ld  (_ptgmx), hl

		.nn_fl_3
			ld  c, 8
			ld  a, (_gpx)
			call nn_cell
			ld  (__x), a
			ld  a, (_gpy)
			call nn_cell
			ld  (__y), a
			call nn_xy
			ld  (_nn_m), a
			ld  a, (__y)
			cp  10
			ret nc
			ld  a, (_nn_m)
			ld  hl, _map_attr
			call nn_add_a
			ld  a, (hl)
			cp  128
			ret nz
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
			ld  (__t), a
			call _nn_set_tile
			call _nn_clear_mark
			ld  l, 1
			call _peta_el_beeper
			ld  a, (_nn_coins)
			inc a
			cp  NN_COINS_LIFE
			jr  nz, nn_fl_coins
			ld  hl, (_player + 29) 		// life
			inc hl
			ld  (_player + 29), hl
			xor a
		.nn_fl_coins
			ld  (_nn_coins), a
	#endasm
}

// ----------------------------------------------------------------------------
// Horizontal movement (PLAYER_CUSTOM_HENG): the engine's, with the NES
// values for ice and quicksand.

signed int nn_ax, nn_rx, nn_vxmax;

void player_custom_heng (void) {
	// if (nn_sand) { nn_ax = 23; nn_rx = 184; nn_vxmax = 77; }		// NES 1, 8, 8
	// else if (nn_ice) { nn_ax = 23; nn_rx = 11; nn_vxmax = 461; }	// NES 1, 1/2, 48
	// else { nn_ax = PLAYER_AX; nn_rx = PLAYER_RX; nn_vxmax = PLAYER_MAX_VX; }
	// if ((pad0 & sp_LEFT) == 0) {
	//	player.vx -= nn_ax;
	//	if (player.vx < -nn_vxmax) player.vx = -nn_vxmax;
	//	player.facing = SIDEVIEW_FACING_LEFT; thrusting = 1;
	// } else if ((pad0 & sp_RIGHT) == 0) {
	//	player.vx += nn_ax;
	//	if (player.vx > nn_vxmax) player.vx = nn_vxmax;
	//	player.facing = SIDEVIEW_FACING_RIGHT; thrusting = 1;
	// } else {
	//	thrusting = 0;
	//	if (player.vx > 0) { player.vx -= nn_rx; if (player.vx < 0) player.vx = 0; }
	//	else if (player.vx < 0) { player.vx += nn_rx; if (player.vx > 0) player.vx = 0; } }
	#asm
			ld  a, (_nn_sand)
			or  a
			jr  z, nn_hg_1
			ld  hl, 23
			ld  de, 184
			ld  bc, 77
			jr  nn_hg_3
		.nn_hg_1
			ld  a, (_nn_ice)
			or  a
			jr  z, nn_hg_2
			ld  hl, 23
			ld  de, 11
			ld  bc, 461
			jr  nn_hg_3
		.nn_hg_2
			ld  hl, PLAYER_AX
			ld  de, PLAYER_RX
			ld  bc, PLAYER_MAX_VX
		.nn_hg_3
			ld  (_nn_ax), hl
			ld  (_nn_rx), de
			ld  (_nn_vxmax), bc

			ld  a, (_pad0)
			and sp_LEFT
			jr  nz, nn_hg_nl
			ld  hl, (_player + 6) 		// vx
			ld  de, (_nn_ax)
			or  a
			sbc hl, de
			ld  de, (_nn_vxmax)
			push hl
			add hl, de
			bit 7, h
			pop hl
			jr  z, nn_hg_l1
			ex  de, hl
			call nn_neg_hl
		.nn_hg_l1
			ld  (_player + 6), hl
			ld  a, SIDEVIEW_FACING_LEFT
			jr  nn_hg_thr
		.nn_hg_nl
			ld  a, (_pad0)
			and sp_RIGHT
			jr  nz, nn_hg_none
			ld  hl, (_player + 6)
			ld  de, (_nn_ax)
			add hl, de
			push hl
			ex  de, hl
			ld  hl, (_nn_vxmax)
			or  a
			sbc hl, de
			ld  a, h
			pop hl
			rlca
			jr  nc, nn_hg_r1
			ld  hl, (_nn_vxmax)
		.nn_hg_r1
			ld  (_player + 6), hl
			ld  a, SIDEVIEW_FACING_RIGHT
		.nn_hg_thr
			ld  (_player + 22), a 		// facing
			ld  a, 1
			ld  (_thrusting), a
			ret
		.nn_hg_none
			xor a
			ld  (_thrusting), a
			ld  hl, (_player + 6)
			ld  a, h
			or  l
			ret z
			ld  de, (_nn_rx)
			bit 7, h
			jr  nz, nn_hg_neg
			or  a
			sbc hl, de
			bit 7, h
			jr  z, nn_hg_st
			ld  hl, 0
			jr  nn_hg_st
		.nn_hg_neg
			add hl, de
			bit 7, h
			jr  nz, nn_hg_st
			ld  hl, 0
		.nn_hg_st
			ld  (_player + 6), hl
	#endasm
}

// ----------------------------------------------------------------------------
// Dying and respawning (the NES DIE_AND_RESPAWN): a killing tile takes a life
// and puts Nin Nin back on the last safe place: where he last stood on solid
// ground, or where he came into the screen.

signed int nn_safe_x, nn_safe_y;

void nn_set_safe (void) {
	// nn_safe_x = player.x; nn_safe_y = player.y;
	#asm
			ld  hl, (_player)
			ld  (_nn_safe_x), hl
			ld  hl, (_player + 2)
			ld  (_nn_safe_y), hl
	#endasm
}

unsigned char custom_bg_hit (void) {
	// if (hit_v == 0 && hit_h == 0) return 0;
	// player.x = nn_safe_x; player.y = nn_safe_y;
	// player.vx = player.vy = 0;
	// gpx = player.x >> 6; gpy = player.y >> 6;
	// return (player.estado == EST_NORMAL);
	#asm
			ld  hl, 0
			ld  a, (_hit_v)
			ld  b, a
			ld  a, (_hit_h)
			or  b
			ret z
			ld  (_player + 6), hl
			ld  (_player + 8), hl
			ld  hl, (_nn_safe_x)
			ld  (_player), hl
			call nn_bh_shr6
			ld  (_gpx), a
			ld  hl, (_nn_safe_y)
			ld  (_player + 2), hl
			call nn_bh_shr6
			ld  (_gpy), a
			ld  hl, 0
			ld  a, (_player + 23) 		// estado
			or  a
			ret nz
			inc l
			ret
		.nn_bh_shr6
			ld  b, 6
		.nn_bh_sl
			sra h
			rr  l
			djnz nn_bh_sl
			ld  a, l
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// Puzzles (the NES press_fire_at_screen), FIRE while standing on:
//  * a ray (27): it goes off (46); when every ray of the screen is off, the
//    fences (10) open
//  * a face (36): the faces must be pressed in the order of the chains (43)
//    hanging over them (none, 1, 2...); when all are, the spikes (12) go
// They start again when the screen is entered again. Springs: a tile with
// behaviour 48 (set by nn_castle.h) under Nin Nin's feet turns into spikes.

#define NN_TILE_RAY 		27
#define NN_TILE_RAY_OFF 	46
#define NN_TILE_FENCE 		10
#define NN_TILE_FACE 		36
#define NN_TILE_CHAIN 		43
#define NN_TILE_SPIKES 		12
unsigned char nn_face_count, nn_face_cleared, nn_fi, nn_ft;

void nn_clear_tiles (void) {
	// every tile rda of the screen to the background
	// _x = _y = 0;
	// for (nn_fi = 0; nn_fi < 150; nn_fi ++) {
	//	if (map_buff [nn_fi] == rda) { _t = tileset_mappings [0]; nn_set_tile (); }
	//	_x ++; if (_x == 15) { _x = 0; _y ++; }
	// }
	#asm
			xor a
			ld  (__x), a
			ld  (__y), a
		.nn_ct_loop
			ld  (_nn_fi), a
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (_rda)
			cp  (hl)
			jr  nz, nn_ct_next
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
			ld  (__t), a
			call _nn_set_tile
		.nn_ct_next
			ld  a, (__x)
			inc a
			cp  15
			jr  nz, nn_ct_x
			ld  a, (__y)
			inc a
			ld  (__y), a
			xor a
		.nn_ct_x
			ld  (__x), a
			ld  a, (_nn_fi)
			inc a
			cp  150
			jr  c, nn_ct_loop
			ld  (_nn_fi), a
	#endasm
}

unsigned char nn_fire_at_screen (void) {
	// 1 if FIRE did something here (then Nin Nin doesn't punch)
	// if (nn_castle_fire ()) return 1; 		// (NN_CASTLE)
	// _x = (gpx + 8) >> 4; _y = (gpy + 16) >> 4;
	// if (_y > 9) return 0;
	// nn_m = _x + (_y << 4) - _y;
	// nn_ft = map_buff [nn_m];
	// if (nn_ft == NN_TILE_RAY) {
	//	_t = NN_TILE_RAY_OFF; nn_set_tile ();
	//	peta_el_beeper (6);
	//	nn_rays --;
	//	if (nn_rays == 0) {
	//		peta_el_beeper (8);
	//		if (nn_castle_rays ()) return 1; 	// (NN_CASTLE)
	//		rda = NN_TILE_FENCE; nn_clear_tiles (); }
	//	return 1;
	// }
	// if (nn_ft == NN_TILE_FACE) {
	//	// chains over it
	//	rda = 0;
	//	while (nn_m >= 15) {
	//		nn_m -= 15;
	//		if (map_buff [nn_m] != NN_TILE_CHAIN) break;
	//		rda ++; }
	//	if (rda == 0) {
	//		nn_face_count = 0; nn_face_cleared = 1; peta_el_beeper (6);
	//	} else if (rda == nn_face_count + 1) {
	//		nn_face_count = rda; nn_face_cleared ++;
	//		if (nn_face_cleared == nn_faces) {
	//			peta_el_beeper (8); rda = NN_TILE_SPIKES; nn_clear_tiles ();
	//		} else peta_el_beeper (6);
	//	} else {
	//		nn_face_count = nn_face_cleared = 0; peta_el_beeper (2); }
	//	return 1;
	// }
	// return 0;
	#asm
		#ifdef NN_CASTLE
			call _nn_castle_fire
			ld  a, l
			or  a
			ret nz
		#endif
			ld  c, 8
			ld  a, (_gpx)
			call nn_cell
			ld  (__x), a
			ld  c, 16
			ld  a, (_gpy)
			call nn_cell
			ld  (__y), a
			ld  hl, 0
			cp  10
			ret nc
			call nn_xy
			ld  (_nn_m), a
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (hl)
			ld  (_nn_ft), a
			cp  NN_TILE_RAY
			jr  nz, nn_fs_face

			ld  a, NN_TILE_RAY_OFF
			ld  (__t), a
			call _nn_set_tile
			ld  l, 6
			call _peta_el_beeper
			ld  hl, _nn_rays
			dec (hl)
			jr  nz, nn_fs_ret1
			ld  l, 8
			call _peta_el_beeper
		#ifdef NN_CASTLE
			call _nn_castle_rays
			ld  a, l
			or  a
			jr  nz, nn_fs_ret1
		#endif
			ld  a, NN_TILE_FENCE
			ld  (_rda), a
			call _nn_clear_tiles
		.nn_fs_ret1
			ld  hl, 1
			ret

		.nn_fs_face
			cp  NN_TILE_FACE
			ld  hl, 0
			ret nz
			ld  a, (_nn_m)
			ld  e, a
			ld  c, 0
		.nn_fs_chain
			ld  a, e
			cp  15
			jr  c, nn_fs_chd
			sub 15
			ld  e, a
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (hl)
			cp  NN_TILE_CHAIN
			jr  nz, nn_fs_chd
			inc c
			jr  nn_fs_chain
		.nn_fs_chd
			ld  a, e
			ld  (_nn_m), a
			ld  a, c
			ld  (_rda), a
			or  a
			jr  nz, nn_fs_f1
			ld  (_nn_face_count), a
			inc a
			ld  (_nn_face_cleared), a
			ld  l, 6
			jr  nn_fs_beep
		.nn_fs_f1
			ld  a, (_nn_face_count)
			inc a
			cp  c
			jr  nz, nn_fs_wrong
			ld  a, c
			ld  (_nn_face_count), a
			ld  hl, _nn_face_cleared
			inc (hl)
			ld  a, (_nn_faces)
			cp  (hl)
			ld  l, 6
			jr  nz, nn_fs_beep
			ld  l, 8
			call _peta_el_beeper
			ld  a, NN_TILE_SPIKES
			ld  (_rda), a
			call _nn_clear_tiles
			jr  nn_fs_ret1
		.nn_fs_wrong
			xor a
			ld  (_nn_face_count), a
			ld  (_nn_face_cleared), a
			ld  l, 2
		.nn_fs_beep
			call _peta_el_beeper
			jr  nn_fs_ret1
	#endasm
}

void nn_springs (void) {
	// behaviour 48 at the feet: spikes there
	// _y = (gpy + 15) >> 4;
	// if (_y > 9) return;
	// _x = (gpx + 4) >> 4;
	// if (map_attr [_x + (_y << 4) - _y] != 48) {
	//	_x = (gpx + 11) >> 4;
	//	if (map_attr [_x + (_y << 4) - _y] != 48) return; }
	// _t = NN_TILE_SPIKES; nn_set_tile ();
	// peta_el_beeper (7);
	#asm
			ld  c, 15
			ld  a, (_gpy)
			call nn_cell
			ld  (__y), a
			cp  10
			ret nc
			ld  c, 4
			call nn_sp_at
			jr  z, nn_sp_yes
			ld  c, 11
			call nn_sp_at
			ret nz
		.nn_sp_yes
			ld  a, NN_TILE_SPIKES
			ld  (__t), a
			call _nn_set_tile
			ld  l, 7
			jp  _peta_el_beeper
			// _x = (gpx + C) >> 4; Z if map_attr [_x + 15 * _y] == 48
		.nn_sp_at
			ld  a, (_gpx)
			call nn_cell
			ld  (__x), a
			call nn_xy
			ld  hl, _map_attr
			call nn_add_a
			ld  a, (hl)
			cp  48
			ret
	#endasm
}
