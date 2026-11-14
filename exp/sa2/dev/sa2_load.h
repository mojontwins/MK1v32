// Sir Ababol 2 - per level data of each load. Hand-written; the blobs are made
// by comp_cpc.sh / comp_cpc.bat with the MK1 utils.

// tile behaviours of tiles 0-15 of each level (the original's tile_behaviour
// of its part: 1 kills, 4 platform, 8 obstacle, 16 slippery and breakable)
//   part A: 1-2 solid, 3-4 platforms, 6-9 solid, 10 platform, 11 ice block,
//           12 spikes, 13 gate
//   parts B, C: 1-2 solid, 3 platform, 5-8 solid, 9 platform, 11 ice block,
//           12 spikes, 13 gate
#define SA2_BEHS_A	0, 8, 8, 4, 4, 0, 8, 8, 8, 8, 4, 24, 1, 8, 0, 0
#define SA2_BEHS_BC	0, 8, 8, 4, 0, 8, 8, 8, 8, 4, 0, 24, 1, 8, 0, 0

extern unsigned char lvl0_area [];
extern unsigned char lvl1_area [];
extern unsigned char lvl2_area [];

#if SA2_LOAD == 1
	unsigned char lvl_behs [] = { SA2_BEHS_A, SA2_BEHS_A, SA2_BEHS_A };
	#asm
		._lvl0_area
			BINARY "cpc/lvl0_area.zx0"
		._lvl1_area
			BINARY "cpc/lvl1_area.zx0"
		._lvl2_area
			BINARY "cpc/lvl2_area.zx0"
	#endasm
#else
	unsigned char lvl_behs [] = { SA2_BEHS_BC, SA2_BEHS_BC, SA2_BEHS_BC };
	#asm
		._lvl0_area
			BINARY "cpc/lvl3_area.zx0"
		._lvl1_area
			BINARY "cpc/lvl4_area.zx0"
		._lvl2_area
			BINARY "cpc/lvl5_area.zx0"
	#endasm
#endif
unsigned char *lvl_area [] = { lvl0_area, lvl1_area, lvl2_area };
