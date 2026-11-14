// Sir Ababol 2 - songs (zx0 of the original's, the ogt .mus.bin files: comp_cpc).
//   0 title, 1 the level's (set by custom.h), 2 game over, 3 level jingle
//   ("LET'S GO"), 4-6 the load's level songs, 7 intro (load 1) / ending (load 2)

extern unsigned char *wyz_songs [0];

#asm
	._sa2_title_mus
		BINARY "../ogt/title.mus.bin"
	._sa2_gover_mus
		BINARY "../ogt/gover.mus.bin"
	._sa2_level_mus
		BINARY "../ogt/level.mus.bin"
#endasm
#if SA2_LOAD == 1
	#asm
		._sa2_l0_mus
			BINARY "../ogt/level0.mus.bin"
		._sa2_l1_mus
			BINARY "../ogt/level1.mus.bin"
		._sa2_l2_mus
			BINARY "../ogt/level2.mus.bin"
		._sa2_cut_mus
			BINARY "../ogt/intro.mus.bin"
	#endasm
#else
	#asm
		._sa2_l0_mus
			BINARY "../ogt/level3.mus.bin"
		._sa2_l1_mus
			BINARY "../ogt/level4.mus.bin"
		._sa2_l2_mus
			BINARY "../ogt/level5.mus.bin"
		._sa2_cut_mus
			BINARY "../ogt/ending.mus.bin"
	#endasm
#endif
#asm
	._wyz_songs
		defw 	_sa2_title_mus, _sa2_l0_mus, _sa2_gover_mus, _sa2_level_mus
		defw 	_sa2_l0_mus, _sa2_l1_mus, _sa2_l2_mus, _sa2_cut_mus
#endasm
