// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// extern.h - Ramiro el Vampiro en "No tiene un respiro"
//
// EXTERN n, m is handled by addons/ramiro.h (see the list there):
//   EXTERN 9, n          BeepFX sound n
//   EXTERN 10..13, v     fire zone x1, y1, x2, y2 (13 switches it on)
//   EXTERN 14, 0         fire zone off
//   EXTERN 15, 0         REENTER
//   EXTERN 16, 0         REDRAW
//
// TEXT BOX "0..."  room name in the HUD; "N..." (N = 1-9) dialog said by
// ramiro_speakers [N - 1]; anything else, plain dialog.

void do_extern_action (unsigned char n, unsigned char m) {
	// Same parameters, same stack frame: just jump.
	#asm
			jp  _ramiro_extern
	#endasm
}

// Speakers, for TEXT BOX "1..." to "9..."
unsigned char rm_spk_1 [] = "RAMIRO";
unsigned char rm_spk_2 [] = "HIJA CANIJA";
unsigned char rm_spk_3 [] = "BRUJA MARUJA";
unsigned char rm_spk_4 [] = "JUANILLO EL COLMILLO";
unsigned char rm_spk_5 [] = "VOZ DE ULTRATUMBA";
unsigned char rm_spk_6 [] = "GIGANTE ELEGANTE";
unsigned char rm_spk_7 [] = "ENANO MARRANO";
unsigned char rm_spk_8 [] = "SIRVIENTE VICENTE";
unsigned char *ramiro_speakers [] = {
	rm_spk_1, rm_spk_2, rm_spk_3, rm_spk_4, rm_spk_5, rm_spk_6, rm_spk_7, rm_spk_8
};

#ifdef ENABLE_ENCODED_TEXT
	void __FASTCALL__ textbox (unsigned char *ptr) {
		#asm
				jp  _ramiro_textbox
		#endasm
	}
#endif
