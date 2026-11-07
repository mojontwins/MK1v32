// Yun, second load (NES levels 4-6: maps 2, 3, 5) - load constants.
// Hand-written. Included by config.h.

#define YUN_LEVELS			3
// Each level has its own map width (VARIABLE_MAP_W, lvl_map_w); MAP_W x MAP_H
// is just the largest map: 24 screens.
#define YUN_MAP_W			4
#define YUN_MAP_H			6
#define VARIABLE_MAP_W
#define YUN_SCR_INI			level_scr_ini
#define YUN_INI_X			level_ini_x
#define YUN_INI_Y			level_ini_y
#define LEVEL_KILLS			level_kills

// The font is the first thing in the charset (gfx/speccy and gfx/cpc tiles.spt): patterns 0-17
// are blank, 0-9, A E G M O R V. draw_text prints character c as pattern c - 32.
#define DIGIT_BASE			1
#define GAME_OVER_TEXT		game_over_text	// yun_load.h
#define HUD_TOTAL_X			23		// kills to win, after the / of the frame

// The level area (enems.h): every level blob (lvlN_area) decompresses over it.
// S = MAP_W * MAP_H screens (24 here; smaller maps are padded, screens=):
//   mapa      S x 75	mapcnv packed bin maxlocks=MAX_LOCKS
//   cerrojos  4 x MAX_LOCKS (np, x, y, st), unused ones np = 255
//   malotes   S x 27	ene2h bin: 3 enemies x 9 bytes per screen
//   hotspots  S x 3	  + (xy, type, act)
//   tile_map  128		mkts_om tmaps, tiles 0-15 (ZX 8 bytes/tile, CPC 4: 64)
// tile_map goes on with tiles 16-18 (hotspots), the same for every level, in
// the binary (tsx_tmaps.bin).
#define MAX_LOCKS			4
#ifdef CPC
	#define LEVEL_TMAP_SIZE	64
#else
	#define LEVEL_TMAP_SIZE	128
#endif

// Enemy sprite cells of the level (spritesN.png, 12 cells): each level
// decompresses its set here. ZX: sprcnv2 nomask blocks (a pair of cells every
// 144 bytes) right below AD_FREE (61440 - NUMBLOCKS * 15); the binary must end
// before (comp.sh checks). CPC: 64 bytes per cell, free RAM in the CPC_DIRTY map.
#ifdef CPC
	#define SPR_VAR_BASE	0xB680
	#define LCELL(n)		((unsigned char *) (SPR_VAR_BASE + (n) * 64))
#else
	#define SPR_VAR_BASE	59376	// 60240 - 6 * 144
	#define LCELL(n)		((unsigned char *) (SPR_VAR_BASE + ((n) >> 1) * 144 + ((n) & 1)))
#endif

// Pezons, gyrosaws, cannons and the monococo in the wall (custom.h), fixed
// cells 22-31 (custom_sprite_cells.h)
#define YUN_EXTRA_ENEMS
