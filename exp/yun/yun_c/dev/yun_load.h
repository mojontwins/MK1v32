// Yun, third load - per level data. Hand-written; the blobs are made by
// comp.bat / comp_cpc.bat with the MK1 utils.

// GAME OVER with the font patterns (yun_level.h): code = pattern + 32
//                            G   A   M   E   _   O   V   E   R
unsigned char game_over_text [] = { 45, 43, 46, 44, 32, 47, 49, 44, 48, 0 };

// start screen and tile, kills to win, map width (screens)
unsigned char lvl_scr_ini [] = { 14, 0 };
unsigned char lvl_ini_x [] = { 1, 2 };
unsigned char lvl_ini_y [] = { 4, 3 };
unsigned char lvl_kills [] = { 62, 56 };
unsigned char lvl_map_w [] = { 6, 9 };

// tile behaviours (tiles 0-15 of each level). NES quicksands (2) are plain
// platforms (4) here; slippery (16) is plain solid; conveyors (32): custom.h.
unsigned char lvl_behs [] = {
	0, 0, 0, 0, 0, 8, 4, 8, 4, 8, 8, 40, 41, 1, 4, 10,
	0, 8, 8, 8, 4, 8, 8, 8, 0, 0, 0, 0, 8, 8, 1, 0
};

// Level blobs (zx0): the level area (map + locks, enemies + hotspots, tile
// map) and the level's enemy sprite cells
extern unsigned char lvl0_area [];
extern unsigned char lvl1_area [];
extern unsigned char lvl0_spr [];
extern unsigned char lvl1_spr [];
#ifdef CPC
	#asm
		._lvl0_area
			BINARY "cpc/lvl0_area.zx0"
		._lvl1_area
			BINARY "cpc/lvl1_area.zx0"
		._lvl0_spr
			BINARY "cpc/lvl0_spr.zx0"
		._lvl1_spr
			BINARY "cpc/lvl1_spr.zx0"
	#endasm

	// palettes (mkts_om mode=pals): pens 0, 4, 8, 12 of pal_cnvtilesN.png
	// are the level's background inks; pens 1-3 of pal_cnvspr.png, the sprites'
	#include "cpc/lvl0_pal.h"
	#include "cpc/lvl1_pal.h"
	#include "cpc/spr_pal.h"
	// dark rooms and lightning (pal_dark.png, pal_light.png)
	#include "cpc/dark_pal.h"
	#include "cpc/light_pal.h"
	unsigned char *lvl_pal [] = { lvl0_pal, lvl1_pal };
#else
	#asm
		._lvl0_area
			BINARY "lvl0_area.zx0"
		._lvl1_area
			BINARY "lvl1_area.zx0"
		._lvl0_spr
			BINARY "lvl0_spr.zx0"
		._lvl1_spr
			BINARY "lvl1_spr.zx0"
	#endasm
#endif

unsigned char *lvl_area [] = { lvl0_area, lvl1_area };
unsigned char *lvl_spr [] = { lvl0_spr, lvl1_spr };
