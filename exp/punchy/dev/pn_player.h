// Punchy - the player: jump, ladders, propellers, punches and kicks,
// breakables, dying and respawning. From MK1_NES (engine/player.h with the
// tester's config): speeds and counters are the NES ones (60 Hz); the
// vertical speed runs 6 * MIN_FAPS_PER_FRAME / 5 NES steps per MK1 frame
// (3: 18/5 at 16.7 fps) and is scaled by as much.

#define PN_PUNCH_FRAMES 	PN_T16	// NES 16
#define PN_HIT_FRAMES 		2		// NES 4: the hitbox is active
#define PN_BRK_FRAMES 		PN_T16	// NES 16
#define PN_BRK_MAX 			4

// NES vertical constants (1/64 px per NES frame)
#define NES_G 				16
#define NES_VY_FALL_MAX 	256
#define NES_VY_JUMP 		64
#define NES_VY_JUMP_MAX 	192
#define NES_AY_JUMP 		12
#define NES_AY_FLOAT 		16
#define NES_VY_FLOAT_MAX 	256
#define NES_VY_FALL_MIN 	64
// PN_VX_MIN (NES PLAYER_VX_MIN) and PN_VY_LADDER (NES 96) are in custom.h

#define BEH_LADDER 			32
#define BEH_BREAKABLE 		16
#define BEH_FLOAT 			64
#define TILE_PROPELLER 		14
#define TILE_PROPELLER_B 	13
#define TILE_BREAKING 		19

// pad bits (sp_LEFT + sp_RIGHT + sp_UP + sp_DOWN + sp_FIRE; LEFT + RIGHT)
#define PN_ALL_KEYS 		sp_LEFT + sp_RIGHT + sp_UP + sp_DOWN + sp_FIRE
#define PN_LR_KEYS 			sp_LEFT + sp_RIGHT

signed int pn_nvy;
unsigned char pn_acc, pn_steps;
unsigned char pn_jump, pn_jct;
unsigned char pn_ladder, pn_lct, pn_float;
unsigned char pn_punch, pn_kick;
signed int pn_safe_x, pn_safe_y;

unsigned char br_x [PN_BRK_MAX], br_y [PN_BRK_MAX], br_ct [PN_BRK_MAX];
#define PN_PROPS_MAX 4
unsigned char pr_x [PN_PROPS_MAX], pr_y [PN_PROPS_MAX], pr_n;

const unsigned char pn_climb [] = { F_P_CLIMB1, F_P_CLIMB2, F_P_CLIMB3, F_P_CLIMB2 };

#asm
	// pn_set_tile (x, y, t): draw_coloured_tile (VIEWPORT_X + x + x,
	// VIEWPORT_Y + y + y, t). B = x, C = y, A = t.
	.pn_set_tile
		ld  (__t), a
		ld  a, b
		add a, a
		add a, VIEWPORT_X
		ld  (__x), a
		ld  a, c
		add a, a
		add a, VIEWPORT_Y
		ld  (__y), a
		jp  _draw_coloured_tile_do

	// A = (A + C) >> 4 (9 bits)
	.pn_cell
		add a, c
		rra
		srl a
		srl a
		srl a
		ret

	// HL = HL >> 6 (signed)
	.pn_shr6
		ld  b, 6
	.pn_shr6_l
		sra h
		rr  l
		djnz pn_shr6_l
		ret

	// HL = A << 6
	.pn_shl6
		ld  l, a
		ld  h, 0
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ret

	// HL = -HL
	.pn_neg_hl
		xor a
		sub l
		ld  l, a
		sbc a, a
		sub h
		ld  h, a
		ret
#endasm

// ----------------------------------------------------------------------------
// Breakables: tile behaviour 16, broken by the hitbox. Shown breaking for a
// moment (tile 19), then empty (tile 0). Not kept when the screen is left.

void pn_break (void) {
	// for (pn_i = 0; pn_i < PN_BRK_MAX; pn_i ++) if (br_ct [pn_i] == 0) {
	//	br_x [pn_i] = pn_cx; br_y [pn_i] = pn_cy; br_ct [pn_i] = PN_BRK_FRAMES;
	//	break;
	// }
	// rdi = pn_cx + (pn_cy << 4) - pn_cy;
	// map_attr [rdi] = 0; map_buff [rdi] = 0;
	// pn_set_tile (pn_cx, pn_cy, TILE_BREAKING);
	// peta_el_beeper (1);
	#asm
			ld  hl, _br_ct
			ld  c, 0
		.pn_bk_find
			ld  a, (hl)
			or  a
			jr  z, pn_bk_found
			inc hl
			inc c
			ld  a, c
			cp  PN_BRK_MAX
			jr  c, pn_bk_find
			jr  pn_bk_full
		.pn_bk_found
			ld  (hl), PN_BRK_FRAMES
			ld  hl, _br_x
			ld  a, c
			call pn_add_a
			ld  a, (_pn_cx)
			ld  (hl), a
			ld  hl, _br_y
			ld  a, c
			call pn_add_a
			ld  a, (_pn_cy)
			ld  (hl), a
		.pn_bk_full
			ld  a, c
			ld  (_pn_i), a

			ld  a, (_pn_cy)
			ld  c, a
			add a, a
			add a, a
			add a, a
			add a, a
			sub c
			ld  b, a
			ld  a, (_pn_cx)
			add a, b
			ld  (_rdi), a
			ld  b, a
			ld  hl, _map_attr
			call pn_add_a
			ld  (hl), 0
			ld  hl, _map_buff
			ld  a, b
			call pn_add_a
			ld  (hl), 0

			ld  a, (_pn_cx)
			ld  b, a
			ld  a, (_pn_cy)
			ld  c, a
			ld  a, TILE_BREAKING
			call pn_set_tile
			ld  l, 1
			call _peta_el_beeper
	#endasm
}

