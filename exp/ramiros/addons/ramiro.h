// ramiro.h - Ramiro el Vampiro add-ons for MTE MK1 v3.2
// Copyleft 2013, 2016, 2021, 2026 by The Mojon Twins
//
// Everything a "Ramiro" game needs on top of MK1 v3.2, ported from
// Ramiro 4 (MK1 v4.8, games_zx/ramiro4) so that the vampire moves exactly
// like in Ramiro 4. All code is inline assembly.
//
// HOW TO USE IT
// -------------
// config.h of the game must define:
//
//   #define PLAYER_CUSTOM_MOVE          // engine.h calls player_custom_move ()
//   #define PLAYER_CUSTOM_FRAME         // player_custom_frame () below
//   #define ENABLE_CUSTOM_ENEMS         // type 6 bats + per-type damage
//   #define ENABLE_CUSTOM_FLICK         // Ramiro 4 vertical screen connections
//   #define ENEMIES_MAY_DIE             // (not needed; leave undefined)
//
// plus the Ramiro 4 physics (PLAYER_MAX_VY_CAYENDO, PLAYER_G,
// PLAYER_VY_INICIAL_SALTO, PLAYER_MAX_VY_SALTANDO, PLAYER_INCR_SALTO,
// PLAYER_MAX_VX, PLAYER_AX, PLAYER_RX) and the RAMIRO_* values below.
//
// custom.h of the game:   #include "../../../addons/ramiro.h"
//                         and call ramiro_before_vsync () from before_vsync ().
//                         The enemy/frame/flick hooks are defined here.
// extern.h of the game:   call ramiro_extern (n, m) from do_extern_action ()
//                         and implement textbox () with ramiro_textbox (ptr).
//
// WHAT'S IN
// ---------
//  * player_custom_move ()   Ramiro 4 move (): BOTH_KEYS_JUMP, RAMIRO_HOP,
//                            RAMIRO_HOVER, Mario-style variable jump, gravity,
//                            SHORT_PLAYER collision (12x12 box), better
//                            vertical connections (y down to -512), evil tile
//                            (EVIL_TILE_SIMPLE), conditional evil zone with
//                            slow kill, coins.
//  * player_custom_frame ()  Ramiro 4 custom animation.
//  * Type 6 enemies          Ramiro 4 flying enemies: always pursue, frozen
//                            while flags [RAMIRO_T6_FLAG] == 0.
//  * Per-type damage         RAMIRO_LINEAR_HIT / RAMIRO_FLYING_HIT, with the
//                            SHORT_PLAYER 8 pixel wide enemy collision.
//  * Fire zone               msc3's SET_FIRE_ZONE, via EXTERN.
//  * BeepFX sound effects    Ramiro 4's beeper set: rm_sfx (n).
//  * HUD                     evil zone gauge.
//  * Text box                Sir Ababol / Papaya's, for msc4's TEXT BOX.
//
// EXTERN numbers (do_extern_action -> ramiro_extern):
//   EXTERN 9, n    play BeepFX sound n (Ramiro 4 numbers, see rm_sfx)
//   EXTERN 10, x1  fire zone x1 (pixels)
//   EXTERN 11, y1  fire zone y1
//   EXTERN 12, x2  fire zone x2
//   EXTERN 13, y2  fire zone y2, and switch the zone on
//   EXTERN 14, 0   fire zone off (do it in ENTERING ANY)
//   EXTERN 16, 0   REDRAW: repaint the play area from the map buffer

// ----------------------------------------------------------------------------
// Defaults
// ----------------------------------------------------------------------------

#ifndef RAMIRO_LINEAR_HIT
	#define RAMIRO_LINEAR_HIT 		5 		// Life taken by linear enemies & evil tiles
#endif
#ifndef RAMIRO_FLYING_HIT
	#define RAMIRO_FLYING_HIT 		7		// Life taken by flying enemies (type 6)
#endif
#ifndef PLAYER_MAX_VY_CAYENDO_H
	#define PLAYER_MAX_VY_CAYENDO_H 256 	// Hovering: max falling speed
#endif
#ifndef PLAYER_G_HOVER
	#define PLAYER_G_HOVER 			4 		// Hovering: gravity
#endif
#ifndef EVIL_ZONE_FRAME_COUNT
	#define EVIL_ZONE_FRAME_COUNT 	8 		// Frames per countdown tick
#endif
#ifndef EVIL_ZONE_BEEPS_COUNT
	#define EVIL_ZONE_BEEPS_COUNT 	32 		// Ticks before it starts draining life
#endif
#ifndef EVIL_ZONE_FREQ
	#define EVIL_ZONE_FREQ 			3 		// Drain 1 life every (N+1) frames
#endif
#ifndef FANTY_MAX_V
	#define FANTY_MAX_V 			128
#endif
#ifndef FANTY_A
	#define FANTY_A 				8
#endif

// RAMIRO_COIN_TILE   tile that is collected when touched (crosses...)
// RAMIRO_COIN_FLAG   flag incremented per coin
// RAMIRO_COINS_REFILL life added per coin
// RAMIRO_EZ_SLOW_FLAG flag: != 0 -> evil zone (beh 3) kills slowly, with a
//                    countdown; == 0 -> it drains life at once.
// RAMIRO_EZ_ACTIVE_FLAG (optional) flag: == 0 -> evil zone off.
// RAMIRO_T6_FLAG     flag: == 0 -> type 6 enemies don't move.
// RAMIRO_MAKE_T6_FLAG (optional) flag: != 0 -> empty enemy slots become
//                    type 6 enemies at a random tile (Ramiro 2).
// RAMIRO_EZ_GAUGE_X/Y (optional) where to show the evil zone countdown.

#define RM_WALL_UP 		1
#define RM_WALL_LEFT 	3
#define RM_WALL_RIGHT 	4

// ----------------------------------------------------------------------------
// Variables
// ----------------------------------------------------------------------------

unsigned char rm_gpxx, rm_gpyy, rm_bj, rm_hop, rm_t;
signed int rm_rdj;
unsigned char rm_fz_on, rm_fzx1, rm_fzy1, rm_fzx2, rm_fzy2;
unsigned char rm_ezg_old = 0xff;

// ----------------------------------------------------------------------------
// BeepFX sound effects (Ramiro 4's beeper.h, by Shiru's BeepFX)
//
//   1 jump, 2 enemy hit, 3 killzone hit, 4 countdown, 5 coin, 6 object,
//   7 talk 1, 8 key in lock, 9 shoot, 10 explosion, 11 talk 2
// ----------------------------------------------------------------------------

#ifdef CPC
	// On CPC sound effects go through the engine (WYZ effects, defined in
	// TABLA_SONIDOS in wyz/instrumentos.h). This table maps the Ramiro 4
	// numbers above to WYZ effects; Ramiro 4's CPC set has 10 (0-9).
	// Define RAMIRO_CPC_SFX_TABLE and your own rm_cpc_sfx [] to change it.
	#ifndef RAMIRO_CPC_SFX_TABLE
		unsigned char rm_cpc_sfx [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 7 };
	#endif

	void __FASTCALL__ rm_sfx (unsigned char n) {
		#asm
				ld  h, 0
				ld  de, _rm_cpc_sfx
				add hl, de
				ld  l, (hl)
				jp  _peta_el_beeper
		#endasm
	}
