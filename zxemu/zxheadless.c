/* zxheadless - minimal headless ZX Spectrum 48K for testing z88dk games.
 *
 * No ROM: the ROM area is filled with RET (0xC9) and 0x38 holds EI;RET, so
 * code that only calls the ROM incidentally (z88dk's crt0 does CALL 5633)
 * keeps working. Programs are loaded straight into RAM and started like
 * "CLEAR sp: RANDOMIZE USR pc".
 *
 *   zxheadless [options]
 *     -l file@addr         load a binary (repeatable; e.g. a .scr @16384)
 *     -pc addr -sp addr    entry point and stack (default 24200 / 24199)
 *     -f N                 frames to run (50 per second)
 *     -k F1-F2:keys        hold keys during frames F1..F2 (repeatable)
 *                          keys: letters, digits, SPACE, ENTER, CAPS, SYM, comma-separated
 *     -s F:file.ppm        screenshot after frame F (repeatable)
 *     -every N:prefix      screenshot every N frames to prefixNNNNN.ppm
 *     -poke addr=val       poke a byte before starting (repeatable)
 *     -dump addr:len@F     hex dump memory after frame F (repeatable)
 *     -pokeat F:addr=val   poke a byte at the start of frame F (repeatable)
 *     -prof                print a histogram of PC (hot spots) at the end
 *     -border              log border colour changes per frame (OUT 254)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "z80.h"

#define FRAME_T 69888

static uint8_t mem [65536];
static uint8_t keyrows [8];            /* bit = 1 pressed; row index per half-row */
static uint8_t border;
static long frame;

typedef struct { long f1, f2; int row [8]; } KeySpan;
static KeySpan spans [256]; static int nspans;
typedef struct { long f; char file [256]; } Shot;
static Shot shots [256]; static int nshots;
typedef struct { long f; int addr, len; } Dump;
static Dump dumps [256]; static int ndumps;
typedef struct { long f; int addr, val; } PokeAt;
static PokeAt pokes [256]; static int npokes;
static long every; static char every_prefix [256];
static unsigned long *prof;

/* half-rows: index = bit position of the zero in the high byte of the port */
static const char *rows [8][5] = {
	{ "CAPS", "Z", "X", "C", "V" }, { "A", "S", "D", "F", "G" }, { "Q", "W", "E", "R", "T" },
	{ "1", "2", "3", "4", "5" }, { "0", "9", "8", "7", "6" }, { "P", "O", "I", "U", "Y" },
	{ "ENTER", "L", "K", "J", "H" }, { "SPACE", "SYM", "M", "N", "B" }
};

static int find_key (const char *k, int *row, int *bit) {
	for (int r = 0; r < 8; r ++) for (int b = 0; b < 5; b ++)
		if (!strcasecmp (k, rows [r][b])) { *row = r; *bit = b; return 1; }
	return 0;
}

static uint8_t port_in (uint16_t port) {
	if ((port & 0xff) == 0x1f) return 0;              /* Kempston: nothing pressed */
	if (port & 1) return 0xff;                         /* floating bus */
	uint8_t v = 0x1f;
	for (int r = 0; r < 8; r ++)
		if (!(port & (0x100 << r))) v &= ~keyrows [r];
	return v | 0xa0;
}

static void port_out (uint16_t port, uint8_t v) {
	if (!(port & 1)) border = v & 7;
}

