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
	if (rdi == 0) nn_rays = nn_faces = 0;
	if (_t == 27) nn_rays ++;
	else if (_t == 36) nn_faces ++;

	rdb = rand ();
	nn_bg = tileset_mappings [0];

	if (_t == nn_bg) {
		if (rdi >= 15) {
			rda = map_buff [rdi - 15];
			if (rda == 3 || rda == 43) { _t = rda; return; }
			if (rda == 22 || rda == 23) {
				if (nn_stalactites && (rdb & 1)) _t = 9;
				return;
			}
			if (rda == 1) { _t = 2; return; }
			if (rda == 2 || rda == 44) {
				if ((rdb & 7) == 1) _t = 31;
				return;
			}
		}
		if (rdi) {
			rda = map_buff [rdi - 1];
			if (rda == 37 || rda == 38) { _t = rda + 1; return; }
		}
		if (nn_alt_bg && (rdb & 0x1f) < 2) {
			if (nn_alt_bg == 34) {
				// a bush: this one and the one to its left, if it's background
				if (rdx && map_buff [rdi - 1] == nn_bg) {
					map_buff [rdi - 1] = 33;
					map_attr [rdi - 1] = comportamiento_tiles [33];
					draw_coloured_tile (VIEWPORT_X + rdx - 2, VIEWPORT_Y + rdy, 33);
					_t = 34;
				}
			} else _t = nn_alt_bg;
		}
	} else if (_t == 22) {
		_t += (rdb & 1);
		if (rdi >= 15 && map_buff [rdi - 15] == 25) _t = 24;
	}
}
