/* z80.c - compact Z80 core for zxheadless.
   Decoding by x/y/z/p/q fields, DD/FD via an index mode, CB/DDCB/FDCB/ED.
   Flags (incl. undocumented X/Y) follow the usual references closely enough
   for game code; timings are the standard uncontended T-states. */
#include "z80.h"
#include <string.h>

#define FC 0x01
#define FN 0x02
#define FP 0x04
#define FX 0x08
#define FH 0x10
#define FY 0x20
#define FZ 0x40
#define FS 0x80

static uint8_t sz53[256], sz53p[256], parity[256];
static int tables_ready;

static void init_tables (void) {
	for (int i = 0; i < 256; i ++) {
		int p = 0; for (int b = 0; b < 8; b ++) p ^= (i >> b) & 1;
		parity [i] = p ? 0 : FP;
		sz53 [i] = (i & (FS | FX | FY)) | (i ? 0 : FZ);
		sz53p [i] = sz53 [i] | parity [i];
	}
	tables_ready = 1;
}

#define RD(a) (z->mem [(uint16_t) (a)])
static inline void WR (Z80 *z, uint16_t a, uint8_t v) {
	if (a >= 16384 || !z->rom_protect) z->mem [a] = v;
	if (z->wr_hook) z->wr_hook (a, v);
}
static inline uint8_t fetch (Z80 *z) { return RD (z->pc ++); }
static inline uint16_t fetch16 (Z80 *z) { uint16_t v = RD (z->pc) | (RD (z->pc + 1) << 8); z->pc += 2; return v; }
static inline uint16_t rd16 (Z80 *z, uint16_t a) { return RD (a) | (RD (a + 1) << 8); }
static inline void wr16 (Z80 *z, uint16_t a, uint16_t v) { WR (z, a, v & 255); WR (z, a + 1, v >> 8); }
static inline void push (Z80 *z, uint16_t v) { z->sp -= 2; wr16 (z, z->sp, v); }
static inline uint16_t pop (Z80 *z) { uint16_t v = rd16 (z, z->sp); z->sp += 2; return v; }

#define BC ((z->b << 8) | z->c)
#define DE ((z->d << 8) | z->e)
#define HL ((z->h << 8) | z->l)
#define AF ((z->a << 8) | z->f)
#define SETBC(v) do { uint16_t _v = (v); z->b = _v >> 8; z->c = _v; } while (0)
#define SETDE(v) do { uint16_t _v = (v); z->d = _v >> 8; z->e = _v; } while (0)
#define SETHL(v) do { uint16_t _v = (v); z->h = _v >> 8; z->l = _v; } while (0)

void z80_reset (Z80 *z) {
	if (!tables_ready) init_tables ();
	z->a = z->f = 0xff; z->b = z->c = z->d = z->e = z->h = z->l = 0;
	z->ix = z->iy = 0xffff; z->sp = 0xffff; z->pc = 0;
	z->i = z->r = 0; z->iff1 = z->iff2 = 0; z->im = 0; z->halted = 0; z->ei_delay = 0;
	z->cycles = 0;
}

