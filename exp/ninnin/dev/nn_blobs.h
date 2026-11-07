// Nin Nin CPC - the level blobs and palettes. Hand-written; the blobs are
// made by comp_cpc.sh / comp_cpc.bat with the MK1 utils, numbered 0.. in the
// load (cpc/lvl0_area.zx0 is the load's first level).

// Per level of this load (zx0): the level area (locks, enemies + hotspots,
// map) and the level's sprite cells
extern unsigned char lvl0_area [];
extern unsigned char lvl1_area [];
extern unsigned char lvl2_area [];
extern unsigned char lvl0_spr [];
extern unsigned char lvl1_spr [];
extern unsigned char lvl2_spr [];
#asm
	._lvl0_area
		BINARY "cpc/lvl0_area.zx0"
	._lvl1_area
		BINARY "cpc/lvl1_area.zx0"
	._lvl2_area
		BINARY "cpc/lvl2_area.zx0"
	._lvl0_spr
		BINARY "cpc/lvl0_spr.zx0"
	._lvl1_spr
		BINARY "cpc/lvl1_spr.zx0"
	._lvl2_spr
		BINARY "cpc/lvl2_spr.zx0"
#endasm
#if NN_LEVELS > 3
	extern unsigned char lvl3_area [];
	extern unsigned char lvl3_spr [];
	#asm
		._lvl3_area
			BINARY "cpc/lvl3_area.zx0"
		._lvl3_spr
			BINARY "cpc/lvl3_spr.zx0"
	#endasm
	unsigned char *lvl_area [] = { lvl0_area, lvl1_area, lvl2_area, lvl3_area };
	unsigned char *lvl_spr [] = { lvl0_spr, lvl1_spr, lvl2_spr, lvl3_spr };
#else
	unsigned char *lvl_area [] = { lvl0_area, lvl1_area, lvl2_area };
	unsigned char *lvl_spr [] = { lvl0_spr, lvl1_spr, lvl2_spr };
#endif

// The 4 inks of every level (mkts_om mode=pals of gfx/cpc/palN.png), all ten:
// nn_pal [] goes by the level's number in the game (nn_l). DE NADAL's surface
// screens use THE BOSQUE's.
#include "cpc/lvl0_pal.h"
#include "cpc/lvl1_pal.h"
#include "cpc/lvl2_pal.h"
#include "cpc/lvl3_pal.h"
#include "cpc/lvl4_pal.h"
#include "cpc/lvl5_pal.h"
#include "cpc/lvl6_pal.h"
#include "cpc/lvl7_pal.h"
#include "cpc/lvl8_pal.h"
#include "cpc/lvl9_pal.h"
unsigned char *nn_pal [] = {
	lvl0_pal, lvl1_pal, lvl2_pal, lvl3_pal, lvl4_pal,
	lvl5_pal, lvl6_pal, lvl7_pal, lvl8_pal, lvl9_pal
};
#define nn_pal_surface 	lvl0_pal