#else
	void __FASTCALL__ rm_sfx (unsigned char n) {
		#asm
				push ix
				push iy
				ld  a, l
				call rm_sound_play
				pop iy
				pop ix
				ret

			.rm_sound_play
				ld  hl, rm_sfx_data
				di
				push iy

				ld  b, 0
				ld  c, a
				add hl, bc
				add hl, bc
				ld  a, (hl)
				inc hl
				ld  h, (hl)
				ld  l, a
				push hl
				pop ix

			.rm_sfx_read
				ld  a, (ix+0)
				or  a
				jr  nz, rm_sfx_read_sound
				pop iy
				ei
				ret

			.rm_sfx_read_sound
				ld  c, (ix+1)
				ld  b, (ix+2)
				ld  e, (ix+3)
				ld  d, (ix+4)
				push de
				pop iy

				dec a
				jr  nz, rm_sfx_noise

			.rm_sfx_tone
				ld  e, (ix+5)
				ld  d, (ix+6)
				ld  a, (ix+9)
				ld  (rm_sfx_tone_duty + 1), a
				ld  hl, 0

			.rm_sfx_tone_l0
				push bc
				push iy
				pop bc
			.rm_sfx_tone_l1
				add hl, de
				ld  a, h
			.rm_sfx_tone_duty
				cp  0
				sbc a, a
				and 16
				out ($fe), a

				dec bc
				ld  a, b
				or  c
				jr  nz, rm_sfx_tone_l1

				ld  a, (rm_sfx_tone_duty + 1)
				add a, (ix+10)
				ld  (rm_sfx_tone_duty + 1), a

				ld  c, (ix+7)
				ld  b, (ix+8)
				ex  de, hl
				add hl, bc
				ex  de, hl

				pop bc
				dec bc
				ld  a, b
				or  c
				jr  nz, rm_sfx_tone_l0

				ld  c, 11
			.rm_sfx_next
				add ix, bc
				jr  rm_sfx_read

			.rm_sfx_noise
				ld  e, (ix+5)
				ld  d, 1
				ld  h, d
				ld  l, d
			.rm_sfx_noise_l0
				push bc
				push iy
				pop bc
			.rm_sfx_noise_l1
				ld  a, (hl)
				and 16
				out ($fe), a
				dec d
				jr  nz, rm_sfx_noise_l2
				ld  d, e
				inc hl
				ld  a, h
				and $1f
				ld  h, a
			.rm_sfx_noise_l2
				dec bc
				ld  a, b
				or  c
				jr  nz, rm_sfx_noise_l1

				ld  a, e
				add a, (ix+6)
				ld  e, a

				pop bc
				dec bc
				ld  a, b
				or  c
				jr  nz, rm_sfx_noise_l0

				ld  c, 7
				jr  rm_sfx_next

			.rm_sfx_data
				defw rm_sfx0, rm_sfx1, rm_sfx2, rm_sfx3, rm_sfx4, rm_sfx5
				defw rm_sfx6, rm_sfx7, rm_sfx8, rm_sfx9, rm_sfx10, rm_sfx11

			.rm_sfx0
				defb 0x01
				defw 0x000a,0x03e8,0x00c8,0x0016,0x1680
				defb 0x00
			.rm_sfx1
				defb 0x01
				defw 0x0064,0x0014,0x01f4,0x0002,0x0010
				defb 0x00
			.rm_sfx2
				defb 0x02
				defw 0x0001,0x03e8,0x000a
				defb 0x01
				defw 0x0014,0x0064,0x0190,0xfff0,0x0080
				defb 0x02
				defw 0x0001,0x07d0,0x0001
				defb 0x00
			.rm_sfx3
				defb 0x01
				defw 0x0014,0x00c8,0x0d48,0x000a,0x0040
				defb 0x00
			.rm_sfx4
				defb 0x01
				defw 0x0050,0x0014,0x03e8,0xffff,0x0080
				defb 0x00
			.rm_sfx5
				defb 0x01
				defw 0x0004,0x03e8,0x03e8,0x0190,0x0080
				defb 0x00
			.rm_sfx6
				defb 0x01
				defw 0x0002,0x0fa0,0x0190,0x00c8,0x0040
				defb 0x01
				defw 0x0002,0x0fa0,0x00c8,0x00c8,0x0020
				defb 0x00
			.rm_sfx7
				defb 0x01
				defw 0x000a,0x03e8,0x00c8,0x0002,0x0010
				defb 0x01
				defw 0x0001,0x0fa0,0x0000,0x0000,0x0000
				defb 0x01
				defw 0x000a,0x03e8,0x00c8,0xfffe,0x0010
				defb 0x01
				defw 0x0001,0x07d0,0x0000,0x0000,0x0000
				defb 0x01
				defw 0x000a,0x03e8,0x00b4,0xfffe,0x0010
				defb 0x01
				defw 0x0001,0x0fa0,0x0000,0x0000,0x0000
				defb 0x00
			.rm_sfx8
				defb 0x02
				defw 0x0001,0x03e8,0x0014
				defb 0x01
				defw 0x0001,0x03e8,0x0000,0x0000,0x0000
				defb 0x02
				defw 0x0001,0x03e8,0x0001
				defb 0x00
			.rm_sfx9
				defb 0x02
				defw 0x0014,0x0032,0x0101
				defb 0x00
			.rm_sfx10
				defb 0x02
				defw 0x0064,0x01f4,0x0264
				defb 0x00
			.rm_sfx11
				defb 0x01
				defw 0x0014,0x01f4,0x00c8,0x0005,0x0110
				defb 0x01
				defw 0x0001,0x03e8,0x0000,0x0000,0x0000
				defb 0x01
				defw 0x001e,0x01f4,0x00c8,0x0008,0x0110
				defb 0x01
				defw 0x0001,0x07d0,0x0000,0x0000,0x0000
				defb 0x00
		#endasm
	}
#endif

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

