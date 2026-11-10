// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// extern.h - Camp Escape
//
// EXTERN n, m from the script lands here:
//
//   EXTERN 0, 0    New game: reset the camp clock (custom.h).
//   EXTERN 1, 0    Put the enemy being touched out of action (a fed dog):
//                  mark it dead and blank its sprite. Only valid inside
//                  ENEMY TOUCHED, where enit / _en_t are the touched enemy.
//   EXTERN 2, h    Set the camp clock to hour h (leaving the cooler).

void do_extern_action (unsigned char n, unsigned char m) {
	switch (n) {
		case 0:
			clock_reset ();
			break;
		case 1:
			_en_t |= 128;
			en_an_next_frame [enit] = spr_empty;
			break;
		case 2:
			flags [F_CLOCK] = m;
			clock_reset ();
			break;
	}
}

#ifdef ENABLE_ENCODED_TEXT

	// Text box. The frame uses the font glyphs < = > # $ ' ( ) which
	// gfx/src/make_gfx.py redraws as box pieces (so texts can't use them).
	// TEXT WIDTH in the script is 22, the inner width of the box.

	#define ATTR_TEXTBOX   71		// bright white on black
	#define TEXTBOX_X      4

	unsigned char top_string []    = "<======================>";
	unsigned char mid_string []    = "#                      $";
	unsigned char bottom_string [] = "'(((((((((((((((((((((()";
	unsigned char temp_string [24];

	// Redraws the viewport from map_buff (as Papaya does).
	void redraw_from_buffer (void) {
		#asm
				ld  a, VIEWPORT_X
				ld  (__x), a
				ld  a, VIEWPORT_Y
				ld  (__y), a

				xor a
			.redraw_from_buffer_loop
				ld  (_gpit), a

				ld  bc, (_gpit)
				ld  b, 0
				ld  hl, _map_buff
				add hl, bc
				ld  a, (hl)
				ld  (__t), a

				call _draw_coloured_tile_do

				ld  a, (__x)
				add a, 2
				cp  VIEWPORT_X + 30
				jr  nz, redraw_from_buffer_set_x
				ld  a, (__y)
				add a, 2
				ld  (__y), a
				ld  a, VIEWPORT_X
			.redraw_from_buffer_set_x
				ld  (__x), a

				ld  a, (_gpit)
				inc a
				cp  150
				jr  nz, redraw_from_buffer_loop
		#endasm
	}

	// Text has been decoded in a buffer pointed by ptr. Lines are separated
	// by '%' (inserted by the compiler's word wrapper), 0 terminated.
	void __FASTCALL__ textbox (unsigned char *ptr) {
		// __FASTCALL__: the pointer arrives in HL (it can't be read by name).
		#asm
				ld  (_gp_gen), hl
		#endasm

		saca_a_todo_el_mundo_de_aqui ();

		rdy = 5;
		draw_text (TEXTBOX_X, rdy, ATTR_TEXTBOX, top_string);

		while (1) {
			rdy ++;
			draw_text (TEXTBOX_X, rdy, ATTR_TEXTBOX, mid_string);

			rdx = 0;
			while (*gp_gen && *gp_gen != '%' && rdx < 22) {
				temp_string [rdx ++] = *gp_gen ++;
			}
			temp_string [rdx] = 0;
			draw_text (TEXTBOX_X + 1, rdy, ATTR_TEXTBOX, temp_string);

			if (*gp_gen == 0 || rdy > 17) break;
			if (*gp_gen == '%') gp_gen ++;
		}

		rdy ++;
		draw_text (TEXTBOX_X, rdy, ATTR_TEXTBOX, bottom_string);

		#asm
				call SPUpdateNow
		#endasm

		peta_el_beeper (1);

		// Wait a bit, then for a fresh key press.
		for (rdx = 0; rdx < 25; rdx ++) {
			#asm
				halt
			#endasm
		}
		do {
			pad_read ();
			#asm
				halt
			#endasm
		} while (pad_this_frame == 0xff);

		// Restore the play area and the sprites.
		redraw_from_buffer ();
		render_all_sprites ();
		#asm
				call SPUpdateNow
		#endasm
	}

#endif
