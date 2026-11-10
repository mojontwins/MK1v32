// Nin Nin - the map renderer (CUSTOM_RENDERER). Hand-written, from the NES
// game's my/custom_renderer.h.
//
// The RLE44 maps leave out detail so they repeat more; it comes back here,
// tile by tile, with the rules of the NES game. The tile is in _t (a global
// tile, CPC numbering), rdi its index (0-149; above = rdi - 15, left = rdi - 1).
// map_buff [] before rdi is final, after it still the decoded tiles. The RNG
// is seeded with the screen number by draw_scr, so a screen always comes out
// the same.
//
//  * The level's background tile (its tile 0):
//      - under a trunk (3) or a chain (43): more trunk / chain
//      - under rocks (22, 23): stalactites (9), at random, where there are
//      - under grass (1): earth (2)
//      - under earth (2) or bricks (44): a root (31) now and then
//      - right of leaves 37 / 38: the next leaf
//      - else, now and then, the level's "alt" tile: a vine (42), or a bush
//        (33 + 34) where the tile to the left is background too
//  * Rocks (22): 22 or 23 at random, 24 under 25
//  * It counts the ray (27) and face (36) tiles, for the puzzles (nn_play.h)

unsigned char nn_bg;
unsigned char nn_rays, nn_faces;		// ray (27) and face (36) tiles in the screen (nn_play.h)

void custom_renderer (void) {
	// if (rdi == 0) nn_rays = nn_faces = 0;
	// if (_t == 27) nn_rays ++;
	// else if (_t == 36) nn_faces ++;
	// 
	// rdb = rand ();
	// nn_bg = tileset_mappings [0];
	// 
	// if (_t == nn_bg) {
	// 	if (rdi >= 15) {
	// 		rda = map_buff [rdi - 15];
	// 		if (rda == 3 || rda == 43) { _t = rda; return; }
	// 		if (rda == 22 || rda == 23) {
	// 			if (nn_stalactites && (rdb & 1)) _t = 9;
	// 			return;
	// 		}
	// 		if (rda == 1) { _t = 2; return; }
	// 		if (rda == 2 || rda == 44) {
	// 			if ((rdb & 7) == 1) _t = 31;
	// 			return;
	// 		}
	// 	}
	// 	if (rdi) {
	// 		rda = map_buff [rdi - 1];
	// 		if (rda == 37 || rda == 38) { _t = rda + 1; return; }
	// 	}
	// 	if (nn_alt_bg && (rdb & 0x1f) < 2) {
	// 		if (nn_alt_bg == 34) {
	// 			// a bush: this one and the one to its left, if it's background
	// 			if (rdx && map_buff [rdi - 1] == nn_bg) {
	// 				map_buff [rdi - 1] = 33;
	// 				map_attr [rdi - 1] = comportamiento_tiles [33];
	// 				draw_coloured_tile (VIEWPORT_X + rdx - 2, VIEWPORT_Y + rdy, 33);
	// 				_t = 34;
	// 			}
	// 		} else _t = nn_alt_bg;
	// 	}
	// } else if (_t == 22) {
	// 	_t += (rdb & 1);
	// 	if (rdi >= 15 && map_buff [rdi - 15] == 25) _t = 24;
	// }
	#asm
			ld  a, (_rdi)
			or  a
			jr  nz, nn_cr_1
			ld  (_nn_rays), a
			ld  (_nn_faces), a
		.nn_cr_1
			ld  a, (__t)
			cp  27
			jr  nz, nn_cr_2
			ld  hl, _nn_rays
			inc (hl)
			jr  nn_cr_3
		.nn_cr_2
			cp  36
			jr  nz, nn_cr_3
			ld  hl, _nn_faces
			inc (hl)
		.nn_cr_3
			call _rand
			ld  a, l
			ld  (_rdb), a
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
			ld  (_nn_bg), a
			ld  c, a
			ld  a, (__t)
			cp  c
			jp  nz, nn_cr_rocks

			// the background: what's above
			ld  a, (_rdi)
			cp  15
			jr  c, nn_cr_left
			sub 15
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (hl)
			ld  (_rda), a
			cp  3
			jr  z, nn_cr_set
			cp  43
			jr  z, nn_cr_set
			cp  22
			jr  z, nn_cr_stal
			cp  23
			jr  z, nn_cr_stal
			cp  1
			jr  z, nn_cr_earth
			cp  2
			jr  z, nn_cr_root
			cp  44
			jr  nz, nn_cr_left
		.nn_cr_root
			ld  a, (_rdb)
			and 7
			cp  1
			ret nz
			ld  a, 31
			jr  nn_cr_set
		.nn_cr_earth
			ld  a, 2
			jr  nn_cr_set
		.nn_cr_stal
			ld  a, (_nn_stalactites)
			or  a
			ret z
			ld  a, (_rdb)
			and 1
			ret z
			ld  a, 9
		.nn_cr_set
			ld  (__t), a
			ret

			// what's to the left
		.nn_cr_left
			ld  a, (_rdi)
			or  a
			jr  z, nn_cr_alt
			dec a
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (hl)
			ld  (_rda), a
			cp  37
			jr  z, nn_cr_leaf
			cp  38
			jr  nz, nn_cr_alt
		.nn_cr_leaf
			inc a
			jr  nn_cr_set

			// the level's alt tile, now and then
		.nn_cr_alt
			ld  a, (_nn_alt_bg)
			or  a
			ret z
			ld  b, a
			ld  a, (_rdb)
			and 0x1f
			cp  2
			ret nc
			ld  a, b
			cp  34
			jr  nz, nn_cr_set
			// a bush: this one and the one to its left, if it's background
			ld  a, (_rdx)
			or  a
			ret z
			ld  a, (_rdi)
			dec a
			ld  e, a
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (_nn_bg)
			cp  (hl)
			ret nz
			ld  (hl), 33
			ld  hl, _map_attr
			ld  a, e
			call nn_add_a
			ld  a, (_comportamiento_tiles + 33)
			ld  (hl), a
			ld  a, (_rdx)
			add a, 256 + VIEWPORT_X - 2
			ld  (__x), a
			ld  a, (_rdy)
			add a, VIEWPORT_Y
			ld  (__y), a
			ld  a, 33
			ld  (__t), a
			call _draw_coloured_tile_do
			ld  a, 34
			jr  nn_cr_set

			// rocks
		.nn_cr_rocks
			cp  22
			ret nz
			ld  a, (_rdb)
			and 1
			add a, 22
			ld  (__t), a
			ld  a, (_rdi)
			cp  15
			ret c
			sub 15
			ld  hl, _map_buff
			call nn_add_a
			ld  a, (hl)
			cp  25
			ret nz
			ld  a, 24
			ld  (__t), a
	#endasm
}
