// Nin Nin - the player's world: special tiles, the punch, breakable blocks,
// coins, cocos, respawning. Hand-written, from the NES game (engine/player.h,
// hitter.h, breakable.h, silly_cocos.h, my/hooks.h).

// ----------------------------------------------------------------------------
// Persistence: broken blocks and coins taken stay so. One bit per tile, 19
// bytes per screen, NN_CLEAR (nn_load.h). Cleared at every level.

unsigned char nn_bit [] = { 1, 2, 4, 8, 16, 32, 64, 128 };
unsigned char *nn_clr;
unsigned char nn_m;

void nn_clear_mark (void) {
	// tile nn_m of this screen is gone
	nn_clr = (unsigned char *) (NN_CLEAR + n_pant * 19 + (nn_m >> 3));
	*nn_clr |= nn_bit [nn_m & 7];
}

void nn_clear_apply (void) {
	// redraw what is gone in this screen as the background
	nn_clr = (unsigned char *) (NN_CLEAR + n_pant * 19);
	rdx = rdy = 0;
	for (nn_m = 0; nn_m < 150; nn_m ++) {
		if (nn_clr [nn_m >> 3] & nn_bit [nn_m & 7]) {
			_t = tileset_mappings [0];
			map_buff [nn_m] = _t;
			map_attr [nn_m] = comportamiento_tiles [_t];
			draw_coloured_tile (VIEWPORT_X + rdx, VIEWPORT_Y + rdy, _t);
		}
		rdx += 2; if (rdx == 30) { rdx = 0; rdy += 2; }
	}
}

// A tile of this screen: (_x, _y) tile coordinates, _t
void nn_set_tile (void) {
	nn_m = _x + (_y << 4) - _y;
	map_buff [nn_m] = _t;
	map_attr [nn_m] = comportamiento_tiles [_t];
	draw_coloured_tile (VIEWPORT_X + (_x << 1), VIEWPORT_Y + (_y << 1), _t);
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
	for (nn_bi = 0; nn_bi < NN_BRK_MAX; nn_bi ++) if (brk_f [nn_bi] == 0) {
		brk_x [nn_bi] = _x; brk_y [nn_bi] = _y; brk_f [nn_bi] = 1;
		brk_t [nn_bi] = map_buff [_x + (_y << 4) - _y];
		_t = NN_TILE_BREAKING; nn_set_tile ();
		map_attr [nn_m] = 8;
		peta_el_beeper (5);
		return;
	}
}

void nn_breakables_do (void) {
	for (nn_bi = 0; nn_bi < NN_BRK_MAX; nn_bi ++) if (brk_f [nn_bi]) {
		brk_f [nn_bi] ++;
		if (brk_f [nn_bi] == NN_BRK_FRAMES) {
			brk_f [nn_bi] = 0;
			_x = brk_x [nn_bi]; _y = brk_y [nn_bi];
			if (brk_t [nn_bi] == 14 && (rand () & 7) < 6) _t = NN_TILE_COIN;
			else _t = tileset_mappings [0];
			nn_set_tile ();
			if (_t != NN_TILE_COIN) nn_clear_mark ();
		}
	}
}

// ----------------------------------------------------------------------------
// The punch (the NES "hitter"): FIRE. 5 frames (advancing every other one, as
// on the NES at half the rate), a fist in front of Nin Nin; frames 2-3 hit.

#define NN_HIT_FRAMES 		5
unsigned char nn_hit_offs [] = { 12, 14, 16, 16, 12 };
unsigned char hit_on, hit_f, hit_x, hit_y, hit_hx, hit_hy;

void nn_hitter_do (void) {
	hit_y = gpy + 6; hit_hy = hit_y + 4;
	if (player.facing == SIDEVIEW_FACING_LEFT) {
		hit_x = gpx + 8 - nn_hit_offs [hit_f];
		hit_hx = hit_x;
	} else {
		hit_x = gpx + nn_hit_offs [hit_f];
		hit_hx = hit_x + 7;
	}
	if (hit_f < 2 || hit_f > 3) hit_hy = 0;

	if (hit_hy && hit_hx < 240) {
		_x = hit_hx >> 4; _y = hit_hy >> 4;
		if (_y < 10 && map_attr [_x + (_y << 4) - _y] == 24) {
			nn_break ();
			hit_on = 0;
		}
	}
	hit_f ++; if (hit_f == NN_HIT_FRAMES) hit_on = 0;
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
	if (player.estado == EST_NORMAL) {
		player.is_dead = PLAYER_KILLED_BY_ENEM;
		player.drain_amount = 1;
		peta_el_beeper (2);
	}
}

