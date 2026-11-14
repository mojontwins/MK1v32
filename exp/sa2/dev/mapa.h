// Sir Ababol 2 - the map part of the level area (see sa2_level.h). Hand-written:
// the data comes in the level blobs; the room for it is in enems.h.

#define MAX_CERROJOS MAX_LOCKS

typedef struct {
	unsigned char np, x, y, st;
} CERROJOS;

extern unsigned char mapa [];
extern CERROJOS cerrojos [];
extern unsigned char tile_map [];

// tiles 0-15 of the level (sa2_load.h); 16-47 are 0
unsigned char comportamiento_tiles [48];
