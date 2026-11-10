// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// custom.h - Camp Escape
//
// 1. The camp clock and the daily routine (the heart of The Great Escape):
//    every HOUR_FRAMES game frames the hour in flags [F_CLOCK] advances. Some
//    hours have a compulsory place to be:
//
//        08, 14, 20   ROLL CALL   parade ground   (screen 14)
//        09           BREAKFAST   mess hall       (screen 20)
//        21 .. 06     LIGHTS OUT  your hut        (screen 18, the night
//                                                  passes 4x faster there)
//
//    flags [F_EVENT] holds the screen where the prisoner must be (255 = free
//    time). After GRACE_FRAMES, being anywhere else raises flags [F_ALARM]
//    and rings the bell. While the alarm is on, any guard or dog that
//    touches you arrests you (that rule lives in the script, ENEMY TOUCHED).
//    Being back where you should be silences the alarm.
//
// 2. Guards never "kill": this_enemy_kills () returns 0, so the engine just
//    calls the script's ENEMY TOUCHED section and lets it decide.

// Flags shared with script/script.spt (keep the ALIASes in sync!)
#define F_CLOCK 		2
#define F_ALARM 		3
#define F_EVENT 		5

#define SCR_PARADE 		14
#define SCR_MESS 		20
#define SCR_HUT 		18
#define SCR_CELL 		29

#define HOUR_FRAMES 	250			// ~10 s per camp hour at 25 fps
#define GRACE_FRAMES 	150			// time to get there before the alarm

unsigned int cl_ticks;
unsigned char cl_hour;
unsigned char cl_cur;				// current routine message (0..3)
unsigned char cl_msg;
unsigned char cl_shown;

char *cl_names [] = {
	"FREE TIME ",
	"ROLL CALL ",
	"BREAKFAST ",
	"LIGHTS OUT",
	"*ALARM!*  "
};

void routine_for_hour (void) {
	cl_hour = flags [F_CLOCK];
	if (cl_hour == 8 || cl_hour == 14 || cl_hour == 20) {
		flags [F_EVENT] = SCR_PARADE; cl_cur = 1;
	} else if (cl_hour == 9) {
		flags [F_EVENT] = SCR_MESS; cl_cur = 2;
	} else if (cl_hour >= 21 || cl_hour < 7) {
		flags [F_EVENT] = SCR_HUT; cl_cur = 3;
	} else {
		flags [F_EVENT] = 255; cl_cur = 0;
		flags [F_ALARM] = 0;
	}
}

// Called from the script (EXTERN 0) when a new game starts.
void clock_reset (void) {
	cl_ticks = 0;
	cl_shown = 0xff;
	routine_for_hour ();
}

void before_vsync (void) {
	// Advance the clock. Asleep in the hut at night, time flies (x4).
	cl_ticks ++;
	if (flags [F_EVENT] == SCR_HUT && n_pant == SCR_HUT) cl_ticks += 3;
	if (cl_ticks >= HOUR_FRAMES) {
		cl_ticks = 0;
		cl_hour = flags [F_CLOCK] + 1;
		if (cl_hour == 24) cl_hour = 0;
		flags [F_CLOCK] = cl_hour;
		routine_for_hour ();
		if (flags [F_EVENT] != 255) peta_el_beeper (8);		// the camp bell
	}

	// Where should the prisoner be?
	if (flags [F_EVENT] != 255 && cl_ticks > GRACE_FRAMES && n_pant != SCR_CELL) {
		if (n_pant != flags [F_EVENT]) {
			if (flags [F_ALARM] == 0) {
				flags [F_ALARM] = 1;
				peta_el_beeper (6);
			}
		} else {
			flags [F_ALARM] = 0;
		}
	}

	// Routine / alarm message in the HUD (only when it changes)
	cl_msg = flags [F_ALARM] ? 4 : cl_cur;
	if (cl_msg != cl_shown) {
		draw_text (1, 23, cl_msg == 4 ? 66 : 70, cl_names [cl_msg]);
		cl_shown = cl_msg;
	}
}

#ifdef ENABLE_CUSTOM_ENEMS

	void extra_enems_init (void) {
	}

	void extra_enems_move (void) {
	}

	void extra_enems_checks (void) {
	}

	void extra_enems_killed (void) {
	}

	unsigned char this_enemy_kills (void) {
		// Nobody hurts the prisoner: the script (ENEMY TOUCHED) decides
		// whether a contact is an arrest, a bribe, a fed dog or nothing.
		return 0;
	}

#endif
