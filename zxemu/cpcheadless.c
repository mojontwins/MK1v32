/* cpcheadless - minimal headless Amstrad CPC 464 for testing z88dk / MK1 games.
 *
 * Loads a .sna (v1-v3) or a raw binary, runs it line by line (312 lines of
 * 64 us per frame, instructions rounded up to whole NOPs), and emulates just
 * enough hardware for games that switch the firmware off:
 *
 *   - Gate Array: pen/ink selection, mode, ROM config bits (ROMs are NOT
 *     emulated: a warning is printed if the game enables them), the 52-line
 *     interrupt counter with VSYNC resync and acknowledge.
 *   - CRTC 6845: registers; the screen is rendered from R1, R6, R9, R12/R13,
 *     VSYNC from R7 / R3 (type 0-ish, no mid-frame tricks beyond the line).
 *   - PPI 8255 + AY: VSYNC on port B bit 0, keyboard via PSG register 14.
 *   - Per-line inks and mode are recorded, so raster splits show correctly.
 *
 * Options:
 *   -sna file.sna            load a snapshot
 *   -l file@addr -pc addr    load a raw binary and start there (SP = 0xC000)
 *   -f N                     frames (50 per second)
 *   -k F1-F2:keys            hold keys during frames F1..F2 (comma-separated:
 *                            A..Z 0..9 SPACE ENTER RETURN ESC TAB SHIFT CTRL
 *                            COPY DEL UP DOWN LEFT RIGHT F0..F9 J_UP J_DOWN
 *                            J_LEFT J_RIGHT J_FIRE1 J_FIRE2)
 *   -s F:file.ppm            screenshot after frame F
 *   -every N:prefix          screenshot every N frames
 *   -pokeat F:addr=val       poke a byte at the start of frame F
 *   -dump addr:len@F         hex dump after frame F
 *   -mark addr               log "frame line nop" each time PC reaches addr
 *   -scrwrites F1-F2         per frame, report screen writes that hit the
 *                            character row the beam is drawing (tearing risk)
 *   -prof                    PC histogram
 *   -beam                    screenshots show each line as the beam drew it
 *                            (latched at the start of the line) instead of
 *                            screen memory at the end of the frame, so a
 *                            render that races the beam shows its tearing
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "z80.h"

#define LINES 312
#define NOPS_PER_LINE 64

static uint8_t mem [65536];
static Z80 z;

/* Gate Array */
static uint8_t ga_pen, ga_ink [17], ga_mode, ga_rom, ga_counter, int_pending;
static int vsync_lines_ago = -1;
/* CRTC */
static uint8_t crtc_sel, crtc [18];
/* PPI / PSG */
static uint8_t ppi_a, ppi_c, ppi_ctrl, psg_sel, psg [16];
static uint8_t keyrow [10];          /* bit = 1 pressed */
static int in_vsync;
static int cur_line;
static long frame;
static int rom_warned;

/* per-line raster state for rendering */
static uint8_t line_ink [LINES][17], line_mode [LINES];
static int beam_mode;
static uint8_t beam_buf [LINES][192];

/* options */
typedef struct { long f1, f2; uint8_t row [10]; } KeySpan;
static KeySpan spans [256]; static int nspans;
typedef struct { long f; char file [256]; } Shot;
static Shot shots [256]; static int nshots;
typedef struct { long f; int addr, len; } Dump;
static Dump dumps [4096]; static int ndumps;
typedef struct { long f; int addr, val; } PokeAt;
static PokeAt pokes [256]; static int npokes;
static int marks [64], nmarks;
static long every; static char every_prefix [256];
static long sw_f1 = -1, sw_f2 = -1;
static long sw_total, sw_hit;
static unsigned long *prof;

static const uint8_t hw_rgb [32][3] = {
	{128,128,128},{128,128,128},{0,255,128},{255,255,128},{0,0,128},{255,0,128},{0,128,128},{255,128,128},
	{255,0,128},{255,255,128},{255,255,0},{255,255,255},{255,0,0},{255,0,255},{255,128,0},{255,128,255},
	{0,0,128},{0,255,128},{0,255,0},{0,255,255},{0,0,0},{0,0,255},{0,128,0},{0,128,255},
	{128,0,128},{128,255,128},{128,255,0},{128,255,255},{128,0,0},{128,0,255},{128,128,0},{128,128,255} };

