// Yun - the level area (see yun_level.h). Hand-written: every level blob
// decompresses over all of it, from _level_area, in this order.

typedef struct {
	unsigned char x, y;
	unsigned char x1, y1, x2, y2;
	signed char mx, my;
	signed char t;
} MALOTE;

typedef struct {
	unsigned char xy, tipo, act;
} HOTSPOT;

#define TOTAL_EXISTING_ENEMS 72

extern MALOTE malotes [];
extern HOTSPOT hotspots [];

#asm
	._level_area
	._mapa
		defs 1800
	._cerrojos
		defs MAX_LOCKS * 4
	._malotes
		defs 648
	._hotspots
		defs 72
	._tile_map
		defs LEVEL_TMAP_SIZE
	// tiles 16-18, shared
#endasm
#ifdef CPC
	#asm
		BINARY "cpc/tsx_tmaps.bin"
	#endasm
#else
	#asm
		BINARY "tsx_tmaps.bin"
	#endasm
#endif
