; cpct_miniload (CPCtelera, CNGSoft & ronaldo, LGPL; adapted by na_th_an for
; MK1's loaders, here in z80asm syntax). IX = where to, DE = size. Reads a raw
; block (cpctape mode=raw): pilot, sync, 2 pulses a bit, a check byte. The
; border flashes between the two colours in COLORES_CARGA. Carry: loaded OK.

.cpct_miniload_asm
	ld   bc, $F610 			; cassette motor on
	out  (c), c
	exx
	push bc
	push de
	ld   de, COLORES_CARGA
	ld   bc, $7F10 			; gate array, border
	out  (c), c
	exx
.ml_init
	ld   h, 0
.ml_tone
	call ml_full
	jr   nc, ml_init
	sub  b
	jr   nc, ml_init
	inc  h
	jr   nz, ml_tone
.ml_wait
	ld   h, -2
.ml_sync
	call ml_full
	jr   nc, ml_init
	sub  b
	jr   c, ml_wait
	inc  h
	jr   nz, ml_sync
	jr   ml_byte
.ml_full
	ld   b, 2-64
.ml_half
	ld   a, 16-2
.ml_delay
	dec  a
	jr   nz, ml_delay
.ml_edge
	inc  b
	ret  z
	ld   a, $F5
	in   a, ($00)
	xor  c
	and  $80
	jr   z, ml_edge
	xor  c
	ld   c, a
	exx
	ld   a, e
	ld   e, d
	ld   d, a
	nop
	out  (c), a
	exx
	ld   a, 2-48
	scf
	ret
.ml_next
	ld   (ix+0), l
	inc  ix
	dec  de
.ml_byte
	ld   l, 1
.ml_bits
	ld   b, 1-80
	call ml_half
	call c, ml_half
	jr   nc, ml_exit
	sub  b
	rl   l
	jr   nc, ml_bits
	ld   a, h
	xor  l
	ld   h, a
	ld   a, d
	or   e
	jr   nz, ml_next
	inc  a
	add  a, h
	ld   l, a
.ml_exit
	exx
	ld   bc, $F600 			; motor off
	out  (c), c
	pop  de
	pop  bc
	exx
	ret
