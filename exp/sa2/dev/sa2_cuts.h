// Sir Ababol 2 - cutscenes (mode 1): the frame, the load's two picture windows
// (16 x 9 cells), the font and the texts (the original's cutscene.h: @ is
// a line break). Load 1: the intro (texts 1-4); load 2: the ending (5-8).

extern unsigned char cut_frame [], cut_font [], cut_win_a [], cut_win_b [], cut_texts [];
#asm
	._cut_frame
		BINARY "cpc/cut_frame.zx0"
	._cut_font
		BINARY "cpc/cut_font.zx0"
#endasm
#if SA2_LOAD == 1
	#asm
		._cut_win_a
			BINARY "cpc/cut_win1.zx0"
		._cut_win_b
			BINARY "cpc/cut_win2.zx0"
		._cut_texts
			defm "I'M ALASTOR DE LA BABUCHA@THE EVIL DUKE-MARQUIS@FROM FREEZIA & FRIDGELAND@(PROVINCE OF BADAJOZ)"
			defb 0
			defm "'I JUST ABDUCTED YOU!@I NEED THE PRINCE JUICE@WHICH CAN BE EXTRACTED@FROM OUR PRINCELY BODIES,'"
			defb 0
			defm "'SO I WILL TURN YOU INTO@UGLY. FIENDISH BATS.@HAIRY AND DISGUSTING.@SMELLY AND DIRTY,'"
			defb 0
			defm "'AND I WILL PLACE THOU@OVER THE 6 TOWERS@IN MY ICE PALACE@SO YOU CAN HOVER AROUND'"
			defb 0
	#endasm
#else
	#asm
		._cut_win_a
			BINARY "cpc/cut_win3.zx0"
		._cut_win_b
			BINARY "cpc/cut_win4.zx0"
		._cut_texts
			defm "'SIR ABABOL! YOU RUINED@MY PERFECT PLANS!@WHY DIDN'T YOU JUST@STAY AT HOME?!'"
			defb 0
			defm "'HAVE THIS! FEEL MY@SWORDY SWORD ON YOUR BONG!'@ONCE AGAIN. SIR ABABOL@MADE THE GRADE!"
			defb 0
			defm "'STOP COCKING AROUND!@AND QUICK!@GET ON MY FLYIN' CAR!@GOING BACK TO THE FUTURE!"
			defb 0
			defm "'BUT WHAT 'BOUT MY HORSE?'@ASKED SIR ABABOL@'WHERE WE ARE GOING,,,@YOU NEED NO HORSE,,,'"
			defb 0
	#endasm
#endif