/* ---- ALU ---- */
static inline void add8 (Z80 *z, uint8_t v, int carry) {
	unsigned r = z->a + v + carry;
	z->f = sz53 [r & 255] | (r >> 8 & FC) | ((z->a ^ v ^ r) & FH) | ((((z->a ^ ~v) & (z->a ^ r)) >> 5) & FP);
	z->a = r;
}
static inline void sub8 (Z80 *z, uint8_t v, int carry) {
	unsigned r = z->a - v - carry;
	z->f = sz53 [r & 255] | ((r >> 8) & FC) | FN | ((z->a ^ v ^ r) & FH) | ((((z->a ^ v) & (z->a ^ r)) >> 5) & FP);
	z->a = r;
}
static inline void cp8 (Z80 *z, uint8_t v) {
	unsigned r = z->a - v;
	z->f = (sz53 [r & 255] & ~(FX | FY)) | (v & (FX | FY)) | ((r >> 8) & FC) | FN | ((z->a ^ v ^ r) & FH) | ((((z->a ^ v) & (z->a ^ r)) >> 5) & FP);
}
static inline void alu (Z80 *z, int op, uint8_t v) {
	switch (op) {
		case 0: add8 (z, v, 0); break;
		case 1: add8 (z, v, z->f & FC); break;
		case 2: sub8 (z, v, 0); break;
		case 3: sub8 (z, v, z->f & FC); break;
		case 4: z->a &= v; z->f = sz53p [z->a] | FH; break;
		case 5: z->a ^= v; z->f = sz53p [z->a]; break;
		case 6: z->a |= v; z->f = sz53p [z->a]; break;
		case 7: cp8 (z, v); break;
	}
}
static inline uint8_t inc8 (Z80 *z, uint8_t v) {
	uint8_t r = v + 1;
	z->f = (z->f & FC) | sz53 [r] | ((v & 0x0f) == 0x0f ? FH : 0) | (v == 0x7f ? FP : 0);
	return r;
}
static inline uint8_t dec8 (Z80 *z, uint8_t v) {
	uint8_t r = v - 1;
	z->f = (z->f & FC) | sz53 [r] | FN | ((v & 0x0f) == 0 ? FH : 0) | (v == 0x80 ? FP : 0);
	return r;
}
static inline uint16_t add16 (Z80 *z, uint16_t a, uint16_t b) {
	unsigned r = a + b;
	z->f = (z->f & (FS | FZ | FP)) | ((r >> 16) & FC) | (((a ^ b ^ r) >> 8) & FH) | ((r >> 8) & (FX | FY));
	return r;
}
static inline uint16_t adc16 (Z80 *z, uint16_t a, uint16_t b) {
	unsigned r = a + b + (z->f & FC);
	z->f = ((r >> 16) & FC) | (((a ^ b ^ r) >> 8) & FH) | ((r >> 8) & (FS | FX | FY)) | ((r & 0xffff) ? 0 : FZ)
		| ((((a ^ ~b) & (a ^ r)) >> 13) & FP);
	return r;
}
static inline uint16_t sbc16 (Z80 *z, uint16_t a, uint16_t b) {
	unsigned r = a - b - (z->f & FC);
	z->f = ((r >> 16) & FC) | FN | (((a ^ b ^ r) >> 8) & FH) | ((r >> 8) & (FS | FX | FY)) | ((r & 0xffff) ? 0 : FZ)
		| ((((a ^ b) & (a ^ r)) >> 13) & FP);
	return r;
}
static uint8_t rot (Z80 *z, int op, uint8_t v) {
	uint8_t c;
	switch (op) {
		case 0: c = v >> 7; v = (v << 1) | c; break;                      /* RLC */
		case 1: c = v & 1; v = (v >> 1) | (c << 7); break;                /* RRC */
		case 2: c = v >> 7; v = (v << 1) | (z->f & FC); break;            /* RL */
		case 3: c = v & 1; v = (v >> 1) | ((z->f & FC) << 7); break;      /* RR */
		case 4: c = v >> 7; v <<= 1; break;                               /* SLA */
		case 5: c = v & 1; v = (v >> 1) | (v & 0x80); break;              /* SRA */
		case 6: c = v >> 7; v = (v << 1) | 1; break;                      /* SLL */
		default: c = v & 1; v >>= 1; break;                               /* SRL */
	}
	z->f = sz53p [v] | c;
	return v;
}

