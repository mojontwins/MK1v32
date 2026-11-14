// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// config.h - Sir Ababol 2 (CPC, nibble mode 0, CPC_DIRTY, MK1's 64 x 192 screen)
//
// The original runs at about 26 fps (measured: 25-28, dips to 20), so its
// speeds are used as they are at a fixed 25 fps (MIN_FAPS_PER_FRAME 2).

#include "sa2_level.h"					// load constants (hand-written)

#define MIN_FAPS_PER_FRAME 			2		// 50 / N fps if possible
#define BLACK_PEN		 			0		// For CPC, which pen is black?
//#define AUTO_SPLIT 						// For CPC, but hud must be 32 raster on top and MODE 1
//#define ALWAYS_SPLIT 						// Useful for DIRECT_TO_PLAY
//#define NO_PAL_MAP 						// For CPC, with AUTO_SPLIT, only one pal (my_inks)

// TURN THIS OFF FOR RELEASE!!!!
//#define DEBUG_NAVIGATOR 					// s + directions (ZX) or x + directions (CPC) to nav map

//#define PACKED_ENEMS 						// Store x1, y1, x2, y2 as two XY bytes.
//#define FIXED_ENEMS_LIMITS  				// If x1, y1, x2, y2 NEVER change.
//#define INDEXED_ENEMS 					// Don't include empty enems & use index

// ============================================================================
// I. General configuration
// ============================================================================

// In this section we define map dimmensions, initial and authomatic ending conditions, etc.

#define MAP_W						1		//
#define MAP_H						20		// Map dimmensions in screens: every level is a 1 x 20 tower
#define SCR_INICIO					19		// Initial screen: the bottom
#define PLAYER_INI_X				12		//
#define PLAYER_INI_Y				5		// Initial tile coordinates
//#define SCR_FIN 					99		// Last screen. 99 = deactivated.
//#define PLAYER_FIN_X				99		//
//#define PLAYER_FIN_Y				99		// Player tile coordinates to finish game
#define PLAYER_NUM_OBJETOS			1		// SA2: killing a boss (sa2_play.h) wins the level
//#define WIN_ON_KILLS				20		// Win when player.killed reaches this (ANDed with the above if defined)
#define PLAYER_LIFE 				3		// Max and starting life gauge.
#define PLAYER_REFILL				1		// Life recharge

// ============================================================================
// II. Engine type
// ============================================================================

// This section is used to define the game engine behaviour. Many directives are related,
// and others are mutually exclusive. I think this will be pretty obvious when you look at them. 

// Right now the shooting engine is only compatible with the side-view engine.

// General directives:
// -------------------

//#define PLAYER_PUSH_BOXES 				// If defined, tile #14 is pushable
#define DIRECT_TO_PLAY						// If defined, title screen is also the game frame.
#define CUSTOM_SCREENS 						// SA2: title, level, game over, password, cutscenes: custom_screens.h
#define DEACTIVATE_KEYS						// SA2: keys and gates are map tiles (custom.h)
//#define FIRE_TO_USE_KEY 					// If defined, press fire to use key.
#define DEACTIVATE_OBJECTS					// SA2: coins and lives are map tiles (custom.h)
//#define ONLY_ONE_OBJECT					// If defined, only one object can be carried at a time.
//#define OBJECT_COUNT				1		// Defines which FLAG will be used to store the object count.
//#define DEACTIVATE_EVIL_TILE				// If defined, no killing tiles (behaviour 1) are detected.
//#define PLAYER_BOUNCES					// If defined, collisions make player bounce
#define PLAYER_FLICKERS 				 	// If defined, collisions make player flicker instead.
//#define ENABLE_BOTIBOINS					// If defined, enable botiboins on beh 2.

// Shooting behaviour (only side view!)
// ------------------------------------
//#define PLAYER_CAN_FIRE 					// If defined, shooting engine is enabled.
//#define PLAYER_BULLET_SPEED 		8		// Pixels/frame. 
//#define MAX_BULLETS 				3		// Max number of bullets on screen. Be careful!.
//#define PLAYER_BULLET_Y_OFFSET	4		// vertical offset from the player's top.
//#define ENEMS_LIFE_GAUGE			5		// Amount of shots needed to kill enemies.
//#define PLAYER_AX_RECOIL 			128		// Push player back a bit when firing

// Random Respawn Fanties
// ----------------------
//#define RANDOM_RESPAWN					// If defined, automatic flying enemies spawn on killed enemies
//#define FANTY_MAX_V 				256 	// Flying enemies max speed.
//#define FANTY_A 					12		// Flying enemies acceleration.
//#define FANTIES_LIFE_GAUGE		10		// Amount of shots needed to kill flying enemies.

// Make your own!
// --------------
#define ENABLE_CUSTOM_ENEMS					// SA2: enemies and bosses, sa2_play.h

// Scripting
// ---------

//#define ACTIVATE_SCRIPTING				// Activates msc scripting and flag related stuff.
//#define SCRIPTING_DOWN					// Use DOWN as the action key.
//#define SCRIPTING_KEY_M					// Use M as the action key instead.
											// If none are selected, FIRE is used.