void ramiro_helpers (void) {
	#asm
		// rm_attr: Ramiro 4's attr () with BETTER_VERTICAL_CONNECTIONS.
		// In: C = x, A = y. Out: L = behaviour.
		// y = 10 (just below the screen) -> 0. y > 10 (that is, negative:
		// above the screen) -> replicate the first row.
		.rm_attr
			cp  10
			jp  c, _attr_2
			cp  11
			jr  nc, rm_attr_neg
			ld  hl, 0
			ret
		.rm_attr_neg
			xor a
			jp  _attr_2

		// gpy = gpyy << 4; player.y = gpy << 6
		.rm_adjust_to_tile_y
			ld  a, (_rm_gpyy)
			sla a
			sla a
			sla a
			sla a
			ld  (_gpy), a
			call Ashl16_HL
			ld  (_player + 2), hl
			ret

		// Coin at (__x, __y)?  C = x, A = y
		.rm_coin_check
			ld  (__y), a
			ld  a, c
			ld  (__x), a
			ld  a, (__y)
			call qtile_do 			// C = x, A = y
			ld  a, l
			cp  RAMIRO_COIN_TILE
			ret nz

		// Ramiro 4's get_coin ()
		.rm_get_coin
			ld  hl, _flags + RAMIRO_COIN_FLAG
			inc (hl)

			// set_map_tile (_x, _y, 0, 0)
			xor a
			ld  (__t), a
			ld  (__n), a
			ld  a, (__x)
			ld  c, a
			ld  a, (__y)
			call set_map_tile_do

			ld  l, 5
			call _rm_sfx

			ld  hl, (_player + 29) 		// player.life
			ld  de, RAMIRO_COINS_REFILL
			add hl, de
			ld  (_player + 29), hl

		#ifdef ACTIVATE_SCRIPTING
			// The script hears about it: PLAYER GOT SOMETHING
			ld  hl, SC_PLAYER_GOT_SOMETHING
			call _script 			// __FASTCALL__
		#endif
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// player_custom_move: Ramiro 4's move ().
// The engine has already called pad_read (). After this the engine runs the
// scripting checks (special tiles) and player_custom_frame ().
// ----------------------------------------------------------------------------

void player_custom_move (void) {
	#asm
			// gpcx = player.x; gpcy = player.y;
			ld  hl, (_player)
			ld  (_gpcx), hl
			ld  hl, (_player + 2)
			ld  (_gpcy), hl

			// BOTH_KEYS_JUMP:
			// button_jump = (pad0 & (sp_UP|sp_FIRE)) != (sp_UP|sp_FIRE);
			ld  a, (_pad0)
			and sp_UP | sp_FIRE
			cp  sp_UP | sp_FIRE
			ld  a, 0
			jr  z, rm_bj_set
			inc a
		.rm_bj_set
			ld  (_rm_bj), a

		// ================================================= Vertical

			// RAMIRO_HOP + SHORT_PLAYER:
			// rdi = ((attr ((gpx + 4) >> 4, gpyy + 1) & 12) || (attr ((gpx + 11) >> 4, gpyy + 1) & 12));
			// (gpyy is the value computed by the previous move, as in Ramiro 4)
			ld  a, (_rm_gpyy)
			inc a
			ld  (_rm_t), a

			ld  a, (_gpx)
			add 4
			srl a
			srl a
			srl a
			srl a
			ld  c, a
			ld  a, (_rm_t)
			call rm_attr
			ld  a, l
			and 12
			jr  nz, rm_hop_yes

			ld  a, (_gpx)
			add 11
			srl a
			srl a
			srl a
			srl a
			ld  c, a
			ld  a, (_rm_t)
			call rm_attr
			ld  a, l
			and 12
			jr  z, rm_hop_set
		.rm_hop_yes
			ld  a, 1
		.rm_hop_set
			ld  (_rm_hop), a

			// if (button_jump) {
			ld  a, (_rm_bj)
			or  a
			jr  z, rm_jump_released

				// if (player.saltando == 0 && player.just_hovered == 0) {
				ld  a, (_player + 19) 		// saltando
				or  a
				jr  nz, rm_jump_cont
				ld  a, (_player + 42) 		// just_hovered
				or  a
				jr  nz, rm_jump_cont

					// if (rdi || player.gotten) {
					ld  a, (_rm_hop)
					ld  c, a
					ld  a, (_player + 25) 	// gotten
					or  c
					jr  z, rm_jump_cont

						ld  a, 1
						ld  (_player + 19), a 	// saltando = 1
						ld  (_player + 43), a 	// just_jumped = 1
						xor a
						ld  (_player + 14), a 	// cont_salto = 0
						ld  l, 1
						call _rm_sfx

			.rm_jump_cont
				// if (player.saltando) {
				ld  a, (_player + 19)
				or  a
				jr  z, rm_jump_done

					// player.vy -= (PLAYER_VY_INICIAL_SALTO + PLAYER_INCR_SALTO - (player.cont_salto >> 1));
					ld  a, (_player + 14)
					srl a
					ld  e, a
					ld  d, 0
					ld  hl, PLAYER_VY_INICIAL_SALTO + PLAYER_INCR_SALTO
					or  a
					sbc hl, de
					ex  de, hl
					ld  hl, (_player + 8)
					or  a
					sbc hl, de

					// if (player.vy < -PLAYER_MAX_VY_SALTANDO) player.vy = -PLAYER_MAX_VY_SALTANDO;
					push hl
					ld  de, PLAYER_MAX_VY_SALTANDO
					add hl, de
					bit 7, h
					pop hl
					jr  z, rm_jump_vy_ok
					ld  hl, -PLAYER_MAX_VY_SALTANDO
				.rm_jump_vy_ok
					ld  (_player + 8), hl

					// player.cont_salto ++; if (player.cont_salto == 8) player.saltando = 0;
					ld  a, (_player + 14)
					inc a
					ld  (_player + 14), a
					cp  8
					jr  nz, rm_jump_done
					xor a
					ld  (_player + 19), a
					jr  rm_jump_done

		.rm_jump_released
			// } else { player.saltando = 0; player.just_jumped = 0; }
			xor a
			ld  (_player + 19), a
			ld  (_player + 43), a
		.rm_jump_done

			// RAMIRO_HOVER
			// player.hovering = 0;
			xor a
			ld  (_player + 41), a

			// if ((pad0 & sp_DOWN) == 0) {
			ld  a, (_pad0)
			and sp_DOWN
			jr  nz, rm_hover_no

				// player.just_hovered = 1;
				ld  a, 1
				ld  (_player + 42), a

				// if (player.vy > 0) {
				ld  hl, (_player + 8)
				bit 7, h
				jr  nz, rm_hover_done
				ld  a, h
				or  l
				jr  z, rm_hover_done

					ld  a, 1
					ld  (_player + 41), a 	// hovering = 1

					ld  a, (_pad0)
					or  sp_DOWN
					ld  (_pad0), a

					ld  hl, (_player + 8) 		// player.vy
					ld  de, PLAYER_MAX_VY_CAYENDO_H - PLAYER_G_HOVER
					or  a
					push hl
					sbc hl, de
					pop hl
					jr  nc, rm_hover_maximum

					ld  de, PLAYER_G_HOVER
					add hl, de
					jr  rm_hover_set

				.rm_hover_maximum
					ld  hl, PLAYER_MAX_VY_CAYENDO_H

				.rm_hover_set
					ld  (_player + 8), hl
					jr  rm_hover_done

		.rm_hover_no
			// } else player.just_hovered = 0;
			xor a
			ld  (_player + 42), a

		.rm_hover_done
			// if (player.hovering == 0) gravity:
			ld  a, (_player + 41)
			or  a
			jr  nz, rm_gravity_done

			// Signed comparison shortcut (Ramiro 4)
			ld  hl, (_player + 8) 		// player.vy
			bit 7, h
			jr  nz, rm_gravity_add  	// < 0

			ld  de, PLAYER_MAX_VY_CAYENDO - PLAYER_G
			or  a
			push hl
			sbc hl, de
			pop hl
			jr  nc, rm_gravity_maximum

		.rm_gravity_add
			ld  de, PLAYER_G
			add hl, de
			jr  rm_gravity_vy_set

		.rm_gravity_maximum
			ld  hl, PLAYER_MAX_VY_CAYENDO

		.rm_gravity_vy_set
			ld  (_player + 8), hl

		.rm_gravity_done
			// if (player.gotten) player.vy = 0;
			ld  a, (_player + 25)
			or  a
			jr  z, rm_gotten_done
			ld  hl, 0
			ld  (_player + 8), hl
		.rm_gotten_done

			// player.y += player.vy;
			ld  hl, (_player + 2)
			ld  de, (_player + 8)
			add hl, de

			// BETTER_VERTICAL_CONNECTIONS: if (player.y < -512) player.y = -512;
			push hl
			ld  de, 512
			add hl, de
			bit 7, h
			pop hl
			jr  z, rm_y_min_ok
			ld  hl, -512
		.rm_y_min_ok
			// if (player.y > 9216) player.y = 9216;
			bit 7, h
			jr  nz, rm_y_max_ok
			push hl
			ld  de, 9217
			or  a
			sbc hl, de
			pop hl
			jr  c, rm_y_max_ok
			ld  hl, 9216
		.rm_y_max_ok
			ld  (_player + 2), hl

			// gpx = player.x >> 6; gpxx = gpx >> 4; gpy = player.y >> 6; gpyy = gpy >> 4;
			ld  hl, (_player + 0)
			call HLshr6_A
			ld  (_gpx), a
			srl a
			srl a
			srl a
			srl a
			ld  (_rm_gpxx), a

			ld  hl, (_player + 2)
			call HLshr6_A
			ld  (_gpy), a
			srl a
			srl a
			srl a
			srl a
			ld  (_rm_gpyy), a

		// Vertical collision (SHORT_PLAYER)

			xor a
			ld  (_player + 26), a 		// possee
			ld  (_player + 37), a 		// ceiling
			ld  (_wall), a

			ld  de, (_player + 8)
			ld  hl, (_ptgmy)
			add hl, de
			ld  (_rm_rdj), hl

			ld  a, h
			or  l
			jp  z, rm_vcoll_done

			bit 7, h
			jr  z, rm_vcoll_positive

		.rm_vcoll_negative
			// if ((gpy & 15) < 12)
			ld  a, (_gpy)
			and 15
			cp  12
			jp  nc, rm_vcoll_done

			// if (((gpx & 15) < 12 && attr (gpxx, gpyy) & 8) || ((gpx & 15) > 4 && attr (gpxx + 1, gpyy) & 8))
			ld  a, (_gpx)
			and 15
			cp  12
			jr  nc, rm_vcoll_if2

			ld  a, (_rm_gpxx)
			ld  c, a
			ld  a, (_rm_gpyy)
			call rm_attr
			ld  a, l
			and 8
			jr  nz, rm_vcoll_up

		.rm_vcoll_if2
			ld  a, (_gpx)
			and 15
			cp  5
			jp  c, rm_vcoll_done

			ld  a, (_rm_gpxx)
			inc a
			ld  c, a
			ld  a, (_rm_gpyy)
			call rm_attr
			ld  a, l
			and 8
			jp  z, rm_vcoll_done

		.rm_vcoll_up
			// player.vy = 0; gpy = (gpyy << 4) + 12; player.y = gpy << 6; player.ceiling = 1;
			ld  hl, 0
			ld  (_player + 8), hl

			ld  a, (_rm_gpyy)
			sla a
			sla a
			sla a
			sla a
			add 12
			ld  (_gpy), a
			call Ashl16_HL
			ld  (_player + 2), hl

			ld  a, 1
			ld  (_player + 37), a

			ld  a, RM_WALL_UP
			ld  (_wall), a
			jr  rm_vcoll_done

		.rm_vcoll_positive
			// else if ((gpy & 15) <= (player.vy >> 6))
			ld  a, (_gpy)
			and 15
			ld  c, a
			ld  hl, (_player + 8)
			call HLshr6_A
			cp  c
			jr  c, rm_vcoll_done

			// if (((gpx & 15) < 12 && (attr (gpxx, gpyy + 1) & 12)) || ((gpx & 15) > 4 && attr (gpxx + 1, gpyy + 1) & 12))
			ld  a, (_gpx)
			and 15
			cp  12
			jr  nc, rm_vcoll_if4

			ld  a, (_rm_gpxx)
			ld  c, a
			ld  a, (_rm_gpyy)
			inc a
			call rm_attr
			ld  a, l
			and 12
			jr  nz, rm_vcoll_down

		.rm_vcoll_if4
			ld  a, (_gpx)
			and 15
			cp  5
			jr  c, rm_vcoll_done

			ld  a, (_rm_gpxx)
			inc a
			ld  c, a
			ld  a, (_rm_gpyy)
			inc a
			call rm_attr
			ld  a, l
			and 12
			jr  z, rm_vcoll_done

		.rm_vcoll_down
			ld  hl, 0
			ld  (_player + 8), hl
			call rm_adjust_to_tile_y
			ld  a, 1
			ld  (_player + 26), a 		// possee

		.rm_vcoll_done

		// ================================================= Horizontal

			// if ((pad0 & sp_LEFT) != 0 && (pad0 & sp_RIGHT) != 0) friction
			ld  a, (_pad0)
			and sp_LEFT | sp_RIGHT
			cp  sp_LEFT | sp_RIGHT
			jr  nz, rm_hfric_done

			ld  hl, (_player + 6)
			ld  a, h
			or  l
			jr  z, rm_hfric_done
			bit 7, h
			jr  nz, rm_hfric_neg

			// if (player.vx > 0) { player.vx -= player.rx; if (player.vx < 0) player.vx = 0; }
			ld  de, PLAYER_RX
			or  a
			sbc hl, de
			bit 7, h
			jr  z, rm_hfric_set
			ld  hl, 0
			jr  rm_hfric_set

		.rm_hfric_neg
			// else { player.vx += player.rx; if (player.vx > 0) player.vx = 0; }
			ld  de, PLAYER_RX
			add hl, de
			bit 7, h
			jr  nz, rm_hfric_set
			ld  hl, 0

		.rm_hfric_set
			ld  (_player + 6), hl
		.rm_hfric_done

			// if ((pad0 & sp_LEFT) == 0) { if (player.vx > -player.max_vx) { facing = left; vx -= ax; } }
			ld  a, (_pad0)
			and sp_LEFT
			jr  nz, rm_hleft_done

			ld  hl, (_player + 6)
			ld  de, PLAYER_MAX_VX
			add hl, de
			bit 7, h
			jr  nz, rm_hleft_done
			ld  a, h
			or  l
			jr  z, rm_hleft_done

			ld  a, 4 					// facing left (MK1 v3.2 adds 4 to the frame)
			ld  (_player + 22), a
			ld  hl, (_player + 6)
			ld  de, PLAYER_AX
			or  a
			sbc hl, de
			ld  (_player + 6), hl
		.rm_hleft_done

			// if ((pad0 & sp_RIGHT) == 0) { if (player.vx < player.max_vx) { vx += ax; facing = right; } }
			ld  a, (_pad0)
			and sp_RIGHT
			jr  nz, rm_hright_done

			ld  hl, (_player + 6)
			ld  de, PLAYER_MAX_VX
			or  a
			sbc hl, de
			bit 7, h
			jr  z, rm_hright_done

			ld  hl, (_player + 6)
			ld  de, PLAYER_AX
			add hl, de
			ld  (_player + 6), hl
			xor a
			ld  (_player + 22), a
		.rm_hright_done

			// player.x += player.vx; player.x += ptgmx;
			ld  hl, (_player)
			ld  de, (_player + 6)
			add hl, de
			ld  de, (_ptgmx)
			add hl, de

			// if (player.x < 0) player.x = 0; if (player.x > 14336) player.x = 14336;
			bit 7, h
			jr  z, rm_x_pos
			ld  hl, 0
			jr  rm_x_set
		.rm_x_pos
			push hl
			ld  de, 14337
			or  a
			sbc hl, de
			pop hl
			jr  c, rm_x_set
			ld  hl, 14336
		.rm_x_set
			ld  (_player), hl

			call HLshr6_A
			ld  (_gpx), a
			srl a
			srl a
			srl a
			srl a
			ld  (_rm_gpxx), a

		// Horizontal collision (SHORT_PLAYER)

			ld  hl, (_player + 6)
			ld  de, (_ptgmx)
			add hl, de
			ld  (_rm_rdj), hl

			ld  a, h
			or  l
			jp  z, rm_hcoll_done

			bit 7, h
			jp  z, rm_hcoll_positive

		.rm_hcoll_negative
			// if ((gpx & 15) < 12)
			ld  a, (_gpx)
			and 15
			cp  12
			jp  nc, rm_hcoll_done

			// ((gpy & 15) < 12 && attr (gpxx, gpyy) & 8) || ((gpy & 15) && attr (gpxx, gpyy + 1) & 8)
			ld  a, (_gpy)
			and 15
			cp  12
			jp  nc, rm_hcoll_if2

			ld  a, (_rm_gpxx)
			ld  c, a
			ld  a, (_rm_gpyy)
			call rm_attr
			ld  a, l
			and 8
			jp  nz, rm_hcoll_left

		.rm_hcoll_if2
			ld  a, (_gpy)
			and 15
			jp  z, rm_hcoll_done

			ld  a, (_rm_gpxx)
			ld  c, a
			ld  a, (_rm_gpyy)
			inc a
			call rm_attr
			ld  a, l
			and 8
			jp  z, rm_hcoll_done

		.rm_hcoll_left
			// player.vx = 0; gpx = (gpxx << 4) + 12; player.x = gpx << 6; wall = WALL_LEFT;
			ld  hl, 0
			ld  (_player + 6), hl

			ld  a, (_rm_gpxx)
			sla a
			sla a
			sla a
			sla a
			add 12
			ld  (_gpx), a
			call Ashl16_HL
			ld  (_player), hl

			ld  a, RM_WALL_LEFT
			ld  (_wall), a
			jr  rm_hcoll_done

		.rm_hcoll_positive
			// if ((gpx & 15) >= 4)
			ld  a, (_gpx)
			and 15
			cp  4
			jp  c, rm_hcoll_done

			ld  a, (_gpy)
			and 15
			cp  12
			jp  nc, rm_hcoll_if4

			ld  a, (_rm_gpxx)
			inc a
			ld  c, a
			ld  a, (_rm_gpyy)
			call rm_attr
			ld  a, l
			and 8
			jp  nz, rm_hcoll_right

		.rm_hcoll_if4
			ld  a, (_gpy)
			and 15
			jp  z, rm_hcoll_done

			ld  a, (_rm_gpxx)
			inc a
			ld  c, a
			ld  a, (_rm_gpyy)
			inc a
			call rm_attr
			ld  a, l
			and 8
			jp  z, rm_hcoll_done

		.rm_hcoll_right
			// player.vx = 0; gpx = (gpxx << 4) + 4; player.x = gpx << 6; wall = WALL_RIGHT;
			ld  hl, 0
			ld  (_player + 6), hl

			ld  a, (_rm_gpxx)
			sla a
			sla a
			sla a
			sla a
			add 4
			ld  (_gpx), a
			call Ashl16_HL
			ld  (_player), hl

			ld  a, RM_WALL_RIGHT
			ld  (_wall), a

		.rm_hcoll_done

		// ================================================= Evil tile (EVIL_TILE_SIMPLE)

			// if (attr ((gpx + 8) >> 4, (gpy + 14) >> 4) == 1)
			ld  a, (_gpx)
			add 8
			srl a
			srl a
			srl a
			srl a
			ld  c, a
			ld  a, (_gpy)
			add 14
			srl a
			srl a
			srl a
			srl a
			call rm_attr
			ld  a, l
			dec a
			jr  nz, rm_evil_tile_done

			// if (player.estado == EST_NORMAL) { sfx 2; life -= LINEAR_ENEMY_HIT; flicker }
			ld  a, (_player + 23)
			or  a
			jr  nz, rm_evil_tile_nohit

			ld  l, 2
			call _rm_sfx

			ld  hl, (_player + 29)
			ld  de, RAMIRO_LINEAR_HIT
			or  a
			sbc hl, de
			ld  (_player + 29), hl

			call _player_flicker

		.rm_evil_tile_nohit
			// player.x = gpcx; player.y = gpcy;
			ld  hl, (_gpcx)
			ld  (_player), hl
			ld  hl, (_gpcy)
			ld  (_player + 2), hl

			// player.vy = -abs (player.vy);
			ld  hl, (_player + 8)
			bit 7, h
			jr  nz, rm_evil_tile_vyneg
			ex  de, hl
			ld  hl, 0
			or  a
			sbc hl, de
		.rm_evil_tile_vyneg
			// if (player.vy > -(PLAYER_G*9)) player.vy = -(PLAYER_G*9);
			push hl
			ld  de, PLAYER_G * 9
			add hl, de
			bit 7, h
			jr  nz, rm_evil_tile_vykeep
			ld  a, h
			or  l
			jr  z, rm_evil_tile_vykeep
			pop hl
			ld  hl, -(PLAYER_G * 9)
			jr  rm_evil_tile_vyset
		.rm_evil_tile_vykeep
			pop hl
		.rm_evil_tile_vyset
			ld  (_player + 8), hl

		.rm_evil_tile_done

		// ================================================= Evil zone (EVIL_ZONE_CONDITIONAL)

			// if (attr ((gpx + 4) >> 4, (gpy + 4) >> 4) == 3 && evil_zone_active)
			ld  a, (_gpx)
			add 4
			srl a
			srl a
			srl a
			srl a
			ld  c, a
			ld  a, (_gpy)
			add 4
			srl a
			srl a
			srl a
			srl a
			call rm_attr
			ld  a, l
			cp  3
			jp  nz, rm_ez_out

		#ifdef RAMIRO_EZ_ACTIVE_FLAG
			ld  a, (_flags + RAMIRO_EZ_ACTIVE_FLAG)
			or  a
			jp  z, rm_ez_out
		#endif

			// if (beepcount >= EVIL_ZONE_BEEPS_COUNT || 0 == evil_kills_slowly)
			ld  a, (_player + 35) 		// killingzone_beepcount
			cp  EVIL_ZONE_BEEPS_COUNT
			jr  nc, rm_ez_drain
			ld  a, (_flags + RAMIRO_EZ_SLOW_FLAG)
			or  a
			jr  nz, rm_ez_countdown

		.rm_ez_drain
			// if (0 == (player.estado & EST_PARP) && 0 == (maincounter & EVIL_ZONE_FREQ))
			ld  a, (_player + 23)
			and EST_PARP
			jp  nz, rm_ez_done
			ld  a, (_maincounter)
			and EVIL_ZONE_FREQ
			jp  nz, rm_ez_done

			// framecount = (framecount + 1) & 3; if (0 == framecount) sfx 3;
			ld  a, (_player + 34)
			inc a
			and 3
			ld  (_player + 34), a
			jr  nz, rm_ez_drain_nosfx
			ld  l, 3
			call _rm_sfx
		.rm_ez_drain_nosfx
			// player.life --;
			ld  hl, (_player + 29)
			dec hl
			ld  (_player + 29), hl
			jr  rm_ez_done

		.rm_ez_countdown
			// if (framecount > EVIL_ZONE_FRAME_COUNT) { framecount = 0; beepcount ++; border 2; sfx 4; halt; border 0 }
			ld  a, (_player + 34)
			cp  EVIL_ZONE_FRAME_COUNT + 1
			jr  c, rm_ez_countdown_inc

			xor a
			ld  (_player + 34), a
			ld  hl, _player + 35
			inc (hl)
		#ifndef CPC
			ld  a, 2
			out (254), a
		#endif
			ld  l, 4
			call _rm_sfx
			halt
		#ifndef CPC
			xor a
			out (254), a
		#endif
			jr  rm_ez_done

		.rm_ez_countdown_inc
			inc a
			ld  (_player + 34), a
			jr  rm_ez_done

		.rm_ez_out
			// else if (player.killingzone_beepcount > 0) player.killingzone_beepcount --;
			ld  a, (_player + 35)
			or  a
			jr  z, rm_ez_done
			dec a
			ld  (_player + 35), a

		.rm_ez_done

		// ================================================= Coins

		#ifdef RAMIRO_COIN_TILE
			// (gpxx, gpyy)
			ld  a, (_rm_gpyy)
			ld  (_rm_t), a
			ld  a, (_rm_gpxx)
			ld  c, a
			ld  a, (_rm_t)
			call rm_coin_check

			// if (gpx & 15) (gpxx + 1, gpyy)
			ld  a, (_gpx)
			and 15
			jr  z, rm_coin_b_done
			ld  a, (_rm_gpyy)
			ld  (_rm_t), a
			ld  a, (_rm_gpxx)
			inc a
			ld  c, a
			ld  a, (_rm_t)
			call rm_coin_check
		.rm_coin_b_done

			// if (gpy & 15) (gpxx, gpyy + 1)
			ld  a, (_gpy)
			and 15
			jr  z, rm_coin_c_done
			ld  a, (_rm_gpyy)
			inc a
			ld  (_rm_t), a
			ld  a, (_rm_gpxx)
			ld  c, a
			ld  a, (_rm_t)
			call rm_coin_check
		.rm_coin_c_done

			// if ((gpx & 15) && (gpy & 15)) (gpxx + 1, gpyy + 1)
			ld  a, (_gpx)
			and 15
			jr  z, rm_coin_d_done
			ld  a, (_gpy)
			and 15
			jr  z, rm_coin_d_done
			ld  a, (_rm_gpyy)
			inc a
			ld  (_rm_t), a
			ld  a, (_rm_gpxx)
			inc a
			ld  c, a
			ld  a, (_rm_t)
			call rm_coin_check
		.rm_coin_d_done
		#endif

	#endasm
}

// ----------------------------------------------------------------------------
// Ramiro 4 custom animation (custom_animation.h):
//   cells: R1 R2 R3 RJ (RH) L1 L2 L3 LJ (LH)
//   in the air: jump (or hover) cell; still: cell 2; walking: 1 2 3 2
// MK1 v3.2 has 8 player cells; the hover cells RH/LH show the jump cell.
// ----------------------------------------------------------------------------

#ifdef PLAYER_CUSTOM_FRAME
	unsigned char player_custom_frame (void) {
		#asm
				// if (0 == player.possee && 0 == player.gotten) rdd = 3 (+ hovering)
				ld  a, (_player + 26)
				ld  c, a
				ld  a, (_player + 25)
				or  c
				ld  c, 3
				jr  z, rm_frame_set

				// else if (player.vx == 0) rdd = 1;
				ld  hl, (_player + 6)
				ld  a, h
				or  l
				ld  c, 1
				jr  z, rm_frame_set

				// else { rdd = ((gpx + 4) >> 3) & 3; if (rdd == 3) rdd = 1; }
				ld  a, (_gpx)
				add 4
				srl a
				srl a
				srl a
				and 3
				cp  3
				jr  nz, rm_frame_walk
				ld  a, 1
			.rm_frame_walk
				ld  c, a

			.rm_frame_set
				ld  a, (_player + 22) 		// facing: 0 or 4
				add c
				ld  h, 0
				ld  l, a
		#endasm
	}
#endif

// ----------------------------------------------------------------------------
// Custom flick: the upward connection is done by player_custom_move () when
// player.y reaches -512, as in Ramiro 4. The other three are the engine's.
// ----------------------------------------------------------------------------

#ifdef ENABLE_CUSTOM_FLICK
	unsigned char flick_up (void) {
		#asm
				ld  hl, 0
		#endasm
	}
	unsigned char flick_down (void) {
		#asm
				ld  hl, 1
		#endasm
	}
	unsigned char flick_left (void) {
		#asm
				ld  hl, 1
		#endasm
	}
	unsigned char flick_right (void) {
		#asm
				ld  hl, 1
		#endasm
	}
#endif

// ----------------------------------------------------------------------------
// Enemies (ENABLE_CUSTOM_ENEMS)
//   Type 6: Ramiro 4 flying enemies. They always pursue the player, and
//   move only while flags [RAMIRO_T6_FLAG] != 0. With RAMIRO_MAKE_T6_FLAG
//   defined, empty slots become bats (internally type 7) while that flag
//   is set (Ramiro 2). Their start position is
//   (x1, y1) of the enemy record (the converter sets x1 = x, y1 = y).
//   Damage: RAMIRO_LINEAR_HIT for types 1-3, RAMIRO_FLYING_HIT for type 6,
//   with Ramiro 4's narrower (8 pixels wide) collision box.
// ----------------------------------------------------------------------------

#ifdef ENABLE_CUSTOM_ENEMS
	void extra_enems_init (void) {
		#asm
				ld  a, (__en_t)

			#ifdef RAMIRO_MAKE_T6_FLAG
				// Ramiro 2's "make type 6": while flags [RAMIRO_MAKE_T6_FLAG]
				// != 0, empty enemy slots become bats at a random tile. They
				// are marked as type 7 so they go back to empty if the flag
				// is cleared (new game).
				cp  7
				jr  nz, rm_mt6_not7
				ld  a, (_flags + RAMIRO_MAKE_T6_FLAG)
				or  a
				jr  nz, rm_mt6_make
				ld  (__en_t), a 			// back to empty
				ret
			.rm_mt6_not7
				or  a
				jr  nz, rm_mt6_done
				ld  a, (_flags + RAMIRO_MAKE_T6_FLAG)
				or  a
				ret z
			.rm_mt6_make
				ld  a, 7
				ld  (__en_t), a

				// x1 = (rand () % 15) << 4; y1 = (rand () % 10) << 4;
				call _rand
				ld  a, l
			.rm_mt6_mod15
				cp  15
				jr  c, rm_mt6_mod15_done
				sub 15
				jr  rm_mt6_mod15
			.rm_mt6_mod15_done
				add a, a
				add a, a
				add a, a
				add a, a
				ld  (__en_x1), a

				call _rand
				ld  a, l
			.rm_mt6_mod10
				cp  10
				jr  c, rm_mt6_mod10_done
				sub 10
				jr  rm_mt6_mod10
			.rm_mt6_mod10_done
				add a, a
				add a, a
				add a, a
				add a, a
				ld  (__en_y1), a
				jr  rm_t6_init

			.rm_mt6_done
				ld  a, (__en_t)
			#endif

				cp  6
				ret nz

			.rm_t6_init

				// en_an_x [enit] = x1 << 6; en_an_y [enit] = y1 << 6; vx = vy = 0
				ld  a, (_enit)
				sla a
				ld  c, a
				ld  b, 0

				ld  a, (__en_x1)
				ld  (__en_x), a
				push bc
				call Ashl16_HL
				pop bc
				ex  de, hl
				ld  hl, _en_an_x
				add hl, bc
				ld  (hl), e
				inc hl
				ld  (hl), d

				ld  a, (__en_y1)
				ld  (__en_y), a
				push bc
				call Ashl16_HL
				pop bc
				ex  de, hl
				ld  hl, _en_an_y
				add hl, bc
				ld  (hl), e
				inc hl
				ld  (hl), d

				ld  hl, _en_an_vx
				add hl, bc
				xor a
				ld  (hl), a
				inc hl
				ld  (hl), a
				ld  hl, _en_an_vy
				add hl, bc
				ld  (hl), a
				inc hl
				ld  (hl), a

				// enems_en_an_calc (2): Ramiro 4 type 6 uses the 3rd enemy sprite
				ld  hl, 2
				jp  _enems_en_an_calc
		#endasm
	}

	void extra_enems_move (void) {
		#asm
				ld  a, (__en_t)
				cp  6
				jr  z, rm_t6_go
			#ifdef RAMIRO_MAKE_T6_FLAG
				cp  7
				ret nz
			#else
				ret
			#endif
			.rm_t6_go

				// if ((rand () & 7) > 1) { pursue }
				call _rand
				ld  a, l
				and 7
				cp  2
				jp  c, rm_t6_move

				ld  a, (_enit)
				sla a
				ld  c, a
				ld  b, 0

				// ---- X
				// if (player.x > en_an_x [enit] && en_an_vx [enit] < FANTY_MAX_V) vx += A
				// else if (player.x < en_an_x [enit] && en_an_vx [enit] > -FANTY_MAX_V) vx -= A
				ld  hl, _en_an_x
				add hl, bc
				ld  e, (hl)
				inc hl
				ld  d, (hl) 				// DE = en_an_x
				ld  hl, (_player)
				call rm_cmp_signed 			// HL - DE : Z if equal, C if HL < DE
				jr  z, rm_t6_x_done
				push af 					// (add hl, bc trashes the carry)
				ld  hl, _en_an_vx
				add hl, bc
				pop af
				jr  c, rm_t6_x_dec

			.rm_t6_x_inc
				push hl
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, FANTY_MAX_V - 1
				ex  de, hl 					// HL = vx, DE = MAX-1
				call rm_cmp_signed 			// vx < MAX <=> vx <= MAX-1
				jr  z, rm_t6_x_inc_do
				jr  nc, rm_t6_x_inc_skip
			.rm_t6_x_inc_do
				ld  de, FANTY_A
				add hl, de
				ex  de, hl
				pop hl
				ld  (hl), e
				inc hl
				ld  (hl), d
				jr  rm_t6_x_done
			.rm_t6_x_inc_skip
				pop hl
				jr  rm_t6_x_done

			.rm_t6_x_dec
				push hl
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, -FANTY_MAX_V + 1
				ex  de, hl 					// HL = vx, DE = -MAX+1
				call rm_cmp_signed 			// vx > -MAX <=> vx >= -MAX+1
				jr  c, rm_t6_x_dec_skip
				ld  de, FANTY_A
				or  a
				sbc hl, de
				ex  de, hl
				pop hl
				ld  (hl), e
				inc hl
				ld  (hl), d
				jr  rm_t6_x_done
			.rm_t6_x_dec_skip
				pop hl
			.rm_t6_x_done

				// ---- Y
				ld  hl, _en_an_y
				add hl, bc
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, (_player + 2)
				call rm_cmp_signed 			// HL - DE : Z if equal, C if HL < DE
				jr  z, rm_t6_y_done
				push af 					// (add hl, bc trashes the carry)
				ld  hl, _en_an_vy
				add hl, bc
				pop af
				jr  c, rm_t6_y_dec

			.rm_t6_y_inc
				push hl
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, FANTY_MAX_V - 1
				ex  de, hl
				call rm_cmp_signed
				jr  z, rm_t6_y_inc_do
				jr  nc, rm_t6_y_inc_skip
			.rm_t6_y_inc_do
				ld  de, FANTY_A
				add hl, de
				ex  de, hl
				pop hl
				ld  (hl), e
				inc hl
				ld  (hl), d
				jr  rm_t6_y_done
			.rm_t6_y_inc_skip
				pop hl
				jr  rm_t6_y_done

			.rm_t6_y_dec
				push hl
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, -FANTY_MAX_V + 1
				ex  de, hl
				call rm_cmp_signed
				jr  c, rm_t6_y_dec_skip
				ld  de, FANTY_A
				or  a
				sbc hl, de
				ex  de, hl
				pop hl
				ld  (hl), e
				inc hl
				ld  (hl), d
				jr  rm_t6_y_done
			.rm_t6_y_dec_skip
				pop hl
			.rm_t6_y_done

			.rm_t6_move
				ld  a, (_enit)
				sla a
				ld  c, a
				ld  b, 0

				// if (allow_type_6) { x += vx; y += vy; }
				ld  a, (_flags + RAMIRO_T6_FLAG)
				or  a
				jr  z, rm_t6_clamp

				ld  hl, _en_an_vx
				add hl, bc
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, _en_an_x
				add hl, bc
				ld  a, (hl)
				add a, e
				ld  (hl), a
				inc hl
				ld  a, (hl)
				adc a, d
				ld  (hl), a

				ld  hl, _en_an_vy
				add hl, bc
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, _en_an_y
				add hl, bc
				ld  a, (hl)
				add a, e
				ld  (hl), a
				inc hl
				ld  a, (hl)
				adc a, d
				ld  (hl), a

			.rm_t6_clamp
				// x in [-1024, 15360], y in [-1024, 10240]
				ld  hl, _en_an_x
				add hl, bc
				push hl
				ld  de, 15360
				call rm_clamp
				pop hl
				ld  hl, _en_an_y
				add hl, bc
				push hl
				ld  de, 10240
				call rm_clamp
				pop hl

				// Collision/render coordinates: _en_x = en_an_x >> 6; _en_y = en_an_y >> 6
				ld  hl, _en_an_x
				add hl, bc
				ld  a, (hl)
				inc hl
				ld  h, (hl)
				ld  l, a
				call HLshr6_A
				ld  (__en_x), a

				ld  hl, _en_an_y
				add hl, bc
				ld  a, (hl)
				inc hl
				ld  h, (hl)
				ld  l, a
				call HLshr6_A
				ld  (__en_y), a

				ret

			// HL - DE (signed): Z if equal, C if HL < DE. Keeps HL, DE, BC.
			.rm_cmp_signed
				push hl
				or  a
				sbc hl, de
				jr  z, rm_cmp_signed_eq
				jp  pe, rm_cmp_signed_ov
				// no overflow: sign of result
				bit 7, h
				jr  nz, rm_cmp_signed_lt
				jr  rm_cmp_signed_gt
			.rm_cmp_signed_ov
				bit 7, h
				jr  z, rm_cmp_signed_lt
			.rm_cmp_signed_gt
				pop hl
				or  1 			// NZ, NC
				ret
			.rm_cmp_signed_lt
				pop hl
				ld  a, 1
				or  a 			// NZ
				scf 			// C
				ret
			.rm_cmp_signed_eq
				pop hl
				xor a 			// Z, NC
				ret

			// (HL) is a signed int; clamp it to [-1024, DE]. Keeps BC.
			.rm_clamp
				push hl
				ld  a, (hl)
				inc hl
				ld  h, (hl)
				ld  l, a 			// HL = value
				call rm_cmp_signed 	// value - max
				jr  z, rm_clamp_min
				jr  c, rm_clamp_min
				ex  de, hl 			// HL = max
				jr  rm_clamp_write
			.rm_clamp_min
				ld  de, -1024
				call rm_cmp_signed
				jr  nc, rm_clamp_write
				ld  hl, -1024
			.rm_clamp_write
				ex  de, hl
				pop hl
				ld  (hl), e
				inc hl
				ld  (hl), d
				ret
		#endasm
	}

	void extra_enems_checks (void) {
	}

	void extra_enems_killed (void) {
	}

	unsigned char this_enemy_kills (void) {
		// Called by the engine when the 12 pixel collision fires. We do
		// Ramiro 4's narrower check and its damage here, and return 0 so the
		// engine doesn't apply its own.
		#asm
				// SHORT_PLAYER collision is 8 pixels wide:
				// _en_x + 8 >= gpx && gpx + 8 >= _en_x
				ld  a, (_gpx)
				ld  c, a
				ld  a, (__en_x)
				add 8
				cp  c
				jr  c, rm_tek_done

				ld  a, (__en_x)
				ld  c, a
				ld  a, (_gpx)
				add 8
				cp  c
				jr  c, rm_tek_done

				// if (player.estado == EST_NORMAL)
				ld  a, (_player + 23)
				or  a
				jr  nz, rm_tek_done

				ld  a, 1
				ld  (_en_tocado), a

				ld  l, 2
				call _rm_sfx

				// _en_t > 4 ? FLYING_ENEMY_HIT : LINEAR_ENEMY_HIT
				ld  de, RAMIRO_LINEAR_HIT
				ld  a, (__en_t)
				cp  5
				jr  c, rm_tek_hit
				ld  de, RAMIRO_FLYING_HIT
			.rm_tek_hit
				ld  hl, (_player + 29)
				or  a
				sbc hl, de
				ld  (_player + 29), hl

				call _player_flicker

			.rm_tek_done
				ld  hl, 0
		#endasm
	}
#endif

// ----------------------------------------------------------------------------
// Per frame: fire zone, Ramiro 4's upward screen connection and the evil
// zone gauge. Call from before_vsync ().
// ----------------------------------------------------------------------------

void ramiro_before_vsync (void) {
	#asm
			// Fire zone: if the player is inside, run PRESS FIRE AT SCREEN n
			ld  a, (_rm_fz_on)
			or  a
			jr  z, rm_bv_fz_done

			ld  a, (_rm_fzx1)
			ld  c, a
			ld  a, (_gpx)
			cp  c
			jr  c, rm_bv_fz_done

			ld  a, (_gpx)
			ld  c, a
			ld  a, (_rm_fzx2)
			cp  c
			jr  c, rm_bv_fz_done

			ld  a, (_rm_fzy1)
			ld  c, a
			ld  a, (_gpy)
			cp  c
			jr  c, rm_bv_fz_done

			ld  a, (_gpy)
			ld  c, a
			ld  a, (_rm_fzy2)
			cp  c
			jr  c, rm_bv_fz_done

		#ifdef ACTIVATE_SCRIPTING
			ld  a, (_n_pant)
			add a, a
			add a, SC_PRESS_FIRE_AT_SCREEN
			ld  l, a
			ld  h, 0
			call _script 			// __FASTCALL__
		#endif

		.rm_bv_fz_done

		// Flick up (BETTER_VERTICAL_CONNECTIONS). Ramiro 4 does this in the
		// main loop, after the enemies and before the other flicks.

			// if (player.y == -512 && player.vy < 0 && n_pant >= MAP_W)
			ld  hl, (_player + 2)
			ld  de, -512
			or  a
			sbc hl, de
			jr  nz, rm_flick_up_done

			ld  a, (_player + 9) 		// vy MSB
			bit 7, a
			jr  z, rm_flick_up_done

			ld  a, (_n_pant)
			cp  MAP_W
			jr  c, rm_flick_up_done

			// player.vy = -PLAYER_MAX_VY_SALTANDO; player.cont_salto = 0;
			ld  hl, -PLAYER_MAX_VY_SALTANDO
			ld  (_player + 8), hl
			xor a
			ld  (_player + 14), a

			// n_pant -= MAP_W; gpy = 144; player.y = 144 << 6;
			ld  a, (_n_pant)
			sub MAP_W
			ld  (_n_pant), a
			ld  a, 144
			ld  (_gpy), a
			ld  hl, 144*64
			ld  (_player + 2), hl

		.rm_flick_up_done

		#ifdef RAMIRO_EZ_GAUGE_X
			// Evil zone countdown: EVIL_ZONE_BEEPS_COUNT - beepcount
			ld  a, (_player + 35)
			ld  c, a
			ld  a, EVIL_ZONE_BEEPS_COUNT
			sub c
			jr  nc, rm_bv_ezg_pos
			xor a
		.rm_bv_ezg_pos
			ld  c, a
			ld  a, (_rm_ezg_old)
			cp  c
			jr  z, rm_bv_ezg_done
			ld  a, c
			ld  (_rm_ezg_old), a

			ld  hl, RAMIRO_EZ_GAUGE_X
			push hl
			ld  hl, RAMIRO_EZ_GAUGE_Y
			push hl
			ld  l, a
			ld  h, 0
			push hl
			call _draw_2_digits
			pop hl
			pop hl
			pop hl
		.rm_bv_ezg_done
		#endif
	#endasm
}

// ----------------------------------------------------------------------------
// EXTERN handler. Returns 1 in L if n was handled.
// ----------------------------------------------------------------------------

unsigned char ramiro_extern (unsigned char n, unsigned char m) {
	#asm
			ld  hl, 2
			add hl, sp
			ld  e, (hl) 			// m
			inc hl
			inc hl
			ld  a, (hl) 			// n

			cp  9
			jr  nz, rm_ext_10
			ld  l, e
			call _rm_sfx
			jr  rm_ext_yes

		.rm_ext_10
			cp  10
			jr  nz, rm_ext_11
			ld  a, e
			ld  (_rm_fzx1), a
			jr  rm_ext_yes

		.rm_ext_11
			cp  11
			jr  nz, rm_ext_12
			ld  a, e
			ld  (_rm_fzy1), a
			jr  rm_ext_yes

		.rm_ext_12
			cp  12
			jr  nz, rm_ext_13
			ld  a, e
			ld  (_rm_fzx2), a
			jr  rm_ext_yes

		.rm_ext_13
			cp  13
			jr  nz, rm_ext_14
			ld  a, e
			ld  (_rm_fzy2), a
			ld  a, 1
			ld  (_rm_fz_on), a
			jr  rm_ext_yes

		.rm_ext_14
			cp  14
			jr  nz, rm_ext_16
			xor a
			ld  (_rm_fz_on), a
			jr  rm_ext_yes

		.rm_ext_16
			cp  16
			jr  nz, rm_ext_no
			call _ramiro_redraw
			jr  rm_ext_yes

		.rm_ext_no
			ld  hl, 0
			ret

		.rm_ext_yes
			ld  hl, 1
	#endasm
}

// ----------------------------------------------------------------------------
// Repaint the play area from map_buff (REDRAW, and after text boxes)
// ----------------------------------------------------------------------------

void ramiro_redraw (void) {
	#asm
			ld  a, VIEWPORT_X
			ld  (__x), a
			ld  a, VIEWPORT_Y
			ld  (__y), a

			xor a
		.rm_redraw_loop
			ld  (_rm_t), a

			ld  c, a
			ld  b, 0
			ld  hl, _map_buff
			add hl, bc
			ld  a, (hl)
			ld  (__t), a

			call _draw_coloured_tile_do

			ld  a, (__x)
			add a, 2
			cp  VIEWPORT_X + 30
			jr  nz, rm_redraw_set_x
			ld  a, (__y)
			add a, 2
			ld  (__y), a
			ld  a, VIEWPORT_X
		.rm_redraw_set_x
			ld  (__x), a

			ld  a, (_rm_t)
			inc a
			cp  150
			jr  nz, rm_redraw_loop

			// Hotspot (object, key...) on top, as Ramiro 4's hotspot_paint
			jp  _hotspot_paint
	#endasm
}

// ----------------------------------------------------------------------------
// Text box for msc4's TEXT BOX: MK1 v3.2's Sir Ababol / Papaya text box
// (games/ababol/dev/extern.h), ported as is, with two Ramiro additions:
//
//   * The frame uses the Ramiro font's own frame glyphs (the 2016 games'),
//     and the speaker's name goes in the tab on top, like in 2016.
//   * The screen is updated (not just validated) before the box is drawn,
//     so boxes shown while entering a screen have the screen behind them.
//   * Room names: a text starting with '0' is not a box but the room name,
//     centered on the HUD line RAMIRO_RN_Y (columns 1-30, RAMIRO_RN_ATTR).
//
// The first character of a dialog may be '1'..'9': speaker N, whose name is
// ramiro_speakers [N-1] (an array of strings the game defines in extern.h).
//
// As in Ababol: the box (x = 4, 24 wide, 22 characters per line, so
// TEXT WIDTH = 22) grows line by line from y = 6 while the text is typed
// character by character; then a beep, wait for a key (pad), and the play
// area is repainted.
//
// The font has , . ? ! and space in slots 91-95 (where msc4's decoder puts
// them) and ¡ ¿ Ñ in the < > = slots, so no translation is done here.
//
// Frame glyphs (Ramiro font):  " #  tab ends   $ top   % top right corner
//                              & left   ' right   ( ) * bottom
// ----------------------------------------------------------------------------

#ifndef RAMIRO_TB_ATTR
	#define RAMIRO_TB_ATTR 			71 		// ZX: frame and text, bright white on black
#endif
#ifndef RAMIRO_TB_SPEAKER_ATTR
	#define RAMIRO_TB_SPEAKER_ATTR 	23 		// ZX: speaker's name (2016: white on red)
#endif
#ifndef RAMIRO_TB_TAB_ATTR
	#define RAMIRO_TB_TAB_ATTR 		2 		// ZX: the tab's rounded ends (2016: red on black)
#endif
#ifndef RAMIRO_RN_Y
	#define RAMIRO_RN_Y 			22
#endif
#ifndef RAMIRO_RN_ATTR
	#define RAMIRO_RN_ATTR 			23
#endif

unsigned char rm_top_string []    = "&                      %";
unsigned char rm_mid_string []    = "&                      '";
unsigned char rm_bottom_string [] = "())))))))))))))))))))))*";
unsigned char rm_temp_string []   = "                              ";	// 30 (room names)
unsigned char *rm_speaker;

#ifndef CPC
	void rm_sprite_remove_aid (void) {
		#asm
				call _saca_a_todo_el_mundo_de_aqui

				// Ababol validates the whole screen here so sprites stay
				// away on the next update. Ramiro shows text boxes also from
				// ENTERING scripts, when the new screen is still pending
				// (invalidated, not on screen yet), so we update instead:
				// same result, and the screen gets drawn.
				jp  SPUpdateNow
		#endasm
	}
#else
	void rm_print_tile_inv (void) {
		#asm
				// Tile address in DE
				call __tile_address

				// Write the tile
				ld  a, (__n)
				ld  (de), a

				// Invalidate tile E, D
				ld  a, (__x)
				ld  e, a
				ld  a, (__y)
				ld  d, a
				jp  cpc_UpdTileTable
		#endasm
	}
#endif

void rm_draw_text_cbc (void) {
	// Ababol's draw_text_cbc: text in HL, 0 terminated, at (rdx, rdy),
	// one character every two frames.
	#ifdef CPC
		#asm
			.rm_dtcbc_loop
				ld  a, (_rdx)
				ld  (__x), a
				ld  c, a
				inc a
				ld  (_rdx), a

				ld  a, (hl)
				or  a
				ret z 						// End of line
				inc hl
				sub 32
				jr  z, rm_dtcbc_loop 		// Skip spaces.

				halt
				halt

				push hl

				// Draw inv tile A @ rdx, rdy
				ld  (__n), a
				ld  a, (_rdy)
				ld  (__y), a
				call _rm_print_tile_inv

				ld  hl, 0
				push hl
				call _cpc_UpdateNow
				pop hl

				pop hl
				jr  rm_dtcbc_loop
		#endasm
	#else
		#asm
			.rm_dtcbc_loop
				ld  a, (_rdx)
				ld  c, a
				inc a
				ld  (_rdx), a
				ld  a, RAMIRO_TB_ATTR
				ld  d, a
				ld  a, (hl)
				or  a
				ret z
				inc hl
				sub 32
				halt
				halt

				jr  z, rm_dtcbc_loop 		// Skip spaces.

				ld  e, a
				ld  a, (_rdy)

				push hl
				call SPPrintAtInv
				call SPUpdateNow
				pop hl

				jr  rm_dtcbc_loop
		#endasm
	#endif
}

void rm_draw_frame_line (void) {
	// HL = string, (__x), (__y) set. Ababol draws frame lines with draw_text.
	#ifdef CPC
		#asm
				jp  draw_text_pre_loop
		#endasm
	#else
		#asm
				ld  a, RAMIRO_TB_ATTR
				ld  (__n), a
				jp  draw_text_loop
		#endasm
	#endif
}

void rm_room_name (void) {
	// HL -> name (after the '0'). msc4 wraps at TEXT WIDTH with '%': those
	// are spaces here. Centered in columns 1-30 of line RAMIRO_RN_Y.
	#asm
			push hl

			// Clear the buffer
			ld  hl, _rm_temp_string
			ld  de, _rm_temp_string + 1
			ld  bc, 29
			ld  (hl), 32
			ldir

			// Length (max 30)
			pop hl
			push hl
			ld  b, 0
		.rm_rn_len
			ld  a, (hl)
			or  a
			jr  z, rm_rn_len_done
			inc hl
			inc b
			ld  a, b
			cp  30
			jr  c, rm_rn_len
		.rm_rn_len_done

			// DE = rm_temp_string + (30 - length) / 2
			ld  a, 30
			sub b
			srl a
			ld  e, a
			ld  d, 0
			ld  hl, _rm_temp_string
			add hl, de
			ex  de, hl

			// Copy, '%' -> ' '
			pop hl
			inc b
		.rm_rn_copy
			dec b
			jr  z, rm_rn_print
			ld  a, (hl)
			inc hl
			cp  '%'
			jr  nz, rm_rn_copy_store
			ld  a, ' '
		.rm_rn_copy_store
			ld  (de), a
			inc de
			jr  rm_rn_copy

		.rm_rn_print
			ld  a, 1
			ld  (__x), a
			ld  (__t), a
			ld  a, RAMIRO_RN_Y
			ld  (__y), a
			ld  hl, _rm_temp_string
		#ifdef CPC
			jp  draw_text_pre_loop
		#else
			ld  a, RAMIRO_RN_ATTR
			ld  (__n), a
			jp  draw_text_loop
		#endif
	#endasm
}

void __FASTCALL__ ramiro_textbox (unsigned char *ptr) {
	#asm
			// Room name?
			ld  a, (hl)
			cp  '0'
			jr  nz, rm_tb_dialog
			inc hl
			jp  _rm_room_name

		.rm_tb_dialog
			// Speaker?
			ld  de, 0
			sub '1'
			cp  9
			jr  nc, rm_tb_nospeaker
			inc hl
			add a, a
			ld  e, a
			push hl
			ld  hl, _ramiro_speakers
			add hl, de
			ld  e, (hl)
			inc hl
			ld  d, (hl)
			pop hl
		.rm_tb_nospeaker
			ld  (_gp_gen), hl
			ld  (_rm_speaker), de
	#endasm

	#ifndef CPC
		rm_sprite_remove_aid ();
	#endif

	#asm
			// Speaker's tab, above the frame, and the top line of the frame
			// under it with a gap where the tab is (as in 2016):
			//   y = 5:   "NAME#
			//   y = 6:   &      $$$$$$$$$$$$$%
			ld  hl, _rm_top_string + 1
			ld  de, _rm_top_string + 2
			ld  bc, 21
			ld  (hl), '$'
			ldir

			ld  hl, (_rm_speaker)
			ld  a, h
			or  l
			jr  z, rm_tb_top

			// Tab
			ld  a, 4
			ld  (__x), a
			ld  a, 5
			ld  (__y), a
			push hl
			ld  hl, rm_tb_tab_l
			call rm_draw_tab_end
			pop hl
		#ifdef CPC
			ld  a, 5
			ld  (__x), a
			call draw_text_pre_loop
		#else
			ld  a, RAMIRO_TB_SPEAKER_ATTR
			ld  (__n), a
			call rm_draw_text_keep_x 		// leaves __x after the name
		#endif

			// Gap in the top line: name length + 1
			ld  hl, (_rm_speaker)
			ld  de, _rm_top_string + 1
			ld  a, ' '
		.rm_tb_gap
			ld  (de), a
			inc de
			ld  c, (hl)
			inc hl
			inc c
			dec c
			jr  nz, rm_tb_gap

		#ifdef CPC
			// After the name: x = 5 + length
			ld  hl, _rm_top_string + 1
			ex  de, hl
			or  a
			sbc hl, de
			ld  a, l
			add 4
			ld  (__x), a
		#endif
			ld  a, 5
			ld  (__y), a
			ld  hl, rm_tb_tab_r
			call rm_draw_tab_end

		.rm_tb_top
			// Draw FRAME TOP
			ld  a, 6
			ld  (__y), a
			ld  a, 4
			ld  (__x), a
			ld  hl, _rm_top_string
			call _rm_draw_frame_line

			ld  a, 7
			ld  (_rdy), a

			// Main loop to output lines of text

		.rm_stb_loop
			// Clear line above, if rdy > 7
			ld  a, (_rdy)
			cp  8
			jr  c, rm_stb_notop

			// Draw FRAME INNER
			ld  a, (_rdy)
			dec a
			ld  (__y), a
			ld  a, 4
			ld  (__x), a
			ld  hl, _rm_mid_string
			call _rm_draw_frame_line

		.rm_stb_notop
			// Draw FRAME INNER (text bg)
			ld  a, (_rdy)
			ld  (__y), a
			ld  a, 4
			ld  (__x), a
			ld  hl, _rm_mid_string
			call _rm_draw_frame_line

			// Draw FRAME BOTTOM
			ld  a, (_rdy)
			inc a
			ld  (__y), a
			ld  a, 4
			ld  (__x), a
			ld  hl, _rm_bottom_string
			call _rm_draw_frame_line

			// Fill buffer with the next line of text
			ld  de, _rm_temp_string
			ld  hl, (_gp_gen) 				// HL -> current text

		.rm_fill_buffer_loop
			ld  a, (hl) 					// Read char from text
			or  a
			jr  z, rm_fill_buffer_end 		// 0 -> done filling buffer (string end)
			cp  '%'
			jr  z, rm_fill_buffer_end 		// % -> done filling buffer (new line)
			ld  (de), a 					// Write to buffer
			inc de
			inc hl
			jr  rm_fill_buffer_loop

		.rm_fill_buffer_end
			xor a
			ld  (de), a 					// String end
			ld  (_gp_gen), hl

			ld  a, 5 						// Left border + 1
			ld  (_rdx), a

			ld  hl, _rm_temp_string
			call rm_dtcbc_loop

			ld  a, (_rdy)
			add 2
			ld  (_rdy), a

			// if (*gp_gen == 0) break;
			ld  hl, (_gp_gen)
			ld  a, (hl)
			or  a
			jr  z, rm_stb_exitloop

			// gp_gen ++;
			inc hl
			ld  (_gp_gen), hl
			jp  rm_stb_loop

		.rm_draw_tab_end
		#ifdef CPC
			jp  draw_text_pre_loop
		#else
			ld  a, RAMIRO_TB_TAB_ATTR
			ld  (__n), a
		.rm_draw_text_keep_x
			// draw_text_loop leaves __x one past the end; fix it
			call draw_text_loop
			ld  hl, __x
			dec (hl)
			ret
		#endif

		.rm_tb_tab_l
			defb 34, 0 					// '"': tab, left end
		.rm_tb_tab_r
			defb 35, 0 					// '#': tab, right end

		.rm_stb_exitloop
		#ifdef CPC
			ld  hl, 0
			push hl
			call _cpc_UpdateNow
			pop hl
		#else
			call SPUpdateNow
		#endif

			// peta_el_beeper (1);
			ld  l, 1
			call _peta_el_beeper

			// do { pad_read (); } while (0xff == pad_this_frame);
		.rm_stb_waitkey
			call _pad_read
			ld  a, (_pad_this_frame)
			inc a
			jr  z, rm_stb_waitkey 			// if pad = 0xff, inc pad = 0, so loop.

			call _ramiro_redraw 			// redraw_from_buffer + hotspot_paint
			call _render_all_sprites

		#ifdef CPC
			ld  hl, 1
			push hl
			call _cpc_UpdateNow
			pop hl
		#else
			call SPUpdateNow
		#endif
	#endasm
}
