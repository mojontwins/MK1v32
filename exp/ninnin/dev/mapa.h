// Nin Nin - the map part of the level area (see nn_load.h). Hand-written:
// the data comes in the level blobs; the room for it is in enems.h.

#define MAX_CERROJOS MAX_LOCKS

typedef struct {
	unsigned char np, x, y, st;
} CERROJOS;

extern unsigned char mapa [];
extern CERROJOS cerrojos [];

// The per level tables (generated), comportamiento_tiles [48] among them
#include "nn_levels.h"

// tile t = patterns tile_map [t * 4 .. t * 4 + 3] (CPC_DIRTY, CHR_MAPPED_TILES):
// 64 + t * 4 .., or 0 where the pattern is all black (filled faster). Made by
// custom_game_init () (custom.h).
unsigned char tile_map [192];
