; MK1 (ZX) - tape loader, at 24000 (BASIC: CLEAR 23999, LOAD ""CODE, USR 24000)
;
; Loads (headerless) the screen and the game at 24200. Then, if it's a 128K,
; the music & effects (RAM1.bin, the WYZ player) into RAM1 at C000. On a 48K
; the last block is not loaded. As MK1v4's loader_128 (ramiro4).
; zxsizes.inc: MAIN_SIZE, RAM1_SIZE.

	org 24000

	INCLUDE "zxsizes.inc"

	di
	ld  sp, 23999

	; Black attributes
	ld  hl, $5800
	ld  de, $5801
	ld  bc, 767
	ld  (hl), l
	ldir

	; Screen
	ld  ix, $4000
	ld  de, 6912
	call load

	; Game
	ld  ix, 24200
	ld  de, MAIN_SIZE
	call load

	; 128K? Paging the ROM changes what's at 0001.
	ld  bc, $7ffd
	xor a
	out (c), a
	ld  a, ($0001)
	ld  h, a
	ld  a, $10
	out (c), a
	ld  a, ($0001)
	cp  h
	jr  z, run

	; RAM1 at C000 (ROM 1, RAM 1), then back to RAM 0
	ld  a, $11
	out (c), a
	ld  ix, $c000
	ld  de, RAM1_SIZE
	call load
	ld  a, $10
	ld  bc, $7ffd
	out (c), a

.run
	jp  24200

.load
	scf
	ld  a, $ff
	call $0556
	di
	ret