static const char *keynames [10][8] = {
	{ "UP", "RIGHT", "DOWN", "F9", "F6", "F3", "ENTER", "F." },
	{ "LEFT", "COPY", "F7", "F8", "F5", "F1", "F2", "F0" },
	{ "CLR", "[", "RETURN", "]", "F4", "SHIFT", "\\", "CTRL" },
	{ "^", "-", "@", "P", ";", ":", "/", "." },
	{ "0", "9", "O", "I", "L", "K", "M", "," },
	{ "8", "7", "U", "Y", "H", "J", "N", "SPACE" },
	{ "6", "5", "R", "T", "G", "F", "B", "V" },
	{ "4", "3", "E", "W", "S", "D", "C", "X" },
	{ "1", "2", "ESC", "Q", "TAB", "A", "CAPS", "Z" },
	{ "J_UP", "J_DOWN", "J_LEFT", "J_RIGHT", "J_FIRE2", "J_FIRE1", "J_FIRE3", "DEL" } };

/* ---------------------------------------------------------------- screen */

static int disp_lines (void) { return crtc [6] * (crtc [9] + 1); }
static int vsync_line (void) { return crtc [7] * (crtc [9] + 1); }

/* Address of byte `b` (0..2*R1-1) of display line `y` */
static uint16_t scr_addr (int y, int b) {
	int rl = crtc [9] + 1;
	int ma = ((crtc [12] << 8) | crtc [13]) + (y / rl) * crtc [1] + (b >> 1);
	int ra = y % rl;
	return (uint16_t) (((ma & 0x3000) << 2) | ((ra & 7) << 11) | ((ma & 0x3ff) << 1) | (b & 1));
}

static void screenshot (const char *file) {
	int w = crtc [1] * 8, h = disp_lines ();
	if (w <= 0 || w > 768) w = 320;
	if (h <= 0 || h > LINES) h = 200;
	FILE *f = fopen (file, "wb"); if (!f) { perror (file); return; }
	fprintf (f, "P6\n%d %d\n255\n", w, h);
	for (int y = 0; y < h; y ++) {
		int mode = line_mode [y] & 3;
		for (int x = 0; x < w; x ++) {
			int b = x >> 2, sub = x & 3, pen;
			uint8_t v = beam_mode ? beam_buf [y][b] : mem [scr_addr (y, b)];
			if (mode == 0) {
				int p = sub >> 1;   /* 2 pixels per byte, each 2 output px wide */
				pen = ((v >> (7 - p)) & 1) | (((v >> (3 - p)) & 1) << 1) | (((v >> (5 - p)) & 1) << 2) | (((v >> (1 - p)) & 1) << 3);
			} else if (mode == 1) {
				pen = ((v >> (7 - sub)) & 1) | (((v >> (3 - sub)) & 1) << 1);
			} else {
				pen = (v >> (7 - sub * 2)) & 1;
			}
			fwrite (hw_rgb [line_ink [y][pen] & 31], 1, 3, f);
		}
	}
	fclose (f);
}

/* ---------------------------------------------------------------- ports */

static uint8_t port_in (uint16_t port) {
	uint8_t v = 0xff;
	if (!(port & 0x0800)) {                          /* PPI */
		switch ((port >> 8) & 3) {
			case 0:                                  /* port A: PSG data */
				if ((ppi_c >> 6) == 1) {
					if (psg_sel == 14) {
						int row = ppi_c & 15;
						v = row < 10 ? (uint8_t) ~keyrow [row] : 0xff;
					} else v = psg [psg_sel & 15];
				}
				break;
			case 1:                                  /* port B */
				/* bit 0 VSYNC, 1-3 brand (7 = Amstrad), 4 50 Hz, 5 no expansion, 6 printer not busy */
				v = (in_vsync ? 1 : 0) | 0x7e;
				break;
			case 2: v = ppi_c; break;
		}
	} else if (!(port & 0x4000)) {                   /* CRTC */
		if (((port >> 8) & 3) == 3) v = crtc [crtc_sel & 31 & 15];
	}
	return v;
}