void pn_breakables_do (void) {
	// for (pn_i = 0; pn_i < PN_BRK_MAX; pn_i ++) if (br_ct [pn_i]) {
	//	if (-- br_ct [pn_i] == 0) pn_set_tile (br_x [pn_i], br_y [pn_i], 0);
	// }
	#asm
			xor a
		.pn_bd_loop
			ld  (_pn_i), a
			ld  hl, _br_ct
			call pn_add_a
			ld  a, (hl)
			or  a
			jr  z, pn_bd_next
			dec (hl)
			jr  nz, pn_bd_next
			ld  hl, _br_x
			ld  a, (_pn_i)
			call pn_add_a
			ld  b, (hl)
			ld  hl, _br_y
			ld  a, (_pn_i)
			call pn_add_a
			ld  c, (hl)
			xor a
			call pn_set_tile
		.pn_bd_next
			ld  a, (_pn_i)
			inc a
			cp  PN_BRK_MAX
			jr  c, pn_bd_loop
			ld  (_pn_i), a
	#endasm
}

// ----------------------------------------------------------------------------
// Propellers (tile 14): the tiles over them (6 at most, up to the first
// non-empty one) get behaviour 64, which pushes the player up. Animated with
// tile 13.

void pn_propellers_setup (void) {
	// pr_n = 0;
	// for (rdi = 0; rdi < 150; rdi ++) if (map_buff [rdi] == TILE_PROPELLER) {
	//	if (pr_n < PN_PROPS_MAX) { pr_x [pr_n] = rdi % 15; pr_y [pr_n] = rdi / 15; pr_n ++; }
	//	pn_k = rdi; pn_l = 6;
	//	while (pn_k >= 15 && pn_l --) {
	//		pn_k -= 15;
	//		if (map_attr [pn_k]) break;
	//		map_attr [pn_k] = BEH_FLOAT;
	//	}
	// }
	#asm
			xor a
			ld  (_pr_n), a
			ld  hl, _map_buff
			ld  c, 0 					// rdi
		.pn_ps_loop
			ld  a, (hl)
			cp  TILE_PROPELLER
			jr  nz, pn_ps_next
			push hl
			ld  a, (_pr_n)
			cp  PN_PROPS_MAX
			jr  nc, pn_ps_full
			ld  e, a
			ld  a, c
			ld  b, 0
		.pn_ps_div
			cp  15
			jr  c, pn_ps_divd
			sub 15
			inc b
			jr  pn_ps_div
		.pn_ps_divd
			ld  d, a 					// rdi % 15 (B = rdi / 15)
			ld  hl, _pr_x
			ld  a, e
			call pn_add_a
			ld  (hl), d
			ld  hl, _pr_y
			ld  a, e
			call pn_add_a
			ld  (hl), b
			ld  a, e
			inc a
			ld  (_pr_n), a
		.pn_ps_full
			ld  d, c 					// pn_k
			ld  e, 6 					// pn_l
		.pn_ps_up
			ld  a, d
			cp  15
			jr  c, pn_ps_upd
			ld  a, e
			dec e
			or  a
			jr  z, pn_ps_upd
			ld  a, d
			sub 15
			ld  d, a
			ld  hl, _map_attr
			call pn_add_a
			ld  a, (hl)
			or  a
			jr  nz, pn_ps_upd
			ld  (hl), BEH_FLOAT
			jr  pn_ps_up
		.pn_ps_upd
			ld  a, d
			ld  (_pn_k), a
			ld  a, e
			ld  (_pn_l), a
			pop hl
		.pn_ps_next
			inc hl
			inc c
			ld  a, c
			cp  150
			jr  c, pn_ps_loop
			ld  (_rdi), a
	#endasm
}

