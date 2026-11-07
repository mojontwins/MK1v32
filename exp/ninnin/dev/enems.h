// Nin Nin - the level area (see nn_load.h). Hand-written: every level blob
// decompresses over all of it, from _level_area, in this order.

typedef struct {
	unsigned char x, y;
	unsigned char x1, y1, x2, y2;
	signed char mx, my;
	unsigned char t;
} MALOTE;

typedef struct {
	unsigned char xy, tipo, act;
} HOTSPOT;

#define TOTAL_EXISTING_ENEMS MAP_W * MAP_H * 3		// no brackets: used in asm (ld bc, ...)

extern MALOTE malotes [];
extern HOTSPOT hotspots [];

#asm
	._level_area
	._cerrojos
		defs MAX_LOCKS * 4
	._malotes
		defs MAP_W * MAP_H * 27
	._hotspots
		defs MAP_W * MAP_H * 3
	._mapa
		defs NN_MAP_MAX
#endasm