static void psg_bus (void) {
	switch (ppi_c >> 6) {
		case 3: psg_sel = ppi_a & 15; break;
		case 2: psg [psg_sel & 15] = ppi_a; break;
	}
}

static void port_out (uint16_t port, uint8_t v) {
	if ((port & 0xc000) == 0x4000) {                 /* Gate Array */
		switch (v >> 6) {
			case 0: ga_pen = (v & 0x10) ? 16 : (v & 15); break;
			case 1: ga_ink [ga_pen] = v & 31; break;
			case 2:
				ga_mode = v & 3;
				ga_rom = (v >> 2) & 3;
				if (ga_rom != 3 && !rom_warned) {
					fprintf (stderr, "warning: frame %ld: game enabled a ROM (%02x) - ROMs are not emulated\n", frame, v);
					rom_warned = 1;
				}
				if (v & 0x10) { ga_counter = 0; int_pending = 0; }
				break;
		}
	}
	if (!(port & 0x4000)) {                          /* CRTC */
		switch ((port >> 8) & 3) {
			case 0: crtc_sel = v & 31; break;
			case 1: if (crtc_sel < 18) crtc [crtc_sel] = v; break;
		}
	}
	if (!(port & 0x0800)) {                          /* PPI */
		switch ((port >> 8) & 3) {
			case 0: ppi_a = v; psg_bus (); break;
			case 2: ppi_c = v; psg_bus (); break;
			case 3:
				if (v & 0x80) ppi_ctrl = v;
				else {                               /* bit set/reset on port C */
					int bit = (v >> 1) & 7;
					if (v & 1) ppi_c |= 1 << bit; else ppi_c &= ~(1 << bit);
					psg_bus ();
				}
				break;
		}
	}
}

/* screen writes vs beam */
static void wr_hook (uint16_t a, uint8_t v) {
	(void) v;
	if (frame < sw_f1 || frame > sw_f2) return;
	if (a < 0xc000) return;
	/* which display line shows this byte? (standard 16K screen at C000) */
	int ra = (a >> 11) & 7;
	int off = (a & 0x7ff) >> 1;
	int base = ((crtc [12] & 3) << 8 | crtc [13]) & 0x3ff;
	int rowpos = (off - base) & 0x3ff;
	int row = crtc [1] ? rowpos / crtc [1] : 0;
	int y = row * (crtc [9] + 1) + ra;
	if (y >= disp_lines ()) return;
	sw_total ++;
	/* risky if the beam is within the same character row right now */
	if (cur_line >= row * (crtc [9] + 1) && cur_line < (row + 1) * (crtc [9] + 1)) sw_hit ++;
}

/* ---------------------------------------------------------------- loading */

static int load_sna (const char *file) {
	FILE *f = fopen (file, "rb"); if (!f) { perror (file); return 0; }
	uint8_t h [256];
	if (fread (h, 1, 256, f) != 256 || memcmp (h, "MV - SNA", 8)) { fprintf (stderr, "%s: not a SNA\n", file); fclose (f); return 0; }
	size_t n = fread (mem, 1, 65536, f); fclose (f);
	z.f = h [0x11]; z.a = h [0x12]; z.c = h [0x13]; z.b = h [0x14]; z.e = h [0x15]; z.d = h [0x16];
	z.l = h [0x17]; z.h = h [0x18]; z.r = h [0x19]; z.i = h [0x1a]; z.iff1 = h [0x1b]; z.iff2 = h [0x1c];
	z.ix = h [0x1d] | (h [0x1e] << 8); z.iy = h [0x1f] | (h [0x20] << 8);
	z.sp = h [0x21] | (h [0x22] << 8); z.pc = h [0x23] | (h [0x24] << 8); z.im = h [0x25];
	z.f_ = h [0x26]; z.a_ = h [0x27]; z.c_ = h [0x28]; z.b_ = h [0x29]; z.e_ = h [0x2a]; z.d_ = h [0x2b];
	z.l_ = h [0x2c]; z.h_ = h [0x2d];
	ga_pen = h [0x2e]; memcpy (ga_ink, h + 0x2f, 17);
	ga_mode = h [0x40] & 3; ga_rom = (h [0x40] >> 2) & 3;
	crtc_sel = h [0x42]; memcpy (crtc, h + 0x43, 18);
	ppi_a = h [0x56]; ppi_c = h [0x58]; ppi_ctrl = h [0x59];
	psg_sel = h [0x5a]; memcpy (psg, h + 0x5b, 16);
	fprintf (stderr, "loaded %s: v%d, %zu bytes RAM, PC=%04x\n", file, h [0x10], n, z.pc);
	return 1;
}

