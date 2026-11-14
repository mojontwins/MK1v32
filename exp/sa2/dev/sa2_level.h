// Sir Ababol 2 - load constants. Hand-written. Included by config.h; the build
// scripts pass -DSA2_LOAD=1 or 2. The per level data of each load is in
// sa2_load.h; comp_cpc.sh / comp_cpc.bat have the same levels for each load.
//
//   load 1: levels 1-3, part A's tiles (ZX levels 0-2)
//   load 2: levels 4-6, part B's tiles (ZX levels 3-4) and part C's (level 6,
//           CPC only)
// (tools/sa2_extract.py rebuilds them from the CPC and the ZX versions)
//
// Every level is a 1 x 20 tower of 15 x 10 screens, played from the bottom
// (screen 19) up.

#ifndef SA2_LOAD
	#define SA2_LOAD		1
#endif

#define SA2_LEVELS			3

// The level area (enems.h): every level blob (lvlN_area) decompresses over it.
//   mapa      20 x 75		mapcnv packed bin (no locks: maxlocks=1, lock tile 99)
//   cerrojos  4			one unused lock (np = 255)
//   malotes   20 x 27		ene2h bin: 3 enemies x 9 bytes per screen
//   hotspots  20 x 3		(unused)
//   tile_map  128			mkts_om tmaps: tiles 0-31 of the level, 4 patterns each
#define MAX_LOCKS			1
#define LEVEL_TMAP_SIZE		128

// The font is the first thing in the charset (gfx/font.png): patterns 0-63
// are ASCII 32-95, so draw_text prints character c as pattern c - 32 and
// the digits start at 16.
#define DIGIT_BASE			16

// Songs are unpacked to WYZ_SONG_BUFFER, up to 9000: room for the load's
// biggest song (comp_cpc checks both numbers and that the binary ends below).
#define SA2_SONG_MAX_1		2345	// level 3
#define SA2_SONG_BUFFER_1	0x86D7	// 0x9000 - 2345
#define SA2_SONG_MAX_2		1814	// level 6
#define SA2_SONG_BUFFER_2	0x88EA	// 0x9000 - 1814
#if SA2_LOAD == 1
	#define WYZ_SONG_BUFFER	SA2_SONG_BUFFER_1
#else
	#define WYZ_SONG_BUFFER	SA2_SONG_BUFFER_2
#endif