/* ---- register access with index mode (0 HL, 1 IX, 2 IY) ---- */
static inline uint8_t *r8p (Z80 *z, int r, int idx) {
	switch (r) {
		case 0: return &z->b; case 1: return &z->c; case 2: return &z->d; case 3: return &z->e;
		case 4: return idx == 1 ? ((uint8_t *) &z->ix) + 1 : idx == 2 ? ((uint8_t *) &z->iy) + 1 : &z->h;
		case 5: return idx == 1 ? (uint8_t *) &z->ix : idx == 2 ? (uint8_t *) &z->iy : &z->l;
		default: return &z->a;
	}
}
static inline uint16_t getHL (Z80 *z, int idx) { return idx == 1 ? z->ix : idx == 2 ? z->iy : HL; }
static inline void setHL (Z80 *z, int idx, uint16_t v) { if (idx == 1) z->ix = v; else if (idx == 2) z->iy = v; else SETHL (v); }
static inline uint16_t getrp (Z80 *z, int p, int idx) {
	switch (p) { case 0: return BC; case 1: return DE; case 2: return getHL (z, idx); default: return z->sp; }
}
static inline void setrp (Z80 *z, int p, int idx, uint16_t v) {
	switch (p) { case 0: SETBC (v); break; case 1: SETDE (v); break; case 2: setHL (z, idx, v); break; default: z->sp = v; }
}
static inline int cond (Z80 *z, int y) {
	switch (y) {
		case 0: return !(z->f & FZ); case 1: return z->f & FZ;
		case 2: return !(z->f & FC); case 3: return z->f & FC;
		case 4: return !(z->f & FP); case 5: return z->f & FP;
		case 6: return !(z->f & FS); default: return z->f & FS;
	}
}

/* little-endian host assumed for the IXH/IXL pointer trick above */

static int exec_cb (Z80 *z, int idx, uint16_t addr) {
	int t;
	uint8_t op = fetch (z);
	int x = op >> 6, y = (op >> 3) & 7, r = op & 7;
	uint8_t v;
	if (idx) {
		v = RD (addr);
		t = 23;
	} else if (r == 6) {
		addr = HL; v = RD (addr); t = 15;
	} else { v = *r8p (z, r, 0); t = 8; }

	switch (x) {
		case 0: v = rot (z, y, v); break;
		case 1:
			z->f = (z->f & FC) | FH | (sz53p [v & (1 << y)] & ~(FX | FY)) | ((idx || r == 6 ? (addr >> 8) : v) & (FX | FY));
			if (y == 7 && (v & 0x80)) z->f |= FS;
			return idx ? 20 : (r == 6 ? 12 : 8);
		case 2: v &= ~(1 << y); break;
		case 3: v |= 1 << y; break;
	}
	if (idx) { WR (z, addr, v); if (r != 6) *r8p (z, r, 0) = v; }
	else if (r == 6) WR (z, addr, v);
	else *r8p (z, r, 0) = v;
	return t;
}