static void default_crtc (void) {
	static const uint8_t r [18] = { 63, 40, 46, 142, 38, 0, 25, 30, 0, 7, 0, 0, 48, 0, 192, 0, 0, 0 };
	memcpy (crtc, r, 18);
	ga_rom = 3; ga_mode = 1;
	for (int i = 0; i < 17; i ++) ga_ink [i] = 20;
}

/* ---------------------------------------------------------------- main */

int main (int argc, char **argv) {
	long frames = 100; int do_prof = 0, loaded = 0;
	memset (&z, 0, sizeof z);
	z.mem = mem; z.in = port_in; z.out = port_out;
	z80_reset (&z);
	default_crtc ();

	for (int i = 1; i < argc; i ++) {
		char *a = argv [i];
		if (!strcmp (a, "-sna")) { if (!load_sna (argv [++ i])) return 1; loaded = 1; }
		else if (!strcmp (a, "-l")) {
			char *spec = argv [++ i], *at = strrchr (spec, '@'); *at = 0;
			int addr = strtol (at + 1, 0, 0);
			FILE *f = fopen (spec, "rb"); if (!f) { perror (spec); return 1; }
			size_t n = fread (mem + addr, 1, 65536 - addr, f); fclose (f);
			fprintf (stderr, "loaded %s: %zu bytes @ %04x\n", spec, n, addr);
			loaded = 1;
		}
		else if (!strcmp (a, "-pc")) { z.pc = strtol (argv [++ i], 0, 0); z.sp = 0xc000; z.im = 1; }
		else if (!strcmp (a, "-f")) frames = atol (argv [++ i]);
		else if (!strcmp (a, "-prof")) do_prof = 1;
		else if (!strcmp (a, "-beam")) beam_mode = 1;
		else if (!strcmp (a, "-k")) {
			KeySpan *k = &spans [nspans ++]; memset (k, 0, sizeof *k);
			char buf [512]; strncpy (buf, argv [++ i], 511); buf [511] = 0;
			char *colon = strchr (buf, ':'); *colon = 0;
			sscanf (buf, "%ld-%ld", &k->f1, &k->f2);
			for (char *t = strtok (colon + 1, ","); t; t = strtok (0, ",")) {
				int found = 0;
				for (int r = 0; r < 10 && !found; r ++) for (int b = 0; b < 8; b ++)
					if (!strcasecmp (t, keynames [r][b])) { k->row [r] |= 1 << b; found = 1; break; }
				if (!found) { fprintf (stderr, "unknown key %s\n", t); return 1; }
			}
		}
		else if (!strcmp (a, "-s")) {
			Shot *s = &shots [nshots ++]; char *spec = argv [++ i], *c = strchr (spec, ':');
			*c = 0; s->f = atol (spec); strncpy (s->file, c + 1, 255);
		}
		else if (!strcmp (a, "-every")) {
			char *spec = argv [++ i], *c = strchr (spec, ':'); *c = 0;
			every = atol (spec); strncpy (every_prefix, c + 1, 255);
		}
		else if (!strcmp (a, "-pokeat")) { PokeAt *p = &pokes [npokes ++]; sscanf (argv [++ i], "%ld:%i=%i", &p->f, &p->addr, &p->val); }
		else if (!strcmp (a, "-dump")) { Dump *d = &dumps [ndumps ++]; sscanf (argv [++ i], "%i:%i@%ld", &d->addr, &d->len, &d->f); }
		else if (!strcmp (a, "-mark")) marks [nmarks ++] = strtol (argv [++ i], 0, 0);
		else if (!strcmp (a, "-scrwrites")) sscanf (argv [++ i], "%ld-%ld", &sw_f1, &sw_f2);
		else { fprintf (stderr, "unknown option %s\n", a); return 1; }
	}
	if (!loaded) { fprintf (stderr, "nothing loaded (-sna or -l)\n"); return 1; }
	if (do_prof) prof = calloc (65536, sizeof *prof);
	if (sw_f1 >= 0) z.wr_hook = wr_hook;

	for (frame = 0; frame < frames; frame ++) {
		memset (keyrow, 0, sizeof keyrow);
		for (int k = 0; k < nspans; k ++)
			if (frame >= spans [k].f1 && frame <= spans [k].f2)
				for (int r = 0; r < 10; r ++) keyrow [r] |= spans [k].row [r];
		for (int k = 0; k < npokes; k ++) if (pokes [k].f == frame) mem [pokes [k].addr & 0xffff] = pokes [k].val;
		long fw_total = sw_total, fw_hit = sw_hit;

		int vs = vsync_line (), vs_len = (crtc [3] >> 4) ? (crtc [3] >> 4) : 16;
		for (cur_line = 0; cur_line < LINES; cur_line ++) {
			/* raster state seen by this line */
			memcpy (line_ink [cur_line], ga_ink, 17); line_mode [cur_line] = ga_mode;
			if (beam_mode && crtc [1] * 2 <= 192)
				for (int b = 0; b < crtc [1] * 2; b ++) beam_buf [cur_line][b] = mem [scr_addr (cur_line, b)];

			int was = in_vsync;
			in_vsync = cur_line >= vs && cur_line < vs + vs_len;
			if (in_vsync && !was) vsync_lines_ago = 0;

			int nops = 0;
			while (nops < NOPS_PER_LINE) {
				if (int_pending && z.iff1 && !z.ei_delay) {
					int t = z80_interrupt (&z, 0xff);
					if (t) { int_pending = 0; ga_counter &= 0x1f; nops += (t + 3) / 4; continue; }
				}
				if (nmarks) for (int m = 0; m < nmarks; m ++)
					if (z.pc == marks [m] && !z.halted) printf ("mark %04x frame %ld line %d nop %d\n", marks [m], frame, cur_line, nops);
				if (prof) prof [z.pc] ++;
				int t = z80_step (&z);
				nops += (t + 3) / 4;
			}

			/* HSYNC: interrupt counter */
			ga_counter ++;
			if (ga_counter >= 52) { ga_counter = 0; int_pending = 1; }
			if (vsync_lines_ago >= 0) {
				vsync_lines_ago ++;
				if (vsync_lines_ago == 2) {
					if (ga_counter >= 32) int_pending = 1;
					ga_counter = 0;
					vsync_lines_ago = -1;
				}
			}
		}

		for (int s = 0; s < nshots; s ++) if (shots [s].f == frame) screenshot (shots [s].file);
		if (every && frame % every == 0) { char fn [300]; snprintf (fn, sizeof fn, "%s%05ld.ppm", every_prefix, frame); screenshot (fn); }
		for (int d = 0; d < ndumps; d ++) if (dumps [d].f == frame) {
			printf ("frame %ld @%d:", frame, dumps [d].addr);
			for (int j = 0; j < dumps [d].len; j ++) printf (" %02x", mem [(dumps [d].addr + j) & 0xffff]);
			printf ("\n");
		}
		if (sw_f1 >= 0 && frame >= sw_f1 && frame <= sw_f2)
			printf ("scrwrites frame %ld: %ld writes, %ld in the row being displayed\n", frame, sw_total - fw_total, sw_hit - fw_hit);
	}
	fprintf (stderr, "ran %ld frames, PC=%04x SP=%04x IM=%d IFF=%d mode=%d\n", frames, z.pc, z.sp, z.im, z.iff1, ga_mode);
	if (prof) {
		for (int n = 0; n < 15; n ++) {
			unsigned long best = 0; int bi = 0;
			for (int a = 0; a < 65536; a ++) if (prof [a] > best) { best = prof [a]; bi = a; }
			if (!best) break;
			printf ("%04x %lu\n", bi, best); prof [bi] = 0;
		}
	}
	return 0;
}