void cocos_add (void) {
	// from (_x, _y) (pixels, the coco's top left) in direction _t
	for (nn_ki = 0; nn_ki < NN_COCOS; nn_ki ++) if (coco_on [nn_ki] == 0) {
		coco_on [nn_ki] = 1;
		coco_x [nn_ki] = _x; coco_y [nn_ki] = _y;
		coco_mx [nn_ki] = nn_coco_mx [_t]; coco_my [nn_ki] = nn_coco_my [_t];
		peta_el_beeper (4);
		return;
	}
}

void cocos_do (void) {
	for (nn_ki = 0; nn_ki < NN_COCOS; nn_ki ++) if (coco_on [nn_ki]) {
		rda = coco_x [nn_ki] + coco_mx [nn_ki];
		rdb = coco_y [nn_ki] + coco_my [nn_ki];
		coco_x [nn_ki] = rda; coco_y [nn_ki] = rdb;
		// out of the room (unsigned: negative is big)
		if (rda > 234 || rdb > 154) { coco_on [nn_ki] = 0; continue; }
		// a wall
		_x = (rda + 3) >> 4; _y = (rdb + 3) >> 4;
		if (map_attr [_x + (_y << 4) - _y] & 8) { coco_on [nn_ki] = 0; continue; }
		// Nin Nin
		if (rda + 5 >= gpx + 2 && rda <= gpx + 13 && rdb + 5 >= gpy + 2 && rdb <= gpy + 15) {
			coco_on [nn_ki] = 0;
			nn_hurt ();
		}
	}
}

// ----------------------------------------------------------------------------
// The floor: quicksand (beh 2), ice (beh & 64), conveyors (40 / 41), coins
// (128, at Nin Nin's centre). Run every frame before the player moves.

unsigned char nn_sand, nn_ice, nn_at1, nn_at2;
#define NN_VX_CONVEYOR 		154		// NES 16
#define NN_VY_SINKING 		10		// NES 1

void nn_floor (void) {
	nn_sand = nn_ice = 0;
	_x = (gpx + 4) >> 4; _x2 = (gpx + 11) >> 4;

	// quicksand: at the feet
	_y = (gpy + 15) >> 4;
	if (_y < 10) {
		nn_at1 = map_attr [_x + (_y << 4) - _y]; nn_at2 = map_attr [_x2 + (_y << 4) - _y];
		if (nn_at1 == 2 || nn_at2 == 2) nn_sand = 1;
	}

	// what's under the feet
	_y = (gpy + 16) >> 4;
	if (_y < 10 && player.possee) {
		nn_at1 = map_attr [_x + (_y << 4) - _y]; nn_at2 = map_attr [_x2 + (_y << 4) - _y];
		nn_ice = (nn_at1 & 64) || (nn_at2 & 64);
		if ((nn_at1 & 0xFE) == 40) ptgmx += (nn_at1 & 1) ? NN_VX_CONVEYOR : -NN_VX_CONVEYOR;
		else if ((nn_at2 & 0xFE) == 40) ptgmx += (nn_at2 & 1) ? NN_VX_CONVEYOR : -NN_VX_CONVEYOR;
	}

	// coins
	_x = (gpx + 8) >> 4; _y = (gpy + 8) >> 4;
	nn_m = _x + (_y << 4) - _y;
	if (_y < 10 && map_attr [nn_m] == 128) {
		_t = tileset_mappings [0]; nn_set_tile ();
		nn_clear_mark ();
		peta_el_beeper (1);
		nn_coins ++;
		if (nn_coins == NN_COINS_LIFE) { nn_coins = 0; player.life ++; }
	}
}

// ----------------------------------------------------------------------------
// Horizontal movement (PLAYER_CUSTOM_HENG): the engine's, with the NES
// values for ice and quicksand.

signed int nn_ax, nn_rx, nn_vxmax;

