// Yun - per level data of each load. Hand-written; the blobs are made by
// comp.sh / comp_cpc.sh (and their .bat) with the MK1 utils.

// GAME OVER with the font patterns (yun_level.h): code = pattern + 32
//                            G   A   M   E   _   O   V   E   R
unsigned char game_over_text [] = { 45, 43, 46, 44, 32, 47, 49, 44, 48, 0 };

#if YUN_LOAD == 1
	// levels 1-4: start screen and tile, kills to win, map width (screens)
	unsigned char lvl_scr_ini [] = { 12, 20, 20, 23 };
	unsigned char lvl_ini_x [] = { 2, 2, 2, 12 };
	unsigned char lvl_ini_y [] = { 2, 4, 7, 4 };
	unsigned char lvl_kills [] = { 20, 40, 48, 61 };
	unsigned char lvl_map_w [] = { 4, 4, 4, 4 };

	// tile behaviours (tiles 0-15 of each level). NES quicksands (2) are plain
	// platforms (4) here; slippery (16) is plain solid; conveyors (32): custom.h.
	unsigned char lvl_behs [] = {
		0, 0, 0, 0, 0, 8, 0, 8, 0, 0, 0, 0, 4, 0, 0, 0,
		0, 0, 0, 0, 8, 8, 8, 8, 0, 8, 0, 0, 1, 4, 8, 10,
		0, 0, 0, 0, 8, 4, 8, 4, 40, 41, 1, 1, 1, 24, 24, 10,
		0, 0, 0, 0, 8, 8, 8, 8, 0, 8, 0, 4, 1, 4, 8, 10
	};

	// Level blobs (zx0): the level area (map + locks, enemies + hotspots, tile
	// map) and the level's enemy sprite cells
	extern unsigned char lvl0_area [];
	extern unsigned char lvl0_spr [];
	extern unsigned char lvl1_area [];
	extern unsigned char lvl1_spr [];
	extern unsigned char lvl2_area [];
	extern unsigned char lvl2_spr [];
	extern unsigned char lvl3_area [];
	extern unsigned char lvl3_spr [];
	#ifdef CPC
		#asm
			._lvl0_area
				BINARY "cpc/lvl0_area.zx0"
			._lvl0_spr
				BINARY "cpc/lvl0_spr.zx0"
			._lvl1_area
				BINARY "cpc/lvl1_area.zx0"
			._lvl1_spr
				BINARY "cpc/lvl1_spr.zx0"
			._lvl2_area
				BINARY "cpc/lvl2_area.zx0"
			._lvl2_spr
				BINARY "cpc/lvl2_spr.zx0"
			._lvl3_area
				BINARY "cpc/lvl3_area.zx0"
			._lvl3_spr
				BINARY "cpc/lvl3_spr.zx0"
		#endasm
		// palettes (mkts_om mode=pals): pens 0, 4, 8, 12 of pal_cnvtilesN.png
		// are the level's background inks
		#include "cpc/lvl0_pal.h"
		#include "cpc/lvl1_pal.h"
		#include "cpc/lvl2_pal.h"
		#include "cpc/lvl3_pal.h"
		unsigned char *lvl_pal [] = { lvl0_pal, lvl1_pal, lvl2_pal, lvl3_pal };
	#else
		#asm
			._lvl0_area
				BINARY "lvl0_area.zx0"
			._lvl0_spr
				BINARY "lvl0_spr.zx0"
			._lvl1_area
				BINARY "lvl1_area.zx0"
			._lvl1_spr
				BINARY "lvl1_spr.zx0"
			._lvl2_area
				BINARY "lvl2_area.zx0"
			._lvl2_spr
				BINARY "lvl2_spr.zx0"
			._lvl3_area
				BINARY "lvl3_area.zx0"
			._lvl3_spr
				BINARY "lvl3_spr.zx0"
		#endasm
	#endif
	unsigned char *lvl_area [] = { lvl0_area, lvl1_area, lvl2_area, lvl3_area };
	unsigned char *lvl_spr [] = { lvl0_spr, lvl1_spr, lvl2_spr, lvl3_spr };