static int exec_ed (Z80 *z) {
	uint8_t op = fetch (z);
	int x = op >> 6, y = (op >> 3) & 7, zz = op & 7, p = y >> 1, q = y & 1;
	z->r = (z->r & 0x80) | ((z->r + 1) & 0x7f);
	if (x == 1) {
		switch (zz) {
			case 0: { uint8_t v = z->in (BC); if (y != 6) *r8p (z, y, 0) = v; z->f = (z->f & FC) | sz53p [v]; return 12; }
			case 1: z->out (BC, y == 6 ? 0 : *r8p (z, y, 0)); return 12;
			case 2: if (q) SETHL (adc16 (z, HL, getrp (z, p, 0))); else SETHL (sbc16 (z, HL, getrp (z, p, 0))); return 15;
			case 3: { uint16_t a = fetch16 (z); if (q) setrp (z, p, 0, rd16 (z, a)); else wr16 (z, a, getrp (z, p, 0)); return 20; }
			case 4: { uint8_t a = z->a; z->a = 0; sub8 (z, a, 0); return 8; }
			case 5: z->pc = pop (z); z->iff1 = z->iff2; return 14;
			case 6: z->im = (y & 3) == 0 || (y & 3) == 1 ? 0 : (y & 3) == 2 ? 1 : 2; return 8;
			case 7:
				switch (y) {
					case 0: z->i = z->a; return 9;
					case 1: z->r = z->a; return 9;
					case 2: z->a = z->i; z->f = (z->f & FC) | sz53 [z->a] | (z->iff2 ? FP : 0); return 9;
					case 3: z->a = z->r; z->f = (z->f & FC) | sz53 [z->a] | (z->iff2 ? FP : 0); return 9;
					case 4: { uint8_t m = RD (HL); WR (z, HL, (z->a << 4) | (m >> 4)); z->a = (z->a & 0xf0) | (m & 0x0f);
						z->f = (z->f & FC) | sz53p [z->a]; return 18; }
					case 5: { uint8_t m = RD (HL); WR (z, HL, (m << 4) | (z->a & 0x0f)); z->a = (z->a & 0xf0) | (m >> 4);
						z->f = (z->f & FC) | sz53p [z->a]; return 18; }
					default: return 8;
				}
		}
	} else if (x == 2 && y >= 4 && zz <= 3) {
		int inc = (y & 1) ? -1 : 1, rep = y >= 6;
		switch (zz) {
			case 0: { /* LDI/LDD */
				uint8_t v = RD (HL); WR (z, DE, v);
				SETHL (HL + inc); SETDE (DE + inc); SETBC (BC - 1);
				uint8_t n = v + z->a;
				z->f = (z->f & (FS | FZ | FC)) | (BC ? FP : 0) | (n & FX) | ((n << 4) & FY);
				if (rep && BC) { z->pc -= 2; return 21; }
				return 16; }
			case 1: { /* CPI/CPD */
				uint8_t v = RD (HL); uint8_t r = z->a - v;
				uint8_t hf = (z->a ^ v ^ r) & FH;
				SETHL (HL + inc); SETBC (BC - 1);
				uint8_t n = r - (hf ? 1 : 0);
				z->f = (z->f & FC) | FN | (r & FS) | (r ? 0 : FZ) | hf | (BC ? FP : 0) | (n & FX) | ((n << 4) & FY);
				if (rep && BC && r) { z->pc -= 2; return 21; }
				return 16; }
			case 2: { /* INI/IND */
				uint8_t v = z->in (BC); WR (z, HL, v); z->b --; SETHL (HL + inc);
				z->f = sz53 [z->b] | FN;
				if (rep && z->b) { z->pc -= 2; return 21; }
				return 16; }
			case 3: { /* OUTI/OUTD */
				uint8_t v = RD (HL); z->b --; z->out (BC, v); SETHL (HL + inc);
				z->f = sz53 [z->b] | FN;
				if (rep && z->b) { z->pc -= 2; return 21; }
				return 16; }
		}
	}
	return 8;	/* NOP-like */
}

