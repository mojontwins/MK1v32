/* z80.h - compact Z80 core for zxheadless (documented + common undocumented ops) */
#ifndef Z80_H
#define Z80_H
#include <stdint.h>

typedef struct {
	uint8_t a, f, b, c, d, e, h, l;
	uint8_t a_, f_, b_, c_, d_, e_, h_, l_;
	uint16_t ix, iy, sp, pc;
	uint8_t i, r, iff1, iff2, im, halted;
	int ei_delay;
	uint64_t cycles;
	uint8_t *mem;                               /* 64K */
	uint8_t (*in) (uint16_t port);
	void (*out) (uint16_t port, uint8_t v);
	int rom_protect;                            /* writes below 16384 ignored */
	void (*wr_hook) (uint16_t addr, uint8_t v); /* optional, called on every memory write */
} Z80;

void z80_reset (Z80 *z);
int  z80_step (Z80 *z);                        /* returns T-states */
int  z80_interrupt (Z80 *z, uint8_t bus);      /* returns T-states (0 if not taken) */

#endif
