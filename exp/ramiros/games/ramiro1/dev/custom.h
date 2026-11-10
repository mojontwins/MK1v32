// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// custom.h - Ramiro el Vampiro en "No tiene un respiro"
//
// Everything Ramiro needs lives in addons/ramiro.h (movement, animation,
// type 6 enemies, per-type damage, Ramiro 4 screen connections, evil zone,
// coins, fire zone, BeepFX, text box). It defines these hooks:
//
//   player_custom_move ()   (PLAYER_CUSTOM_MOVE, called from engine.h)
//   player_custom_frame ()  (PLAYER_CUSTOM_FRAME)
//   extra_enems_init / move / checks / killed, this_enemy_kills ()
//                           (ENABLE_CUSTOM_ENEMS)
//   flick_left / right / up / down ()   (ENABLE_CUSTOM_FLICK)
//
// The per-game settings (RAMIRO_*) are in config.h.

#include "../../../addons/ramiro.h"

void before_vsync (void) {
	#asm
			jp  _ramiro_before_vsync
	#endasm
}

#ifdef ENABLE_CUSTOM_HOTSPOTS
	unsigned char custom_hotspots() {
	}
#endif
