// Punchy - the engine's frame tables are not used: player_custom_frame_ptr ()
// and extra_enems_move () (pn_gfx.h) set every frame. One entry each is
// enough for the code that still reads them at the start of a screen.

#ifdef CPC
	extern unsigned char *sprite_cells [0];
	#asm
		._sprite_cells
			defw _spr_tall, _spr_tall, _spr_tall, _spr_tall
			defw _spr_tall, _spr_tall, _spr_tall, _spr_tall
			defw _spr_tall, _spr_tall, _spr_tall, _spr_tall
			defw _spr_tall, _spr_tall, _spr_tall, _spr_tall
	#endasm
#else
	unsigned char *sprite_cells [] = {
		spr_tall, spr_tall, spr_tall, spr_tall, spr_tall, spr_tall, spr_tall, spr_tall,
		spr_tall, spr_tall, spr_tall, spr_tall, spr_tall, spr_tall, spr_tall, spr_tall
	};
#endif
