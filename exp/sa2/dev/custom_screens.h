// Sir Ababol 2 - the screens (CUSTOM_SCREENS): title, "LEVEL n", game over,
// the password (shown at the end of load 1, asked for at the start of load
// 2), the cutscenes (the intro before the first title of load 1, the ending
// after level 6) and the "UN PERRO" egg. The original's main (), menu.h,
// cpcpass.h and cutscene.h. Hand-written assembly.

extern unsigned char s_title [];
#asm
	._s_title
		BINARY "cpc/titlec.bin"
#endasm

// The password: the original's arrows (tiles 22-26: left, right, up, jump,
// sword). Part A shows it, part B asks for it.
unsigned char sa2_password [] = { 0, 2, 3, 4, 1, 4, 3, 0, 2 };
unsigned char sa2_pass_bits [] = { sp_LEFT, sp_RIGHT, sp_UP, sp_FIRE2, sp_FIRE };
unsigned char sa2_pass_in [9];
unsigned char sa2_first, sa2_typing, sa2_perro, sa2_pass_ok;

// the cutscene's 4 inks (the original's my_inks: hardware 20, 23, 27, 11)
unsigned char sa2_cut_inks [] = { 0x14, 0x17, 0x1b, 0x0b };

#define SA2_CUT_FONT 	0x9C00		// cutscene font (1K), over the 9000-BFFF buffer
#define SA2_CUT_WIN 	0x9000		// a picture window (2304 bytes)
#define SA2_CUT_TX 		4			// text: chars (2 bytes) 4-31, rows 13-20
#define SA2_CUT_TY 		13

#asm
	LIB cd_pal_m1

	// ------------------------------------------------------------------
	// Waits: HL halts (300 Hz), or until a key (pad) is pressed: NZ then.
	.sa2_wait
		halt
		push hl
		call _pad_read
		pop hl
		ld  a, (_pad0)
		inc a
		ret nz
		dec hl
		ld  a, h
		or  l
		jr  nz, sa2_wait
		ret

	// Until no key is pressed
	.sa2_release
		halt
		call _pad_read
		ld  a, (_pad0)
		inc a
		jr  nz, sa2_release
		ret

	// ------------------------------------------------------------------
	// Text with the game's font (ASCII 32-95): HL = string, B, C = chars.
	// Into the tile map inside the viewport (cd_char).
	.sa2_print
		ld  a, (hl)
		or  a
		ret z
		sub 32
		ld  (__t), a
		ld  a, b
		ld  (__x), a
		ld  a, c
		ld  (__y), a
		push hl
		push bc
		call _cd_char
		pop bc
		pop hl
		inc hl
		inc b
		jr  sa2_print

	// The screen as it is now (sprites too)
	.sa2_show
		ld  hl, 1
		push hl
		call _cpc_UpdateNow
		pop hl
		ret

	// blackout_area: the play area, all background (backdrop, sparkles), no
	// sprites. Not shown yet.
	.sa2_blackout
		call _saca_a_todo_el_mundo_de_aqui
		ld  c, 0
	.sa2_bo_y
		ld  b, 0
	.sa2_bo_x
		xor a
		call sa2_draw_s
		inc b
		ld  a, b
		cp  15
		jr  c, sa2_bo_x
		inc c
		ld  a, c
		cp  10
		jr  c, sa2_bo_y
		ret

	// The password arrows: HL = 9 values, at tiles (3.., 5)
	.sa2_arrows
		ld  bc, 3 * 256 + 5
	.sa2_ar_loop
		ld  a, (hl)
		add a, 22
		push hl
		call sa2_draw
		pop hl
		inc hl
		inc b
		ld  a, b
		cp  12
		jr  c, sa2_ar_loop
		ret
#endasm

// ----------------------------------------------------------------------------
// Cutscenes, mode 1: the frame, then the load's two pictures with two texts
// each, typed (FIRE: all at once), erased line by line.