#else
	// levels 5-8: start screen and tile, kills to win, map width (screens)
	unsigned char lvl_scr_ini [] = { 12, 12, 14, 0 };
	unsigned char lvl_ini_x [] = { 2, 7, 1, 2 };
	unsigned char lvl_ini_y [] = { 4, 1, 4, 3 };
	unsigned char lvl_kills [] = { 36, 49, 62, 56 };
	unsigned char lvl_map_w [] = { 12, 6, 6, 9 };

	// tile behaviours (tiles 0-15 of each level). NES quicksands (2) are plain
	// platforms (4) here; slippery (16) is plain solid; conveyors (32): custom.h.
	unsigned char lvl_behs [] = {
		0, 8, 8, 8, 8, 4, 8, 8, 0, 0, 0, 0, 1, 0, 0, 10,
		0, 4, 0, 0, 8, 8, 24, 24, 24, 0, 8, 4, 1, 0, 0, 10,
		0, 0, 0, 0, 0, 8, 4, 8, 4, 8, 8, 40, 41, 1, 4, 10,
		0, 8, 8, 8, 4, 8, 8, 8, 0, 0, 0, 0, 8, 8, 1, 0
	};

	// Level blobs (zx0): the level area (map + locks, enemies + hotspots, tile
	// map) and the level's enemy sprite cells
	extern unsigned char lvl4_area [];
	extern unsigned char lvl4_spr [];
	extern unsigned char lvl5_area [];
	extern unsigned char lvl5_spr [];
	extern unsigned char lvl6_area [];
	extern unsigned char lvl6_spr [];
	extern unsigned char lvl7_area [];
	extern unsigned char lvl7_spr [];
	#ifdef CPC
		#asm
			._lvl4_area
				BINARY "cpc/lvl4_area.zx0"
			._lvl4_spr
				BINARY "cpc/lvl4_spr.zx0"
			._lvl5_area
				BINARY "cpc/lvl5_area.zx0"
			._lvl5_spr
				BINARY "cpc/lvl5_spr.zx0"
			._lvl6_area
				BINARY "cpc/lvl6_area.zx0"
			._lvl6_spr
				BINARY "cpc/lvl6_spr.zx0"
			._lvl7_area
				BINARY "cpc/lvl7_area.zx0"
			._lvl7_spr
				BINARY "cpc/lvl7_spr.zx0"
		#endasm
		// palettes (mkts_om mode=pals): pens 0, 4, 8, 12 of pal_cnvtilesN.png
		// are the level's background inks
		#include "cpc/lvl4_pal.h"
		#include "cpc/lvl5_pal.h"
		#include "cpc/lvl6_pal.h"
		#include "cpc/lvl7_pal.h"
		unsigned char *lvl_pal [] = { lvl4_pal, lvl5_pal, lvl6_pal, lvl7_pal };
		// dark rooms and lightning (pal_dark.png, pal_light.png)
		#include "cpc/dark_pal.h"
		#include "cpc/light_pal.h"
	#else
		#asm
			._lvl4_area
				BINARY "lvl4_area.zx0"
			._lvl4_spr
				BINARY "lvl4_spr.zx0"
			._lvl5_area
				BINARY "lvl5_area.zx0"
			._lvl5_spr
				BINARY "lvl5_spr.zx0"
			._lvl6_area
				BINARY "lvl6_area.zx0"
			._lvl6_spr
				BINARY "lvl6_spr.zx0"
			._lvl7_area
				BINARY "lvl7_area.zx0"
			._lvl7_spr
				BINARY "lvl7_spr.zx0"
		#endasm
	#endif
	unsigned char *lvl_area [] = { lvl4_area, lvl5_area, lvl6_area, lvl7_area };
	unsigned char *lvl_spr [] = { lvl4_spr, lvl5_spr, lvl6_spr, lvl7_spr };
#endif

#ifdef CPC
	// pens 1-3 of pal_cnvspr.png: the sprite inks
	#include "cpc/spr_pal.h"
#endif
