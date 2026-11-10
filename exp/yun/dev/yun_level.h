// Yun - load constants. Hand-written. Included by config.h; the build scripts
// pass -DYUN_LOAD=1 or 2. The per level data of each load is in yun_load.h;
// comp.sh / comp_cpc.sh (and their .bat) have the same numbers for each load
// (levels, map sizes, lock tiles, SCREENS): keep them together.
//
//   load 1: levels 1-4 (NES maps 4, 0, 1, 2)
//   load 2: levels 5-8 (NES maps 3, 5, 6, 7)

#ifndef YUN_LOAD
	#define YUN_LOAD		1
#endif

#define YUN_LEVELS			4
// Each level has its own map width (VARIABLE_MAP_W, lvl_map_w); MAP_W x MAP_H
// is just the largest map of the load: 24 screens in load 1, 27 in load 2.
#if YUN_LOAD == 1
	#define YUN_MAP_W		4
	#define YUN_MAP_H		6
	#define MAX_LOCKS		2
#else
	#define YUN_MAP_W		9
	#define YUN_MAP_H		3
	#define MAX_LOCKS		4
#endif
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
// S = MAP_W * MAP_H screens (smaller maps are padded, screens=):
//   mapa      S x 75	mapcnv packed bin maxlocks=MAX_LOCKS
//   cerrojos  4 x MAX_LOCKS (np, x, y, st), unused ones np = 255
//   malotes   S x 27	ene2h bin: 3 enemies x 9 bytes per screen
//   hotspots  S x 3	  + (xy, type, act)
//   tile_map  128		mkts_om tmaps, tiles 0-15 (ZX 8 bytes/tile, CPC 4: 64)
// tile_map goes on with tiles 16-20 (hotspots), the same for every level, in
// the binary (tsx_tmaps.bin).
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
	#define SPR_VAR_BASE	59331	// AD_FREE (61440 - NUMBLOCKS 83 * 15 = 60195) - 6 * 144 (comp checks)
	#define LCELL(n)		((unsigned char *) (SPR_VAR_BASE + ((n) >> 1) * 144 + ((n) & 1)))
#endif

// Pezons, gyrosaws, cannons and the monococo in the wall (custom.h), fixed
// cells 22-31 (custom_sprite_cells.h)
#define YUN_EXTRA_ENEMS

// Level 8 (load 2, level 3 of the load: NES level 8) is dark: each room
// lights up for good when its bulb (hotspot type 4, tile 20) is taken; until
// then, lightning. ZX: Sprite Pack stops writing attributes (lib/splib2a.lib)
// and custom.h keeps the room black but for the 3x3 cells around Yun; the
// lightning fades INK 0 to 7 and back. CPC: dark_pal / light_pal.
#if YUN_LOAD == 2
	#define YUN_CYCLIC_LEVEL 0		// level 5 (12x2): the rows wrap, left of 0 is 11, left of 12 is 23 (custom_flick.h)
	#define YUN_DARK_LEVEL	3
#endif
#define HOTSPOT_TYPE_BULB	4
#define DARK_SPOT_ATTR		71		// ZX: around Yun, bright white on black
#define DARK_FLASH			6		// ZX: frames (25 Hz) the lightning stays at INK 7