#asm
	// HL = screen address of char (B, C) (2 bytes x 8 lines, 64 bytes a row)
	.sa2_cut_addr
		ld  l, c
		ld  h, 0
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  a, b
		add a, a
		add a, l
		ld  l, a
		ld  a, h
		adc a, 0xc0
		ld  h, a
		ret

	// 16 bytes at DE (8 lines of 2) to char (B, C). Keeps BC.
	.sa2_cut_char
		push bc
		call sa2_cut_addr
		ld  b, 8
	.sa2_cc_loop
		ld  a, (de)
		ld  (hl), a
		inc de
		inc hl
		ld  a, (de)
		ld  (hl), a
		inc de
		dec hl
		ld  a, h
		add a, 8
		ld  h, a
		djnz sa2_cc_loop
		pop bc
		ret

	// 9000-BFFF to the screen; 0 first: a blank screen
	.sa2_cut_blank
		ld  hl, 0x9000
		ld  de, 0x9001
		ld  bc, 12287
		ld  (hl), 0
		ldir
	.sa2_cut_copy
		ld  hl, 0x9000
		jp  cd_screen_copy

	// The picture window: zx0 at HL, 16 x 9 chars at char (8, 2)
	.sa2_cut_window
		ld  de, SA2_CUT_WIN
		call dzx0_standard
		ld  de, SA2_CUT_WIN
		ld  c, 2
	.sa2_cw_y
		ld  b, 8
	.sa2_cw_x
		call sa2_cut_char
		inc b
		ld  a, b
		cp  24
		jr  c, sa2_cw_x
		inc c
		ld  a, c
		cp  11
		jr  c, sa2_cw_y
		ret

	// The text at HL (@: next line, 2 rows down); HL after its 0 on return
	.sa2_cut_text
		ld  bc, SA2_CUT_TX * 256 + SA2_CUT_TY
		ld  a, 1
		ld  (_sa2_typing), a 		// typing, char by char
	.sa2_ct_loop
		ld  a, (hl)
		inc hl
		or  a
		ret z
		cp  64 			// '@'
		jr  nz, sa2_ct_char
		ld  b, SA2_CUT_TX
		inc c
		inc c
		jr  sa2_ct_loop
	.sa2_ct_char
		push hl
		sub 32
		ld  l, a
		ld  h, 0
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  de, SA2_CUT_FONT
		add hl, de
		ex  de, hl
		call sa2_cut_char
		inc b
		ld  a, (_sa2_typing)
		or  a
		jr  z, sa2_ct_next
		push bc
		ld  hl, 4
		call sa2_wait
		ld  a, (_pad0)
		and sp_FIRE
		jr  nz, sa2_ct_1
		xor a
		ld  (_sa2_typing), a
	.sa2_ct_1
		pop bc
	.sa2_ct_next
		pop hl
		jr  sa2_ct_loop

	// After a text: a while (or a key), FIRE up, the text erased line by line
	.sa2_cut_pause
		ld  hl, 250
		call sa2_wait
	.sa2_cp_fire
		halt
		call _pad_read
		ld  a, (_pad0)
		and sp_FIRE
		jr  z, sa2_cp_fire
		ld  c, SA2_CUT_TY
	.sa2_cp_line
		ld  b, SA2_CUT_TX
	.sa2_cp_ch
		ld  de, SA2_CUT_FONT 		// ' '
		call sa2_cut_char
		inc b
		ld  a, b
		cp  32
		jr  c, sa2_cp_ch
		push bc
		ld  hl, 4
		call sa2_wait
		pop bc
		inc c
		ld  a, c
		cp  SA2_CUT_TY + 8
		jr  c, sa2_cp_line
		ret
#endasm

void sa2_cutscene (void) {
	#asm
			ld  hl, 7 				// intro / ending song
			call _wyz_play_music
			call sa2_cut_blank
			ld  l, 1
			call cpc_SetMode
			ld  hl, _sa2_cut_inks
			call cd_pal_m1
			ld  hl, _cut_frame
			ld  de, 0x9000
			call dzx0_standard
			call sa2_cut_copy
			ld  hl, _cut_font
			ld  de, SA2_CUT_FONT
			call dzx0_standard

			ld  hl, _cut_win_a
			call sa2_cut_window
			ld  hl, _cut_texts
			call sa2_cut_text
			push hl
			call sa2_cut_pause
			pop hl
			call sa2_cut_text
			push hl
			call sa2_cut_pause
			ld  hl, _cut_win_b
			call sa2_cut_window
			pop hl
			call sa2_cut_text
			push hl
			call sa2_cut_pause
			pop hl
			call sa2_cut_text
			call sa2_cut_pause

			call _wyz_stop_sound
			// blank, back to mode 0 (the next screen sets the palette)
			call sa2_cut_blank
			ld  l, 0
			call cpc_SetMode
	#endasm
}

// ----------------------------------------------------------------------------
// The level's tiles must be there to draw the play area: the first level of
// the load, for the password screen.

#if SA2_LOAD == 2
void sa2_enter_password (void) {
	#asm
			call sa2_level_load
		.sa2_ep_again
			call sa2_blackout
			ld  hl, sa2_ep_text
			ld  bc, 9 * 256 + 9
			call sa2_print
			call sa2_show
			call sa2_release
			ld  hl, _sa2_pass_in
			ld  b, 0
		.sa2_ep_key
			// a key: which of the 5?
			push hl
			push bc
		.sa2_ep_wait
			halt
			call _pad_read
			ld  a, (_pad0)
			cpl
			ld  e, a
			ld  hl, _sa2_pass_bits
			ld  c, 0
		.sa2_ep_which
			ld  a, (hl)
			and e
			jr  nz, sa2_ep_got
			inc hl
			inc c
			ld  a, c
			cp  5
			jr  c, sa2_ep_which
			jr  sa2_ep_wait
		.sa2_ep_got
			ld  a, c
			pop bc
			pop hl
			ld  (hl), a
			inc hl
			push hl
			push bc
			// its arrow at (3 + n, 5)
			add a, 22
			ld  c, 5
			push af
			ld  a, b
			add a, 3
			ld  b, a
			pop af
			call sa2_draw
			ld  l, 1
			call _peta_el_beeper
			call sa2_show
			call sa2_release
			pop bc
			pop hl
			inc b
			ld  a, b
			cp  9
			jr  c, sa2_ep_key

			// check
			ld  hl, _sa2_pass_in
			ld  de, _sa2_password
			ld  b, 9
		.sa2_ep_check
			ld  a, (de)
			cp  (hl)
			jr  nz, sa2_ep_wrong
			inc hl
			inc de
			djnz sa2_ep_check
			ret
		.sa2_ep_wrong
			ld  l, 11
			call _peta_el_beeper
			jr  sa2_ep_again

		.sa2_ep_text
			defm "ENTER PASSWORD"
			defb 0
	#endasm
}
#endif