//#define NO_INDEXED_SCRIPTING 				// You aren't using * SCREEN * sections.
//#define ENABLE_ENCODED_TEXT 				// Add support for TEXT BOX
//#define ENABLE_DIALOG						// Add support for DIALOG

// Top view:
// ---------

//#define PLAYER_MOGGY_STYLE				// Enable top view.
//#define FAKE_SIDE_VIEW 					// For gravity 0 games! Needs PLAYER_MOGGY_STYLE enabled.

// Side view:
// ----------

#define PLAYER_HAS_JUMP 					// If defined, player is able to jump.
//#define JUMP_ON_UP_ONLY 					// Jump with UP only (as with PLAYER_CAN_FIRE), leaving FIRE for custom.h
//#define PLAYER_HAS_JETPAC 				// If defined, player can thrust a vertical jetpac
//#define PLAYER_KILLS_ENEMIES		  		// If defined, stepping on enemies kills them
//#define PLAYER_MIN_KILLABLE 		3		// Only kill enemies with id >= PLAYER_MIN_KILLABLE
//#define ENEMIES_CAN_DIE					// enems_kill () + KILLED counter without PLAYER_KILLS_ENEMIES (kill from custom.h)
#define SIMPLE_PLATFORMS 					// No type 4 tiles or no type 8s over type 4s.
//#define AVOID_PLATFORM_HOP 				// Only works if SIMPLE_PLATFORMS is DISABLED!
//#define JUMP_DOWN_PLATFORM 				// Use down on type 4s to jump down.
//#define DEACTIVATE_MOVING_PLATFORMS 		// No moving platforms, type 4s are normal linear.

// Custom:
// -------

//#define PLAYER_CUSTOM_VENG 				// Replace Vertical axis for your own 
//#define PLAYER_CUSTOM_HENG 				// Replace Horizontal axis for your own
//#define PLAYER_CUSTOM_FRAME 				// Replace frame number selection for your own
//#define PLAYER_CUSTOM_BG_HIT 				// Implement your own bounce.
#define PLAYER_CUSTOM_MOVE 					// SA2: the whole player engine is player_custom_move () (sa2_play.h)
//#define ENABLE_CUSTOM_HOTSPOTS 			// Add your own hotspots!
#define ENABLE_CUSTOM_FLICK 				// SA2: custom_flick.h
#define ENABLE_CUSTOM_SPRITES 				// SA2: the sword
//#define NUMBLOCKS 				40		// (ZX) splib2 blocks: raise 10 per 16x16 / 5 per 8x8 custom sprite
#define MAX_CUSTOM_SW_SPRITES		1		// (CPC) sprite slots for you, from SP_CUSTOM_BASE
#define ENABLE_CUSTOM_GAME_START 			// custom_game_start () when a game starts
#define ENABLE_CUSTOM_SCREEN_ENTER 			// custom_screen_enter () after a screen is set up
#define ENABLE_CUSTOM_PRE_MOVE 				// custom_pre_move () every frame: keys read, enemies done, player not moved yet
#define ENABLE_CUSTOM_LEVELS 				// SA2: 3 levels per load (custom_level_setup / custom_level_won)

#define CUSTOM_SPRITE_CELLS 				// SA2: the 22 frames of cpc/sprites.h (custom_sprite_cells.h)
#define CPC_CUSTOM_KEYS 					// SA2: def_keys / def_keys_joy in cpc_keys.h (A D W, N sword, M jump)
//#define ENEMS_CELL_OFFSET 		8 		// If the above is defined, this is the 1st cell for the enemies.

// ============================================================================
// III. Screen configuration
// ============================================================================

// This sections defines how stuff is rendered, where to show counters, etcetera
// It's good practice to comment what you are not using!

// The original's screens are cut to MK1's 64 x 192 (sa2_extract.py): the
// play area is char 1, row 1; the HUD numbers are on row 22.
#define VIEWPORT_X					1		//
#define VIEWPORT_Y					1		// Viewport character coordinates
#define LIFE_X						8		//
#define LIFE_Y						22		// Life gauge counter character coordinates
//#define OBJECTS_X					13		//
//#define OBJECTS_Y					22		// Objects counter character coordinates
//#define OBJECTS_ICON_X			9		// 
//#define OBJECTS_ICON_Y			21		// Objects icon character coordinates (use with ONLY_ONE_OBJECT)
//#define KEYS_X					19		//
//#define KEYS_Y					22		// Keys counter character coordinates
//#define KILLED_X					20		//
//#define KILLED_Y					21		// Kills counter character coordinates
//#define ITEM_X 					7		// 
//#define ITEM_Y 					0 		//
//#define ITEM_FLAG 				0 		// Show item @ flag
//#define TEXT_X 					4		//
//#define TEXT_Y					3		// To use TEXT from the script
//#define TEXT_A 					71      // (Speccy only) text atribute.

// Graphic FX, uncomment which applies...