void player_custom_heng (void) {
	if (nn_sand) { nn_ax = 23; nn_rx = 184; nn_vxmax = 77; }		// NES 1, 8, 8
	else if (nn_ice) { nn_ax = 23; nn_rx = 11; nn_vxmax = 461; }	// NES 1, 1/2, 48
	else { nn_ax = PLAYER_AX; nn_rx = PLAYER_RX; nn_vxmax = PLAYER_MAX_VX; }

	if ((pad0 & sp_LEFT) == 0) {
		player.vx -= nn_ax;
		if (player.vx < -nn_vxmax) player.vx = -nn_vxmax;
		player.facing = SIDEVIEW_FACING_LEFT;
		thrusting = 1;
	} else if ((pad0 & sp_RIGHT) == 0) {
		player.vx += nn_ax;
		if (player.vx > nn_vxmax) player.vx = nn_vxmax;
		player.facing = SIDEVIEW_FACING_RIGHT;
		thrusting = 1;
	} else {
		thrusting = 0;
		if (player.vx > 0) { player.vx -= nn_rx; if (player.vx < 0) player.vx = 0; }
		else if (player.vx < 0) { player.vx += nn_rx; if (player.vx > 0) player.vx = 0; }
	}
}

// ----------------------------------------------------------------------------
// Dying and respawning (the NES DIE_AND_RESPAWN): a killing tile takes a life
// and puts Nin Nin back on the last safe place: where he last stood on solid
// ground, or where he came into the screen.

signed int nn_safe_x, nn_safe_y;

void nn_set_safe (void) {
	nn_safe_x = player.x; nn_safe_y = player.y;
}

unsigned char custom_bg_hit (void) {
	if (hit_v == 0 && hit_h == 0) return 0;
	player.x = nn_safe_x; player.y = nn_safe_y;
	player.vx = player.vy = 0;
	gpx = player.x >> 6; gpy = player.y >> 6;
	return (player.estado == EST_NORMAL);
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
	_x = _y = 0;
	for (nn_fi = 0; nn_fi < 150; nn_fi ++) {
		if (map_buff [nn_fi] == rda) { _t = tileset_mappings [0]; nn_set_tile (); }
		_x ++; if (_x == 15) { _x = 0; _y ++; }
	}
}

unsigned char nn_fire_at_screen (void) {
	// 1 if FIRE did something here (then Nin Nin doesn't punch)
	#ifdef NN_CASTLE
		if (nn_castle_fire ()) return 1;
	#endif
	_x = (gpx + 8) >> 4; _y = (gpy + 16) >> 4;
	if (_y > 9) return 0;
	nn_m = _x + (_y << 4) - _y;
	nn_ft = map_buff [nn_m];

	if (nn_ft == NN_TILE_RAY) {
		_t = NN_TILE_RAY_OFF; nn_set_tile ();
		peta_el_beeper (6);
		nn_rays --;
		if (nn_rays == 0) {
			peta_el_beeper (8);
			#ifdef NN_CASTLE
				if (nn_castle_rays ()) return 1;
			#endif
			rda = NN_TILE_FENCE; nn_clear_tiles ();
		}
		return 1;
	}

	if (nn_ft == NN_TILE_FACE) {
		// chains over it
		rda = 0;
		while (nn_m >= 15) {
			nn_m -= 15;
			if (map_buff [nn_m] != NN_TILE_CHAIN) break;
			rda ++;
		}
		if (rda == 0) {
			nn_face_count = 0; nn_face_cleared = 1;
			peta_el_beeper (6);
		} else if (rda == nn_face_count + 1) {
			nn_face_count = rda;
			nn_face_cleared ++;
			if (nn_face_cleared == nn_faces) {
				peta_el_beeper (8);
				rda = NN_TILE_SPIKES; nn_clear_tiles ();
			} else peta_el_beeper (6);
		} else {
			nn_face_count = nn_face_cleared = 0;
			peta_el_beeper (2);
		}
		return 1;
	}
	return 0;
}

void nn_springs (void) {
	// behaviour 48 at the feet: spikes there
	_y = (gpy + 15) >> 4;
	if (_y > 9) return;
	_x = (gpx + 4) >> 4;
	if (map_attr [_x + (_y << 4) - _y] != 48) {
		_x = (gpx + 11) >> 4;
		if (map_attr [_x + (_y << 4) - _y] != 48) return;
	}
	_t = NN_TILE_SPIKES; nn_set_tile ();
	peta_el_beeper (7);
}