void pn_propellers_do (void) {
	// if (maincounter & 3) return;
	// pn_t1 = (maincounter & 4) ? TILE_PROPELLER_B : TILE_PROPELLER;
	// for (pn_i = 0; pn_i < pr_n; pn_i ++) pn_set_tile (pr_x [pn_i], pr_y [pn_i], pn_t1);
	#asm
			ld  a, (_maincounter)
			ld  c, a
			and 3
			ret nz
			ld  a, c
			and 4
			ld  a, TILE_PROPELLER
			jr  z, pn_pd_t
			ld  a, TILE_PROPELLER_B
		.pn_pd_t
			ld  (_pn_t1), a
			xor a
		.pn_pd_loop
			ld  (_pn_i), a
			ld  hl, _pr_n
			cp  (hl)
			ret nc
			ld  hl, _pr_x
			call pn_add_a
			ld  b, (hl)
			ld  hl, _pr_y
			ld  a, (_pn_i)
			call pn_add_a
			ld  c, (hl)
			ld  a, (_pn_t1)
			call pn_set_tile
			ld  a, (_pn_i)
			inc a
			jr  pn_pd_loop
	#endasm
}

// ----------------------------------------------------------------------------
// Every frame, keys read, the player not moved yet

void custom_pre_move (void) {
	#asm
			// last place standing on solid ground (where a killing tile sends you)
			// if (player.possee && player.vy == 0 && pn_ladder == 0 && (gpy & 15) == 0) {
			//	pn_safe_x = player.x; pn_safe_y = player.y; }
			ld  a, (_player + 26) 		// possee
			or  a
			jr  z, pn_pm_1
			ld  hl, (_player + 8) 		// vy
			ld  a, h
			or  l
			jr  nz, pn_pm_1
			ld  a, (_pn_ladder)
			or  a
			jr  nz, pn_pm_1
			ld  a, (_gpy)
			and 15
			jr  nz, pn_pm_1
			ld  hl, (_player)
			ld  (_pn_safe_x), hl
			ld  hl, (_player + 2)
			ld  (_pn_safe_y), hl
		.pn_pm_1

			// a hit landed: nothing moves for a moment
			// if (pn_frozen) { if (-- pn_frozen == 0) pn_punch = pn_kick = 0;
			//	pad0 |= every key; pad_this_frame |= every key; player.vx = 0; }
			ld  a, (_pn_frozen)
			or  a
			jr  z, pn_pm_2
			dec a
			ld  (_pn_frozen), a
			jr  nz, pn_pm_fz
			ld  (_pn_punch), a
			ld  (_pn_kick), a
		.pn_pm_fz
			ld  a, (_pad0)
			or  PN_ALL_KEYS
			ld  (_pad0), a
			ld  a, (_pad_this_frame)
			or  PN_ALL_KEYS
			ld  (_pad_this_frame), a
			ld  hl, 0
			ld  (_player + 6), hl 		// vx
		.pn_pm_2

			// FIRE: a warper if touching one (warps at the end of the frame),
			// else punch (on the floor) or kick (in the air)
			// if ((pad_this_frame & sp_FIRE) == 0 && pn_ladder == 0) {
			//	if (pn_warp_t) { pn_warp = pn_warp_t; pn_punch = pn_kick = pn_hitact = 0; }
			//	else if (pn_punch == 0 && pn_kick == 0) {
			//		if (player.possee) pn_punch = PN_PUNCH_FRAMES;
			//		else pn_kick = PN_PUNCH_FRAMES;
			//		pn_hitact = 1; peta_el_beeper (0); } }
			// pn_warp_t = 0;
			ld  a, (_pad_this_frame)
			and sp_FIRE
			jr  nz, pn_pm_3
			ld  a, (_pn_ladder)
			or  a
			jr  nz, pn_pm_3
			ld  a, (_pn_warp_t)
			or  a
			jr  z, pn_pm_fire
			ld  (_pn_warp), a
			xor a
			ld  (_pn_punch), a
			ld  (_pn_kick), a
			ld  (_pn_hitact), a
			jr  pn_pm_3
		.pn_pm_fire
			ld  a, (_pn_punch)
			ld  b, a
			ld  a, (_pn_kick)
			or  b
			jr  nz, pn_pm_3
			ld  a, (_player + 26) 		// possee
			or  a
			ld  a, PN_PUNCH_FRAMES
			jr  z, pn_pm_kick
			ld  (_pn_punch), a
			jr  pn_pm_hit
		.pn_pm_kick
			ld  (_pn_kick), a
		.pn_pm_hit
			ld  a, 1
			ld  (_pn_hitact), a
			ld  l, 0
			call _peta_el_beeper
		.pn_pm_3
			xor a
			ld  (_pn_warp_t), a

			// the hitbox, 8x8 (NES offsets from the 8 px wide box at gpx + 4)
			// if (pn_punch) { if (pn_frozen == 0) -- pn_punch;
			//	if (pn_punch < PN_PUNCH_FRAMES - PN_HIT_FRAMES) pn_hitact = 0;
			//	pn_hx = player.facing ? gpx + 4 - 15 : gpx + 4 + 15; pn_hy = gpy - 7; }
			ld  a, (_pn_punch)
			or  a
			jr  z, pn_pm_4
			ld  hl, _pn_punch
			call pn_pm_count
			ld  de, 19
			ld  a, (_player + 22) 		// facing
			or  a
			jr  z, pn_pm_px
			ld  de, -11
		.pn_pm_px
			ld  bc, -7
			call pn_pm_box
		.pn_pm_4
			// if (pn_kick) { if (pn_frozen == 0) -- pn_kick;
			//	if (pn_kick < PN_PUNCH_FRAMES - PN_HIT_FRAMES) pn_hitact = 0;
			//	pn_hx = player.facing ? gpx + 4 - 12 : gpx + 4 + 12; pn_hy = gpy - 3;
			//	if (player.possee) pn_kick = 0; }
			ld  a, (_pn_kick)
			or  a
			jr  z, pn_pm_5
			ld  hl, _pn_kick
			call pn_pm_count
			ld  de, 16
			ld  a, (_player + 22) 		// facing
			or  a
			jr  z, pn_pm_kx
			ld  de, -8
		.pn_pm_kx
			ld  bc, -3
			call pn_pm_box
			ld  a, (_player + 26) 		// possee
			or  a
			jr  z, pn_pm_5
			xor a
			ld  (_pn_kick), a
		.pn_pm_5
			// if (pn_punch == 0 && pn_kick == 0) pn_hitact = 0;
			ld  a, (_pn_punch)
			ld  b, a
			ld  a, (_pn_kick)
			or  b
			jr  nz, pn_pm_6
			ld  (_pn_hitact), a
		.pn_pm_6

			// breakables
			// if (pn_hitact) { rdx = pn_hx + 4; rdy = pn_hy + 4;
			//	if (pn_hx >= -4 && pn_hy >= -4) { pn_cx = rdx >> 4; pn_cy = rdy >> 4;
			//		if (pn_attr () & BEH_BREAKABLE) { pn_break ();
			//			pn_frozen = PN_FROZEN_FRAMES; pn_hitact = 0; } } }
			ld  a, (_pn_hitact)
			or  a
			jr  z, pn_pm_7
			ld  de, 4
			ld  hl, (_pn_hx)
			add hl, de
			ld  a, l
			ld  (_rdx), a
			ld  b, h
			ld  hl, (_pn_hy)
			add hl, de
			ld  a, l
			ld  (_rdy), a
			ld  a, h
			or  b
			rlca
			jr  c, pn_pm_7 				// pn_hx < -4 or pn_hy < -4
			ld  a, (_rdx)
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  (_pn_cx), a
			ld  a, (_rdy)
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  (_pn_cy), a
			call _pn_attr
			and BEH_BREAKABLE
			jr  z, pn_pm_7
			call _pn_break
			ld  a, PN_FROZEN_FRAMES
			ld  (_pn_frozen), a
			xor a
			ld  (_pn_hitact), a
		.pn_pm_7

			// pn_breakables_do (); pn_propellers_do (); pn_cocos_do ();
			call _pn_breakables_do
			call _pn_propellers_do
			jp  _pn_cocos_do

			// (HL) = pn_punch / pn_kick: counts down unless frozen; the hitbox
			// is only active for its first PN_HIT_FRAMES
		.pn_pm_count
			ld  a, (_pn_frozen)
			or  a
			jr  nz, pn_pm_c1
			dec (hl)
		.pn_pm_c1
			ld  a, (hl)
			cp  PN_PUNCH_FRAMES - PN_HIT_FRAMES
			ret nc
			xor a
			ld  (_pn_hitact), a
			ret

			// pn_hx = gpx + DE; pn_hy = gpy + BC
		.pn_pm_box
			ld  a, (_gpx)
			ld  l, a
			ld  h, 0
			add hl, de
			ld  (_pn_hx), hl
			ld  a, (_gpy)
			ld  l, a
			ld  h, 0
			add hl, bc
			ld  (_pn_hy), hl
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// Vertical engine

void player_custom_veng (void) {
	#asm
			// if (pn_frozen) { player.vy = pn_nvy = 0; return; }
			// if (player.vy == 0) pn_nvy = 0; 	// a floor or a ceiling stopped it
			ld  hl, 0
			ld  a, (_pn_frozen)
			or  a
			jr  z, pn_vg_1
			ld  (_player + 8), hl
			ld  (_pn_nvy), hl
			ret
		.pn_vg_1
			ld  hl, (_player + 8)
			ld  a, h
			or  l
			jr  nz, pn_vg_2
			ld  (_pn_nvy), hl
		.pn_vg_2

			// pn_acc += PN_NUM; pn_steps = pn_acc / 5; pn_acc = pn_acc % 5;
			ld  a, (_pn_acc)
			add a, PN_NUM
			ld  b, 0
		.pn_vg_d5
			cp  5
			jr  c, pn_vg_d5e
			sub 5
			inc b
			jr  pn_vg_d5
		.pn_vg_d5e
			ld  (_pn_acc), a
			ld  a, b
			ld  (_pn_steps), a

			// Ladders: the tile under the body's centre (or under the feet, to
			// go down from the top) has behaviour 32
			// pn_cx = (gpx + 8) >> 4;
			// pn_cy = (gpy + 8) >> 4; pn_t1 = pn_attr () == BEH_LADDER;
			// pn_cy = (gpy + 16) >> 4; pn_t2 = pn_attr () == BEH_LADDER;
			ld  c, 8
			ld  a, (_gpx)
			call pn_cell
			ld  (_pn_cx), a
			ld  a, (_gpy)
			call pn_cell
			ld  (_pn_cy), a
			call _pn_attr
			cp  BEH_LADDER
			ld  a, 0
			jr  nz, pn_vg_t1
			inc a
		.pn_vg_t1
			ld  (_pn_t1), a
			ld  c, 16
			ld  a, (_gpy)
			call pn_cell
			ld  (_pn_cy), a
			call _pn_attr
			cp  BEH_LADDER
			ld  a, 0
			jr  nz, pn_vg_t2
			inc a
		.pn_vg_t2
			ld  (_pn_t2), a

			// if (pn_ladder == 0 && (((pad0 & sp_UP) == 0 && pn_t1) ||
			//	((pad0 & sp_DOWN) == 0 && pn_t2 && player.possee))) {
			//	pn_ladder = 1; pn_jump = 0;
			//	gpx = pn_cx << 4; player.x = gpx << 6; player.vx = 0; }
			ld  a, (_pn_ladder)
			or  a
			jr  nz, pn_vg_4
			ld  a, (_pad0)
			ld  c, a
			and sp_UP
			jr  nz, pn_vg_3
			ld  a, (_pn_t1)
			or  a
			jr  nz, pn_vg_on
		.pn_vg_3
			ld  a, c
			and sp_DOWN
			jp  nz, pn_vg_5
			ld  a, (_pn_t2)
			or  a
			jp  z, pn_vg_5
			ld  a, (_player + 26) 		// possee
			or  a
			jp  z, pn_vg_5
		.pn_vg_on
			ld  a, 1
			ld  (_pn_ladder), a
			xor a
			ld  (_pn_jump), a
			ld  a, (_pn_cx)
			add a, a
			add a, a
			add a, a
			add a, a
			ld  (_gpx), a
			call pn_shl6
			ld  (_player), hl
			ld  hl, 0
			ld  (_player + 6), hl

			// if (pn_ladder) {
			//	if ((pn_t1 == 0 && pn_t2 == 0) || (pad0 & (sp_LEFT | sp_RIGHT)) != (sp_LEFT | sp_RIGHT))
			//		pn_ladder = 0;
			//	else { player.vx = 0;
			//		player.vy = !UP ? -PN_VY_LADDER : !DOWN ? PN_VY_LADDER : 0;
			//		if (player.vy) pn_lct ++; pn_nvy = 0;
			//		// climbing down onto the floor: get off
			//		if (pn_t2 == 0 && player.vy > 0 && player.possee) pn_ladder = 0;
			//		return; } }
		.pn_vg_4
			ld  a, (_pn_t1)
			ld  b, a
			ld  a, (_pn_t2)
			or  b
			jr  z, pn_vg_off
			ld  a, (_pad0)
			and PN_LR_KEYS
			cp  PN_LR_KEYS
			jr  z, pn_vg_climb
		.pn_vg_off
			xor a
			ld  (_pn_ladder), a
			jr  pn_vg_5
		.pn_vg_climb
			ld  hl, 0
			ld  (_player + 6), hl
			ld  (_pn_nvy), hl
			ld  a, (_pad0)
			ld  c, a
			ld  hl, -PN_VY_LADDER
			and sp_UP
			jr  z, pn_vg_cl
			ld  hl, PN_VY_LADDER
			ld  a, c
			and sp_DOWN
			jr  z, pn_vg_cl
			ld  hl, 0
		.pn_vg_cl
			ld  (_player + 8), hl
			ld  a, h
			or  l
			ret z 						// still: not getting off either
			ld  a, (_pn_lct)
			inc a
			ld  (_pn_lct), a
			bit 7, h
			ret nz 						// going up
			ld  a, (_pn_t2)
			or  a
			ret nz
			ld  a, (_player + 26) 		// possee
			or  a
			ret z
			xor a
			ld  (_pn_ladder), a
			ret

		.pn_vg_5
			// Propellers: behaviour 64 where the feet are
			// pn_cy = (gpy + 15) >> 4;
			// pn_cx = (gpx + 4) >> 4; pn_float = pn_attr () == BEH_FLOAT;
			// pn_cx = (gpx + 11) >> 4; if (pn_attr () == BEH_FLOAT) pn_float = 1;
			ld  c, 15
			ld  a, (_gpy)
			call pn_cell
			ld  (_pn_cy), a
			ld  c, 4
			ld  a, (_gpx)
			call pn_cell
			ld  (_pn_cx), a
			call _pn_attr
			cp  BEH_FLOAT
			ld  a, 0
			jr  nz, pn_vg_f1
			inc a
		.pn_vg_f1
			ld  (_pn_float), a
			ld  c, 11
			ld  a, (_gpx)
			call pn_cell
			ld  (_pn_cx), a
			call _pn_attr
			cp  BEH_FLOAT
			jr  nz, pn_vg_6
			ld  a, 1
			ld  (_pn_float), a
		.pn_vg_6

			// Jump start: UP on the floor (UP over a ladder climbs it instead)
			// if ((pad_this_frame & sp_UP) == 0 && pn_jump == 0 && (player.possee || player.gotten)) {
			//	pn_nvy = -NES_VY_JUMP; pn_jump = 1; pn_jct = 0; peta_el_beeper (3); }
			ld  a, (_pad_this_frame)
			and sp_UP
			jr  nz, pn_vg_7
			ld  a, (_pn_jump)
			or  a
			jr  nz, pn_vg_7
			ld  a, (_player + 26) 		// possee
			ld  b, a
			ld  a, (_player + 25) 		// gotten
			or  b
			jr  z, pn_vg_7
			ld  hl, -NES_VY_JUMP
			ld  (_pn_nvy), hl
			ld  a, 1
			ld  (_pn_jump), a
			xor a
			ld  (_pn_jct), a
			ld  l, 3
			call _peta_el_beeper
		.pn_vg_7

			// while (pn_steps --) {
			//	if (pn_float) { pn_nvy -= NES_AY_FLOAT;
			//		if (pn_nvy < -NES_VY_FLOAT_MAX) pn_nvy = -NES_VY_FLOAT_MAX; }
			//	else if (pn_jump == 0) { 	// (no gravity while the jump is held, as on the NES)
			//		pn_nvy += NES_G; if (pn_nvy > NES_VY_FALL_MAX) pn_nvy = NES_VY_FALL_MAX; }
			//	if (pn_jump) {
			//		if ((pad0 & sp_UP) == 0) {
			//			if (pn_jct < NES_AY_JUMP) pn_nvy -= (NES_AY_JUMP - pn_jct);
			//			if (pn_nvy < -NES_VY_JUMP_MAX) pn_nvy = -NES_VY_JUMP_MAX;
			//			if (++ pn_jct == 16) pn_jump = 0;
			//		} else pn_jump = 0; } }
			ld  hl, (_pn_nvy)
			ld  a, (_pn_steps)
			or  a
			jr  z, pn_vg_8
			ld  b, a
		.pn_vg_step
			ld  a, (_pn_float)
			or  a
			jr  z, pn_vg_nf
			ld  de, -NES_AY_FLOAT
			add hl, de
			ld  de, NES_VY_FLOAT_MAX
			push hl
			add hl, de
			bit 7, h
			pop hl
			jr  z, pn_vg_jmp
			ld  hl, -NES_VY_FLOAT_MAX
			jr  pn_vg_jmp
		.pn_vg_nf
			ld  a, (_pn_jump)
			or  a
			jr  nz, pn_vg_jmp
			ld  de, NES_G
			add hl, de
			push hl
			ex  de, hl
			ld  hl, NES_VY_FALL_MAX
			or  a
			sbc hl, de
			bit 7, h
			pop hl
			jr  z, pn_vg_jmp
			ld  hl, NES_VY_FALL_MAX
		.pn_vg_jmp
			ld  a, (_pn_jump)
			or  a
			jr  z, pn_vg_sn
			ld  a, (_pad0)
			and sp_UP
			jr  nz, pn_vg_jend
			ld  a, (_pn_jct)
			cp  NES_AY_JUMP
			jr  nc, pn_vg_jc
			ld  c, a
			ld  a, NES_AY_JUMP
			sub c
			ld  e, a
			ld  d, 0
			or  a
			sbc hl, de
		.pn_vg_jc
			ld  de, NES_VY_JUMP_MAX
			push hl
			add hl, de
			bit 7, h
			pop hl
			jr  z, pn_vg_jc2
			ld  hl, -NES_VY_JUMP_MAX
		.pn_vg_jc2
			ld  a, (_pn_jct)
			inc a
			ld  (_pn_jct), a
			cp  16
			jr  nz, pn_vg_sn
		.pn_vg_jend
			xor a
			ld  (_pn_jump), a
		.pn_vg_sn
			djnz pn_vg_step
		.pn_vg_8
			ld  (_pn_nvy), hl
			ld  a, 0xff
			ld  (_pn_steps), a

			// player.vy = pn_nvy * PN_NUM / 5; (signed, rounded towards 0)
			ld  a, h
			push af
			bit 7, h
			call nz, pn_neg_hl
			ld  d, h
			ld  e, l
			ld  hl, 0
			ld  b, PN_NUM
		.pn_vg_mul
			add hl, de
			djnz pn_vg_mul
			call pn_div5
			pop af
			bit 7, a
			call nz, pn_neg_hl
			ld  (_player + 8), hl
	#endasm
}

// ----------------------------------------------------------------------------
// Killing tiles: a life, and back to the last safe place

unsigned char custom_bg_hit (void) {
	// if (hit_v == 0 && hit_h == 0) return 0;
	// player.x = pn_safe_x; player.y = pn_safe_y;
	// player.vx = player.vy = 0; pn_nvy = 0;
	// gpx = player.x >> 6; gpy = player.y >> 6;
	// pn_ladder = pn_jump = pn_punch = pn_kick = pn_hitact = 0;
	// return (player.estado == EST_NORMAL);
	#asm
			ld  hl, 0
			ld  a, (_hit_v)
			ld  b, a
			ld  a, (_hit_h)
			or  b
			ret z
			ld  hl, (_pn_safe_x)
			ld  (_player), hl
			call pn_shr6
			ld  a, l
			ld  (_gpx), a
			ld  hl, (_pn_safe_y)
			ld  (_player + 2), hl
			call pn_shr6
			ld  a, l
			ld  (_gpy), a
			ld  hl, 0
			ld  (_player + 6), hl
			ld  (_player + 8), hl
			ld  (_pn_nvy), hl
			xor a
			ld  (_pn_ladder), a
			ld  (_pn_jump), a
			ld  (_pn_punch), a
			ld  (_pn_kick), a
			ld  (_pn_hitact), a
			ld  a, (_player + 23) 		// estado
			or  a
			ret nz
			inc l
	#endasm
}

// ----------------------------------------------------------------------------
// Frame (at the end of move ())

void player_custom_frame_ptr (void) {
	// pn_pdx = 0; pc_on [0] = 0;
	// pn_i = player.facing ? 1 : 0;
	// if (pn_ladder) { pn_f = pn_climb [(pn_lct >> 1) & 3]; pn_i = 0; }
	// else if (pn_float) pn_f = F_P_DOWN;
	// else if (pn_punch) { pn_f = F_P_PUNCH; pn_pdx = DX_P_PUNCH;
	//	pc_on [0] = 1; pc_f [0] = PC_P_PUNCH; pn_y = PD_P_PUNCH; }
	// else if (pn_kick) { pn_f = F_P_KICK; pn_pdx = DX_P_KICK;
	//	pc_on [0] = 1; pc_f [0] = PC_P_KICK; pn_y = PD_P_KICK; }
	// else if (player.possee || player.gotten) {
	//	if (player.vx > PN_VX_MIN || player.vx < -PN_VX_MIN) pn_f = F_P_WALK1 + ((gpx >> 3) & 3);
	//	else if (player.vx) pn_f = F_P_PREWALK; else pn_f = F_P_IDLE; }
	// else pn_f = pn_nvy < NES_VY_FALL_MIN ? F_P_UP : F_P_DOWN;
	#asm
			xor a
			ld  (_pn_pdx), a
			ld  (_pc_on), a
			ld  a, (_player + 22) 		// facing
			or  a
			jr  z, pn_pf_fc
			ld  a, 1
		.pn_pf_fc
			ld  (_pn_i), a

			ld  a, (_pn_ladder)
			or  a
			jr  z, pn_pf_1
			ld  a, (_pn_lct)
			rrca
			and 3
			ld  hl, _pn_climb
			call pn_add_a
			ld  a, (hl)
			ld  (_pn_f), a
			xor a
			ld  (_pn_i), a
			jp  pn_pf_piece
		.pn_pf_1
			ld  a, (_pn_float)
			or  a
			ld  a, F_P_DOWN
			jr  nz, pn_pf_set
			ld  a, (_pn_punch)
			or  a
			jr  z, pn_pf_2
			ld  a, F_P_PUNCH
			ld  bc, DX_P_PUNCH * 256 + PC_P_PUNCH
			ld  d, PD_P_PUNCH
			jr  pn_pf_pc
		.pn_pf_2
			ld  a, (_pn_kick)
			or  a
			jr  z, pn_pf_3
			ld  a, F_P_KICK
			ld  b, DX_P_KICK
			ld  c, PC_P_KICK
			ld  d, PD_P_KICK
		.pn_pf_pc
			ld  (_pn_f), a
			ld  a, b
			ld  (_pn_pdx), a
			ld  a, 1
			ld  (_pc_on), a
			ld  a, c
			ld  (_pc_f), a
			ld  a, d
			ld  (_pn_y), a
			jp  pn_pf_piece
		.pn_pf_3
			ld  a, (_player + 26) 		// possee
			ld  b, a
			ld  a, (_player + 25) 		// gotten
			or  b
			jr  z, pn_pf_air
			ld  hl, (_player + 6) 		// vx
			ld  de, -PN_VX_MIN - 1
			add hl, de
			bit 7, h
			jr  z, pn_pf_walk 			// vx > PN_VX_MIN
			ld  hl, (_player + 6)
			ld  de, PN_VX_MIN
			add hl, de
			bit 7, h
			jr  nz, pn_pf_walk 			// vx < -PN_VX_MIN
			ld  hl, (_player + 6)
			ld  a, h
			or  l
			ld  a, F_P_PREWALK
			jr  nz, pn_pf_set
			ld  a, F_P_IDLE
			jr  pn_pf_set
		.pn_pf_walk
			ld  a, (_gpx)
			rrca
			rrca
			rrca
			and 3
			add a, F_P_WALK1
			jr  pn_pf_set
		.pn_pf_air
			ld  hl, (_pn_nvy)
			ld  de, -NES_VY_FALL_MIN
			add hl, de
			bit 7, h
			ld  a, F_P_UP
			jr  nz, pn_pf_set
			ld  a, F_P_DOWN
		.pn_pf_set
			ld  (_pn_f), a

		.pn_pf_piece
			// if (pc_on [0]) {
			//	// the piece is right of the body; facing left, both are mirrored
			//	// around the middle of the 16 px box (x -> 16 - x - w)
			//	pc_left [0] = pn_i;
			//	pc_x [0] = pn_i ? gpx - pn_pdx - 8 : gpx + pn_pdx + 16;
			//	pc_y [0] = gpy - 16 + pn_y;
			//	if ((player.estado & EST_PARP) && half_life == 0) pc_on [0] = 0; }
			ld  a, (_pc_on)
			or  a
			jr  z, pn_pf_np
			ld  a, (_pn_i)
			ld  (_pc_left), a
			ld  a, (_pn_pdx)
			ld  e, a
			ld  d, 0
			bit 7, e
			jr  z, pn_pf_dp
			dec d
		.pn_pf_dp
			ld  a, (_gpx)
			ld  l, a
			ld  h, 0
			ld  a, (_pn_i)
			or  a
			jr  z, pn_pf_pr
			sbc hl, de 					// (carry clear)
			ld  de, -8
			jr  pn_pf_px
		.pn_pf_pr
			add hl, de
			ld  de, 16
		.pn_pf_px
			add hl, de
			ld  (_pc_x), hl
			ld  a, (_pn_y)
			ld  e, a
			ld  d, 0
			ld  a, (_gpy)
			ld  l, a
			ld  h, 0
			add hl, de
			ld  de, -16
			add hl, de
			ld  (_pc_y), hl
			ld  a, (_player + 23) 		// estado
			and EST_PARP
			jr  z, pn_pf_np
			ld  a, (_half_life)
			or  a
			jr  nz, pn_pf_np
			ld  (_pc_on), a
		.pn_pf_np

			// if (pn_i) pn_pdx = -pn_pdx;
			// if ((signed int) gpx + pn_pdx < 0) pn_pdx = -gpx;
			ld  a, (_pn_i)
			or  a
			jr  z, pn_pf_nf
			ld  a, (_pn_pdx)
			neg
			ld  (_pn_pdx), a
		.pn_pf_nf
			ld  a, (_pn_pdx)
			ld  e, a
			ld  d, 0
			bit 7, e
			jr  z, pn_pf_dp2
			dec d
		.pn_pf_dp2
			ld  a, (_gpx)
			ld  l, a
			ld  h, 0
			add hl, de
			bit 7, h
			jr  z, pn_pf_in
			ld  a, (_gpx)
			neg
			ld  (_pn_pdx), a
		.pn_pf_in

			// pn_a = 0; pn_frame (); player.next_frame = pn_fp;
			// CPC: sp_sw [SP_PLAYER].cox = pn_pdx;
			xor a
			ld  (_pn_a), a
			call _pn_frame
			ld  hl, (_pn_fp)
			ld  (_player + 17), hl
		#ifdef CPC
			ld  a, (_pn_pdx)
			ld  (BASE_SPRITES + 16 * SP_PLAYER + 6), a
		#endif
	#endasm
}

// ----------------------------------------------------------------------------
// Warping (FIRE on a warper, enemy type 32 + screen): done here, before the
// screen is updated, so the next frame draws the new screen

void before_vsync (void) {
	// if (pn_warp) {
	//	n_pant = pn_warp - 1;
	//	player.x = pn_warp_x << 6; player.y = pn_warp_y << 6;
	//	gpx = pn_warp_x; gpy = pn_warp_y;
	//	player.vx = player.vy = 0; pn_nvy = 0;
	//	pn_ladder = pn_jump = 0;
	//	pn_warp = 0; }
	#asm
			ld  a, (_pn_warp)
			or  a
			ret z
			dec a
			ld  (_n_pant), a
			ld  a, (_pn_warp_x)
			ld  (_gpx), a
			call pn_shl6
			ld  (_player), hl
			ld  a, (_pn_warp_y)
			ld  (_gpy), a
			call pn_shl6
			ld  (_player + 2), hl
			ld  hl, 0
			ld  (_player + 6), hl
			ld  (_player + 8), hl
			ld  (_pn_nvy), hl
			xor a
			ld  (_pn_ladder), a
			ld  (_pn_jump), a
			ld  (_pn_warp), a
	#endasm
}