// (CPC) build with -DCPC_DIRTY (and -lcpcdirty) for the renderer without superbuffer
// (cpc_dirty.h, lib/cpcdirty): -DMODE_1, -DMODE_0 (split palette, sprites ORed) or:
#define MODE_0_NIBBLE 					// (CPC_DIRTY, MODE_0) 16 pens for tiles and sprites, masked sprites
#define TRANSPARENT_PEN 			12	// (MODE_0_NIBBLE) the sprites' transparent pen (the original's grey)
//#define CPC_SCREEN_80 				// (CPC_DIRTY) standard 80 x 200 screen (screens: mkts_om mode=scr)
#define CHR_MAPPED_TILES 				// tile t = 4 chars from tile_map [t*8..] (mkts_om tile maps: attr, char x 4; CPC_DIRTY: 4 patterns)
//#define TILE_ATTR_MASK 				// (ZX, CHR_MAPPED_TILES) tiles drawn with (attr & tile_attr_and) | tile_attr_or
//#define VARIABLE_MAP_W 				// map width per level in map_w; MAP_W x MAP_H only sizes the arrays
//#define DIGIT_BASE 				16		// (ZX) character of '0' for the HUD numbers
//#define GAME_OVER_TEXT 			my_text	// char array (codes + 32, 0-ended) for game over; default "GAME OVER!"
//#define USE_AUTO_SHADOWS				// Automatic shadows made of darker attributes
//#define USE_AUTO_TILE_SHADOWS			// Automatic shadows using specially defined tiles 32-47.
//#define UNPACKED_MAP					// Full, uncompressed maps. Shadows settings are ignored.
//#define RLE_MAP 					53 	// Use RLE compression (values 44, 54, 62; see docs)
//#define MAPPED_TILESETS 				// RLE_MAP: tile = tileset_mappings [tile] (pointer to 16 bytes you set)
#define CUSTOM_RENDERER 					// custom_renderer () may change each tile before it is drawn (SA2: backdrop, sparkles)
//#define CUSTOM_CLEAR_TILE 			// custom_clear_tile () sets _t, _n: what an opened lock leaves (default tile 0)
//#define NO_MASKS		sp_OR_SPRITE 	// Sprites are rendered using OR instead of masks.
//#define PLAYER_ALTERNATE_ANIMATION	// If defined, animation is 1,2,3,1,2,3... 
#define NO_ALT_BG 						// No subs 0 for 19 at random
//#define HOTSPOTS_FIRST_TILE		32 	// Override default value of 16
#define REVERSE_OBJECT_COUNT 			// Count objects left rather than objects got
//#define ENABLE_ANIMATED_TILES 	46  // Must be EVEN
//#define TILANIM_PERIOD 			16  // Must be power of two

// ============================================================================
// IV. Player movement configuration
// ============================================================================

// This section is used to define which constants are used in the gravity/acceleration engine.
// If a side-view engine is configured, we have to define vertical and horizontal constants
// separately. If a top-view engine is configured instead, the horizontal values are also
// applied to the vertical component, vertical values are ignored.

// IV.1. Vertical movement. Only for side-view.

// The original's values (config.h of the three parts)
#define PLAYER_MAX_VY_CAYENDO	512 	// Max falling speed (512/64 = 8 pixels/frame)
#define PLAYER_G				64		// Gravity acceleration (64/64 = 1 pixel/frame^2)

#define PLAYER_VY_INICIAL_SALTO 120		// Initial jump velocity
#define PLAYER_MAX_VY_SALTANDO	420 	// Max jump velocity
#define PLAYER_INCR_SALTO		100		// acceleration while JUMP is pressed
#define PLAYER_JUMP_FRAMES		6		// frames of thrust while JUMP is pressed

#define PLAYER_INCR_JETPAC		32		// Vertical jetpac gauge
#define PLAYER_MAX_VY_JETPAC	256 	// Max vertical jetpac speed

// IV.2. Horizontal (side view) or general (top view) movement.

#define PLAYER_MAX_VX			256 	// Max velocity (256/64 = 4 pixels/frame)
#define PLAYER_AX				48		// Acceleration
#define PLAYER_RX				64		// Friction

// ============================================================================
// V. Tile behaviour
// ============================================================================

// Defines the behaviour for each tile. Remember that if keys are activated, tile #15 is a bolt
// and, therefore, it should be made a full obstacle!

// 0 = Walkable (no action)
// 1 = Walkable and kills.
// 2 = Walkable and hides.
// 4 = Platform (only stops player if falling on it)
// 8 = Full obstacle (blocks player from all directions)
// 16 = Stairs

// 128|N = Special tile (scripting)

// Per level, in sa2_load.h (comportamiento_tiles is in mapa.h)

// On CPC, using MODE_1 and AUTO_SPLIT, add here your custom palete includes
// Then change cpc/palmap.h
#if defined CPC && defined MODE_1 && defined AUTO_SPLIT

	#include "cpc/pal0.h"
	#include "cpc/pal1.h"
	#include "cpc/pal2.h"
	#include "cpc/pal3.h"

#endif
