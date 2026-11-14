; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_bc_m1: mode 1 sprite cache. HL = frame (4 bytes x 16 lines), DE = cache
; (5 x 32 bytes), C = s (0-3).

	XLIB cd_bc_m1

	XDEF cd_bc_curs
	XREF CDK_MLUT

	INCLUDE "cd_layout.inc"

; MODE_1. HL = frame (4 bytes x 16 lines), DE = cache (5 x 32 bytes), C = s
; (0-3). Byte column k (screen byte xb + k) of line y, (AND mask, data) at
; cache + k * 32 + y * 2: out = ((cur >> s) & m1) | ((prev << (4 - s)) & m2)
; per nibble (cur = frame byte k, prev = k - 1), m1 = (0x0F >> s) * 0x11,
; m2 = ~m1; mask = CD_MLUT [out]. The shifts are rrca / rlca patched into
; the code below for this s (only when s changes). Interrupts are off
; while SP reads the frame (about 30 raster lines; the Gate Array keeps
; the interrupt pending).
.cd_bc_m1
	ld  a, (cd_bc_curs)
	cp  c
	jr  z, cd_bc_patched
	push hl
	push de
	call cd_bc_patch
	pop de
	pop hl
.cd_bc_patched
	push ix
	push iy
	ld  ix, 64
	add ix, de 				; IX = column 2: columns at -64 .. +64
	ld  (cd_bc_frame), hl
	di
	ld  (cd_bc_sp), sp
	ld  sp, (cd_bc_frame)
	ld  h, CDK_MLUT / 256
	ld  iyl, 16
.cd_bc_line
	pop bc 					; c = byte 0, b = byte 1
	pop de 					; e = byte 2, d = byte 3
	; column 0: R (b0)
	ld  a, c
.cd_r0
	nop
	nop
	nop
.cd_r0m
	and $FF
	ld  l, a
	ld  a, (hl)
	ld  (ix-64), a
	ld  (ix-63), l
	; column 1: C (b0) | R (b1)
	ld  a, c
.cd_c0
	nop
	nop
	nop
.cd_c0m
	and $00
	ld  l, a
	ld  a, b
.cd_r1
	nop
	nop
	nop
.cd_r1m
	and $FF
	or  l
	ld  l, a
	ld  a, (hl)
	ld  (ix-32), a
	ld  (ix-31), l
	; column 2: C (b1) | R (b2)
	ld  a, b
.cd_c1
	nop
	nop
	nop
.cd_c1m
	and $00
	ld  l, a
	ld  a, e
.cd_r2
	nop
	nop
	nop
.cd_r2m
	and $FF
	or  l
	ld  l, a
	ld  a, (hl)
	ld  (ix+0), a
	ld  (ix+1), l
	; column 3: C (b2) | R (b3)
	ld  a, e
.cd_c2
	nop
	nop
	nop
.cd_c2m
	and $00
	ld  l, a
	ld  a, d
.cd_r3
	nop
	nop
	nop
.cd_r3m
	and $FF
	or  l
	ld  l, a
	ld  a, (hl)
	ld  (ix+32), a
	ld  (ix+33), l
	; column 4: C (b3)
	ld  a, d
.cd_c3
	nop
	nop
	nop
.cd_c3m
	and $00
	ld  l, a
	ld  a, (hl)
	ld  (ix+64), a
	ld  (ix+65), l
	inc ix
	inc ix
	dec iyl
	jp  nz, cd_bc_line
	ld  sp, (cd_bc_sp)
	ei
	pop iy
	pop ix
	ret

	; Patch the shifts for s = C
.cd_bc_patch
	ld  a, c
	ld  (cd_bc_curs), a
	; m1, m2
	ld  hl, cd_bc_mk
	ld  e, a
	ld  d, 0
	add hl, de
	ld  a, (hl)
	ld  (cd_r0m + 1), a
	ld  (cd_r1m + 1), a
	ld  (cd_r2m + 1), a
	ld  (cd_r3m + 1), a
	cpl
	ld  (cd_c0m + 1), a
	ld  (cd_c1m + 1), a
	ld  (cd_c2m + 1), a
	ld  (cd_c3m + 1), a
	; rrca slots: s of them; rlca slots: 4 - s (s > 0)
	ld  hl, cd_bc_rot
	ld  a, c
	add a, a
	add a, a
	add a, a
	ld  e, a
	add hl, de 				; HL = 6 opcodes: 3 for R, 3 for C
	ld  de, cd_r0
	call cd_bc_p3
	ld  de, cd_r1
	call cd_bc_p3
	ld  de, cd_r2
	call cd_bc_p3
	ld  de, cd_r3
	call cd_bc_p3
	inc hl
	inc hl
	inc hl
	ld  de, cd_c0
	call cd_bc_p3
	ld  de, cd_c1
	call cd_bc_p3
	ld  de, cd_c2
	call cd_bc_p3
	ld  de, cd_c3
.cd_bc_p3
	push hl
	ldi
	ldi
	ldi
	pop hl
	ret

.cd_bc_mk
	defb $FF, $77, $33, $11
.cd_bc_rot
	; s = 0..3: R (3), C (3), 2 spare. rrca $0F, rlca $07, nop $00
	defb $00, $00, $00, $00, $00, $00, 0, 0
	defb $0F, $00, $00, $07, $07, $07, 0, 0
	defb $0F, $0F, $00, $07, $07, $00, 0, 0
	defb $0F, $0F, $0F, $07, $00, $00, 0, 0
.cd_bc_curs
	defb 0
.cd_bc_frame
	defw 0
.cd_bc_sp
	defw 0