void title_screen (void) {
	#if SA2_LOAD == 1
		// The intro, before the first title
		#asm
				ld  a, (_sa2_first)
				or  a
				jr  nz, sa2_ts_1
				inc a
				ld  (_sa2_first), a
				call _sa2_cutscene
			.sa2_ts_1
		#endasm
	#endif

	#asm
			ld  hl, _s_title
			call _unpack_screen
			ld  hl, 0
			push hl
			call _cpc_UpdateNow
			pop hl
			ld  hl, 0
			call _wyz_play_music
			call _select_controls
	#endasm

	#if SA2_LOAD == 2
		// The password, once
		#asm
				ld  a, (_sa2_pass_ok)
				or  a
				ret nz
				inc a
				ld  (_sa2_pass_ok), a
				jp  _sa2_enter_password
		#endasm
	#endif
}

// "LEVEL n" / "LET'S GO!!" with the jingle (from custom_level_setup)
unsigned char sa2_str_level [] = "LEVEL 0X";

void sa2_level_screen (void) {
	#asm
			call sa2_blackout
			ld  a, (_level)
		#if SA2_LOAD == 1
			add a, 49 			// '1'
		#else
			add a, 52 			// '4'
		#endif
			ld  (_sa2_str_level + 7), a
			ld  hl, _sa2_str_level
			ld  bc, 12 * 256 + 10
			call sa2_print
			ld  hl, sa2_ls_go
			ld  bc, 11 * 256 + 12
			call sa2_print
			call sa2_show
			ld  hl, 3
			call _wyz_play_music
			ld  hl, 500
			call sa2_wait
			jp  _wyz_stop_sound
		.sa2_ls_go
			defm "LET'S GO!!"
			defb 0
	#endasm
}

void game_over (void) {
	#asm
			ld  hl, sa2_go_sp
			ld  bc, 10 * 256 + 10
			call sa2_print
			ld  hl, sa2_go_text
			ld  bc, 10 * 256 + 11
			call sa2_print
			ld  hl, sa2_go_sp
			ld  bc, 10 * 256 + 12
			call sa2_print
			call sa2_show
			ld  hl, 2
			call _wyz_play_music
			ld  hl, 500
			call sa2_wait
			jp  _wyz_stop_sound
		.sa2_go_sp
			defm "            "
			defb 0
		.sa2_go_text
			defm " GAME OVER! "
			defb 0
	#endasm
}

void game_ending (void) {
	#if SA2_LOAD == 1
		// Next part's password
		#asm
				call sa2_blackout
				ld  hl, _sa2_password
				call sa2_arrows
				ld  hl, sa2_ge_t1
				ld  bc, 11 * 256 + 8
				call sa2_print
				ld  hl, sa2_ge_t2
				ld  bc, 12 * 256 + 9
				call sa2_print
				call sa2_show
				call sa2_release
				ld  hl, 32767
				jp  sa2_wait
			.sa2_ge_t1
				defm "NEXT LEVEL"
				defb 0
			.sa2_ge_t2
				defm "PASSWORD"
				defb 0
		#endasm
	#else
		#asm
				jp  _sa2_cutscene
		#endasm
	#endif
}

// ----------------------------------------------------------------------------
// Level 6, screen 3: un perro (from custom_pre_move)

void sa2_perro_check (void) {
	#if SA2_LOAD == 2
		#asm
				ld  a, (_level)
				cp  2
				ret nz
				ld  a, (_n_pant)
				cp  3
				ret nz
				ld  a, (_sa2_perro)
				or  a
				ret nz
				ld  a, (_gpx)
				sub 193
				cp  31
				ret nc
				ld  a, (_gpy)
				sub 65
				cp  31
				ret nc
				ld  a, 1
				ld  (_sa2_perro), a
				ld  hl, sa2_pe_t
				ld  bc, 14 * 256 + 9
				jp  sa2_print
			.sa2_pe_t
				defm "UN PERRO"
				defb 0
		#endasm
	#endif
}