int z80_step (Z80 *z) {
	int idx = 0, t = 0;
	uint8_t op;

	if (z->halted) { z->r = (z->r & 0x80) | ((z->r + 1) & 0x7f); return 4; }
	if (z->ei_delay) z->ei_delay --;

again:
	op = fetch (z);
	z->r = (z->r & 0x80) | ((z->r + 1) & 0x7f);
	if (op == 0xdd || op == 0xfd) { idx = op == 0xdd ? 1 : 2; t += 4; goto again; }
	if (op == 0xed) return t + exec_ed (z);
	if (op == 0xcb) {
		if (idx) {
			int8_t d = (int8_t) fetch (z);
			uint16_t a = getHL (z, idx) + d;
			return t + exec_cb (z, idx, a);
		}
		z->r = (z->r & 0x80) | ((z->r + 1) & 0x7f);
		return t + exec_cb (z, 0, 0);
	}

	int x = op >> 6, y = (op >> 3) & 7, zz = op & 7, p = y >> 1, q = y & 1;

	switch (x) {
	case 0:
		switch (zz) {
		case 0:
			switch (y) {
				case 0: return t + 4;
				case 1: { uint8_t a = z->a, f = z->f; z->a = z->a_; z->f = z->f_; z->a_ = a; z->f_ = f; return t + 4; }
				case 2: { int8_t d = fetch (z); z->b --; if (z->b) { z->pc += d; return t + 13; } return t + 8; }
				case 3: { int8_t d = fetch (z); z->pc += d; return t + 12; }
				default: { int8_t d = fetch (z); if (cond (z, y - 4)) { z->pc += d; return t + 12; } return t + 7; }
			}
		case 1:
			if (!q) { setrp (z, p, idx, fetch16 (z)); return t + 10; }
			setHL (z, idx, add16 (z, getHL (z, idx), getrp (z, p, idx))); return t + 11;
		case 2:
			switch (y) {
				case 0: WR (z, BC, z->a); return t + 7;
				case 1: z->a = RD (BC); return t + 7;
				case 2: WR (z, DE, z->a); return t + 7;
				case 3: z->a = RD (DE); return t + 7;
				case 4: wr16 (z, fetch16 (z), getHL (z, idx)); return t + 16;
				case 5: setHL (z, idx, rd16 (z, fetch16 (z))); return t + 16;
				case 6: WR (z, fetch16 (z), z->a); return t + 13;
				default: z->a = RD (fetch16 (z)); return t + 13;
			}
		case 3:
			if (!q) setrp (z, p, idx, getrp (z, p, idx) + 1); else setrp (z, p, idx, getrp (z, p, idx) - 1);
			return t + 6;
		case 4: case 5: case 6:
			if (y == 6) {
				uint16_t a = getHL (z, idx);
				if (idx) { a += (int8_t) fetch (z); t += 8; }
				if (zz == 4) { WR (z, a, inc8 (z, RD (a))); return t + 11; }
				if (zz == 5) { WR (z, a, dec8 (z, RD (a))); return t + 11; }
				{ uint8_t n = fetch (z); WR (z, a, n); return t + (idx ? 7 : 10); }
			} else {
				uint8_t *r = r8p (z, y, idx);
				if (zz == 4) { *r = inc8 (z, *r); return t + 4; }
				if (zz == 5) { *r = dec8 (z, *r); return t + 4; }
				*r = fetch (z); return t + 7;
			}
		case 7:
			switch (y) {
				case 0: { uint8_t c = z->a >> 7; z->a = (z->a << 1) | c; z->f = (z->f & (FS | FZ | FP)) | (z->a & (FX | FY)) | c; return t + 4; }
				case 1: { uint8_t c = z->a & 1; z->a = (z->a >> 1) | (c << 7); z->f = (z->f & (FS | FZ | FP)) | (z->a & (FX | FY)) | c; return t + 4; }
				case 2: { uint8_t c = z->a >> 7; z->a = (z->a << 1) | (z->f & FC); z->f = (z->f & (FS | FZ | FP)) | (z->a & (FX | FY)) | c; return t + 4; }
				case 3: { uint8_t c = z->a & 1; z->a = (z->a >> 1) | ((z->f & FC) << 7); z->f = (z->f & (FS | FZ | FP)) | (z->a & (FX | FY)) | c; return t + 4; }
				case 4: { /* DAA */
					uint8_t a = z->a, corr = 0, c = z->f & FC;
					if ((z->f & FH) || (a & 0x0f) > 9) corr |= 0x06;
					if (c || a > 0x99) { corr |= 0x60; c = FC; }
					if (z->f & FN) z->a = a - corr; else z->a = a + corr;
					z->f = (z->f & FN) | c | sz53p [z->a] | ((a ^ z->a) & FH);
					return t + 4; }
				case 5: z->a = ~z->a; z->f = (z->f & (FS | FZ | FP | FC)) | FH | FN | (z->a & (FX | FY)); return t + 4;
				case 6: z->f = (z->f & (FS | FZ | FP)) | FC | (z->a & (FX | FY)); return t + 4;
				default: z->f = (z->f & (FS | FZ | FP)) | ((z->f & FC) ? FH : FC) | (z->a & (FX | FY)); return t + 4;
			}
		}
		break;

	case 1:
		if (y == 6 && zz == 6) { z->halted = 1; return t + 4; }
		if (y == 6 || zz == 6) {
			uint16_t a = getHL (z, idx);
			if (idx) { a += (int8_t) fetch (z); t += 8; }
			if (y == 6) WR (z, a, *r8p (z, zz, 0)); else *r8p (z, y, 0) = RD (a);
			return t + 7;
		}
		*r8p (z, y, idx) = *r8p (z, zz, idx);
		return t + 4;

	case 2:
		if (zz == 6) {
			uint16_t a = getHL (z, idx);
			if (idx) { a += (int8_t) fetch (z); t += 8; }
			alu (z, y, RD (a)); return t + 7;
		}
		alu (z, y, *r8p (z, zz, idx)); return t + 4;

	case 3:
		switch (zz) {
		case 0: if (cond (z, y)) { z->pc = pop (z); return t + 11; } return t + 5;
		case 1:
			if (!q) {
				uint16_t v = pop (z);
				if (p == 3) { z->a = v >> 8; z->f = v; } else setrp (z, p, idx, v);
				return t + 10;
			}
			switch (p) {
				case 0: z->pc = pop (z); return t + 10;
				case 1: { uint8_t s;
					s = z->b; z->b = z->b_; z->b_ = s; s = z->c; z->c = z->c_; z->c_ = s;
					s = z->d; z->d = z->d_; z->d_ = s; s = z->e; z->e = z->e_; z->e_ = s;
					s = z->h; z->h = z->h_; z->h_ = s; s = z->l; z->l = z->l_; z->l_ = s;
					return t + 4; }
				case 2: z->pc = getHL (z, idx); return t + 4;
				default: z->sp = getHL (z, idx); return t + 6;
			}
		case 2: { uint16_t a = fetch16 (z); if (cond (z, y)) z->pc = a; return t + 10; }
		case 3:
			switch (y) {
				case 0: z->pc = fetch16 (z); return t + 10;
				case 2: z->out ((z->a << 8) | fetch (z), z->a); return t + 11;
				case 3: z->a = z->in ((z->a << 8) | fetch (z)); return t + 11;
				case 4: { uint16_t v = rd16 (z, z->sp); wr16 (z, z->sp, getHL (z, idx)); setHL (z, idx, v); return t + 19; }
				case 5: { uint16_t v = DE; SETDE (HL); SETHL (v); return t + 4; }
				case 6: z->iff1 = z->iff2 = 0; return t + 4;
				default: z->iff1 = z->iff2 = 1; z->ei_delay = 1; return t + 4;
			}
		case 4: { uint16_t a = fetch16 (z); if (cond (z, y)) { push (z, z->pc); z->pc = a; return t + 17; } return t + 10; }
		case 5:
			if (!q) { push (z, p == 3 ? AF : getrp (z, p, idx)); return t + 11; }
			if (p == 0) { uint16_t a = fetch16 (z); push (z, z->pc); z->pc = a; return t + 17; }
			return t + 4;	/* prefixes handled above */
		case 6: alu (z, y, fetch (z)); return t + 7;
		default: push (z, z->pc); z->pc = y * 8; return t + 11;
		}
	}
	return t + 4;
}

int z80_interrupt (Z80 *z, uint8_t bus) {
	if (!z->iff1 || z->ei_delay) return 0;
	if (z->halted) { z->halted = 0; }
	z->iff1 = z->iff2 = 0;
	z->r = (z->r & 0x80) | ((z->r + 1) & 0x7f);
	switch (z->im) {
		case 2: push (z, z->pc); z->pc = rd16 (z, (z->i << 8) | bus); return 19;
		default: push (z, z->pc); z->pc = 0x38; return 13;
	}
}
