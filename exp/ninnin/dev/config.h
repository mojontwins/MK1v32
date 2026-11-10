// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// config.h - Nin Nin (CPC, mode 1, CPC_DIRTY)

#define MIN_FAPS_PER_FRAME 			2		// 50 / N fps if possible. Nin Nin: 25 Hz; physics below are for 2.
#define BLACK_PEN		 			1		// For CPC, which pen is black? (pen 1 in every level palette)

#include "nn_load.h"						// this load: levels, sizes (hand-written)

// TURN THIS OFF FOR RELEASE!!!!
//#define DEBUG_NAVIGATOR 					// s + directions (ZX) or x + directions (CPC) to nav map

// ============================================================================
// I. General configuration
// ============================================================================

#define DIRECT_TO_PLAY 						// The title screen has the frame: no s_marco

#define MAP_W						NN_MAP_W	// arrays for the largest map (VARIABLE_MAP_W)
#define MAP_H						NN_MAP_H	//
#define SCR_INICIO					level_scr_ini	// Initial screen (per level)
#define PLAYER_INI_X				level_ini_x		//
#define PLAYER_INI_Y				level_ini_y		// Initial tile coordinates
#define PLAYER_NUM_OBJETOS			1		// Nin Nin: the onigiri ends the level
#define PLAYER_LIFE 				10		// Max and starting life gauge.
#define PLAYER_REFILL				1		// Life recharge

// ============================================================================
// II. Engine type
// ============================================================================

//#define DEACTIVATE_KEYS					// If defined, keys are not present.
#define PLAYER_FLICKERS 				 	// If defined, collisions make player flicker instead.

#define ENABLE_CUSTOM_ENEMS					// Nin Nin's enemies: custom.h / nn_enems.h

#define PLAYER_HAS_JUMP 					// If defined, player is able to jump.
#define JUMP_ON_UP_ONLY 					// Jump with UP only: FIRE punches (the hitter)
#define ENEMIES_CAN_DIE						// enems_kill () + KILLED counter (the hitter kills)
#define SIMPLE_PLATFORMS 					// No type 4 tiles or no type 8s over type 4s.

#define PLAYER_CUSTOM_VENG 					// The jump (Nin Nin's) and swim engines, custom.h
#define PLAYER_CUSTOM_HENG 					// Ice and quicksand (nn_play.h)
#define PLAYER_CUSTOM_FRAME 				// Walk / jump / swim frames
#define PLAYER_CUSTOM_BG_HIT 				// Killing tiles: respawn on the last safe place (nn_play.h)
#define ENABLE_CUSTOM_SPRITES 				// cocos, the punch, explosions
#define MAX_CUSTOM_SW_SPRITES		4		// (CPC) sprite slots for you, from SP_CUSTOM_BASE
#define ENABLE_CUSTOM_GAME_START 			// custom_game_start () when a game starts
#define ENABLE_CUSTOM_SCREEN_ENTER 			// custom_screen_enter () after a screen is set up
#define ENABLE_CUSTOM_PRE_MOVE 				// custom_pre_move () every frame: keys read, enemies done, player not moved yet
#define ENABLE_CUSTOM_LEVELS 				// custom_level_setup () before each game/level, custom_level_won () to go on

#define CUSTOM_SPRITE_CELLS 				// Use your own sprite cell luts from custom_sprite_cells.h
#define ENEMS_CELL_OFFSET 			8 		// (CUSTOM_SPRITE_CELLS) enemy frames: sprite_cells [8..]

// ============================================================================
// III. Screen configuration
// ============================================================================

#define VIEWPORT_X					1		//
#define VIEWPORT_Y					2		// Viewport character coordinates
#define LIFE_X						3		//
#define LIFE_Y						0		// Life gauge counter character coordinates
#define DIGIT_BASE 					16		// '0' in the font
#define GAME_OVER_TEXT 				"GAME OVER!"	// the font is ASCII from 32

// (CPC) build with -DCPC_DIRTY for the renderer without superbuffer (cpc_dirty.h)
#define CHR_MAPPED_TILES 					// CPC_DIRTY: tile t = 4 patterns from tile_map [t*4..]
#define VARIABLE_MAP_W 						// map width per level in map_w; MAP_W x MAP_H only sizes the arrays
#define RLE_MAP 					44 		// RLE44 maps, 4-bit level tiles
#define MAPPED_TILESETS 					// tile = tileset_mappings [tile]: 16 of the 48 per level
#define CUSTOM_RENDERER 					// custom_renderer () adds the detail (nn_renderer.h)
#define NO_ALT_BG 							// No subs 0 for 19 at random
#define CUSTOM_CLEAR_TILE 					// an opened lock leaves the level's background tile

// ============================================================================
// IV. Player movement configuration
// ============================================================================

// From the NES game (60 Hz, 1/16 px) to 25 Hz, 1/64 px: speeds x 2.4 x 4,
// accelerations x 5.76 x 4.

#define PLAYER_MAX_VY_CAYENDO	614 	// NES 64
#define PLAYER_G				92		// NES 4

#define PLAYER_VY_INICIAL_SALTO 192		// (engine's jump, not used: PLAYER_CUSTOM_VENG)
#define PLAYER_MAX_VY_SALTANDO	312 	//
#define PLAYER_INCR_SALTO		32		//

#define PLAYER_MAX_VX			307 	// NES 32
#define PLAYER_AX				69		// NES 3
#define PLAYER_RX				92		// NES 4

// Nin Nin's jump and swim (custom.h)
#define PLAYER_G_JUMPING 		23		// NES 1
#define PLAYER_VY_JUMP_INITIAL 	499		// NES 52
#define PLAYER_VY_JUMP_STEPS 	7		// NES 16 frames
#define PLAYER_VY_JUMP_RELEASE 	230		// NES 24
#define PLAYER_AY_SWIM			46		// NES 2
#define PLAYER_VY_SWIM_MAX		154		// NES 16

// ============================================================================
// V. Tile behaviour
// ============================================================================

// comportamiento_tiles [] (48 tiles) is in nn_levels.h (generated). Nin Nin's
// values: 1 kills, 2 quicksand, 4 platform, 8 obstacle, 10 lock, 24 breakable,
// 40 / 41 conveyor left / right, 72 ice, 48 spring, 128 coin.