static void screenshot (const char *file) {
	static const uint8_t pal [16][3] = {
		{0,0,0},{0,0,192},{192,0,0},{192,0,192},{0,192,0},{0,192,192},{192,192,0},{192,192,192},
		{0,0,0},{0,0,255},{255,0,0},{255,0,255},{0,255,0},{0,255,255},{255,255,0},{255,255,255} };
	FILE *f = fopen (file, "wb");
	if (!f) { perror (file); return; }
	fprintf (f, "P6\n256 192\n255\n");
	for (int y = 0; y < 192; y ++) for (int x = 0; x < 256; x ++) {
		int addr = 16384 | ((y & 0xc0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2) | (x >> 3);
		uint8_t att = mem [22528 + (y >> 3) * 32 + (x >> 3)];
		int on = (mem [addr] >> (7 - (x & 7))) & 1;
		int c = (on ? (att & 7) : ((att >> 3) & 7)) | ((att & 64) ? 8 : 0);
		fwrite (pal [c], 1, 3, f);
	}
	fclose (f);
}

int main (int argc, char **argv) {
	Z80 z; memset (&z, 0, sizeof z);
	long frames = 100; int pc = 24200, sp = 24199, do_prof = 0, log_border = 0;
	int fprof_addr = -1, f_in = 0, f_sp0 = 0; long long f_t0 = 0, f_tot = 0, f_calls = 0, tall = 0;

	memset (mem, 0xc9, 16384);
	mem [0x38] = 0xfb; mem [0x39] = 0xc9;
	z.mem = mem; z.in = port_in; z.out = port_out;
	z80_reset (&z);

	for (int i = 1; i < argc; i ++) {
		char *a = argv [i];
		if (!strcmp (a, "-l") && i + 1 < argc) {
			char *spec = argv [++ i], *at = strrchr (spec, '@');
			if (!at) { fprintf (stderr, "-l file@addr\n"); return 1; }
			*at = 0; int addr = strtol (at + 1, 0, 0);
			FILE *f = fopen (spec, "rb"); if (!f) { perror (spec); return 1; }
			size_t n = fread (mem + addr, 1, 65536 - addr, f); fclose (f);
			fprintf (stderr, "loaded %s: %zu bytes @ %d\n", spec, n, addr);
		} else if (!strcmp (a, "-pc")) pc = strtol (argv [++ i], 0, 0);
		else if (!strcmp (a, "-sp")) sp = strtol (argv [++ i], 0, 0);
		else if (!strcmp (a, "-f")) frames = strtol (argv [++ i], 0, 0);
		else if (!strcmp (a, "-prof")) do_prof = 1;
		else if (!strcmp (a, "-fprof") && i + 1 < argc) fprof_addr = strtol (argv [++ i], 0, 0) & 0xffff;
		else if (!strcmp (a, "-border")) log_border = 1;
		else if (!strcmp (a, "-k")) {
			KeySpan *k = &spans [nspans ++]; memset (k, 0, sizeof *k);
			char buf [512]; strncpy (buf, argv [++ i], 511); buf [511] = 0;
			char *colon = strchr (buf, ':'); *colon = 0;
			sscanf (buf, "%ld-%ld", &k->f1, &k->f2);
			for (char *t = strtok (colon + 1, ","); t; t = strtok (0, ",")) {
				int r, b; if (!find_key (t, &r, &b)) { fprintf (stderr, "unknown key %s\n", t); return 1; }
				k->row [r] |= 1 << b;
			}
		} else if (!strcmp (a, "-s")) {
			Shot *s = &shots [nshots ++]; char *spec = argv [++ i], *colon = strchr (spec, ':');
			*colon = 0; s->f = atol (spec); strncpy (s->file, colon + 1, 255);
		} else if (!strcmp (a, "-every")) {
			char *spec = argv [++ i], *colon = strchr (spec, ':'); *colon = 0;
			every = atol (spec); strncpy (every_prefix, colon + 1, 255);
		} else if (!strcmp (a, "-poke")) {
			int ad, v; sscanf (argv [++ i], "%i=%i", &ad, &v); mem [ad] = v;
		} else if (!strcmp (a, "-pokeat")) {
			PokeAt *p = &pokes [npokes ++]; sscanf (argv [++ i], "%ld:%i=%i", &p->f, &p->addr, &p->val);
		} else if (!strcmp (a, "-dump")) {
			Dump *d = &dumps [ndumps ++]; sscanf (argv [++ i], "%i:%i@%ld", &d->addr, &d->len, &d->f);
		} else { fprintf (stderr, "unknown option %s\n", a); return 1; }
	}

	if (do_prof) prof = calloc (65536, sizeof *prof);
	z.rom_protect = 1;
	z.pc = pc; z.sp = sp; z.iy = 23610; z.im = 1; z.iff1 = z.iff2 = 1;
	z.sp -= 2; mem [z.sp] = 0; mem [z.sp + 1] = 0;     /* return to 0 (RET loop) */

	for (frame = 0; frame < frames; frame ++) {
		memset (keyrows, 0, sizeof keyrows);
		for (int k = 0; k < nspans; k ++)
			if (frame >= spans [k].f1 && frame <= spans [k].f2)
				for (int r = 0; r < 8; r ++) keyrows [r] |= spans [k].row [r];
		for (int k = 0; k < npokes; k ++) if (pokes [k].f == frame) mem [pokes [k].addr & 0xffff] = pokes [k].val;
		uint8_t b0 = border;
		long t = z80_interrupt (&z, 0xff);
		tall += t;
		while (t < FRAME_T) {
			if (prof) prof [z.pc] ++;
			if (fprof_addr >= 0 && !f_in && z.pc == fprof_addr) { f_in = 1; f_sp0 = z.sp; f_t0 = tall; }
			int dt = z80_step (&z);
			t += dt; tall += dt;
			if (f_in && z.sp > f_sp0) { f_in = 0; f_tot += tall - f_t0; f_calls ++; }
		}
		if (log_border && border != b0) fprintf (stderr, "frame %ld border %d\n", frame, border);
		for (int s = 0; s < nshots; s ++) if (shots [s].f == frame) screenshot (shots [s].file);
		if (every && frame % every == 0) {
			char fn [300]; snprintf (fn, sizeof fn, "%s%05ld.ppm", every_prefix, frame); screenshot (fn);
		}
		for (int d = 0; d < ndumps; d ++) if (dumps [d].f == frame) {
			printf ("frame %ld @%d:", frame, dumps [d].addr);
			for (int j = 0; j < dumps [d].len; j ++) printf (" %02x", mem [(dumps [d].addr + j) & 0xffff]);
			printf ("\n");
		}
	}
	fprintf (stderr, "ran %ld frames, PC=%04x SP=%04x IM=%d IFF=%d halted=%d\n", frames, z.pc, z.sp, z.im, z.iff1, z.halted);

	if (fprof_addr >= 0)
		printf ("fprof %04x: %lld calls, %lld T total, %lld T per call\n", fprof_addr, f_calls, f_tot, f_calls ? f_tot / f_calls : 0);
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
