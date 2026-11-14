; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_bc_n0: nibble mode 0 sprite cache. HL = frame (4 bytes x 16 lines, 16
; pens, CD_TPEN transparent), DE = cache, C = s (0/1: shifted one mode 0
; pixel right).
; The cache is in byte-column order, as in mode 1: column k (screen byte
; xb + k) of line y, (AND mask, data) at cache + k * 32 + y * 2; 4 columns
; (s = 0) or 5 (s = 1). out_k = frame byte k (s = 0) or
; ((prev & $55) << 1) | ((cur & $AA) >> 1) (s = 1: pixel 1 of byte k - 1,
; pixel 0 of byte k), the missing pixels in the transparent pen; mask =
; CD_MLUT [out], data = out & ~mask (so the transparent pen ORs nothing).
; Rebuilt only when the frame or s change.

	XLIB cd_bc_n0

	XREF CDK_MLUT
	XREF CDK_TPEN_B

	INCLUDE "cd_layout.inc"

.cd_bc_n0
	ld  a, c
	ld  (cd_bn_s), a
	ld  b, 16
.cd_bn_line
	push bc
	push de 					; cache line
	ld  de, CD_TMP
	ldi
	ldi
	ldi
	ldi 						; HL = next frame line
	pop de
	push de
	push hl
	ex  de, hl 					; HL = column 0 of this line
	ld  a, (cd_bn_s)
	or  a
	jr  nz, cd_bn_sh
	ld  a, (CD_TMP)
	call cd_bn_put
	ld  a, (CD_TMP + 1)
	call cd_bn_put
	ld  a, (CD_TMP + 2)
	call cd_bn_put
	ld  a, (CD_TMP + 3)
	call cd_bn_put
	jr  cd_bn_next
.cd_bn_sh
	ld  d, CDK_TPEN_B
	ld  a, (CD_TMP)
	ld  e, a
	call cd_bn_shp
	ld  a, (CD_TMP + 1)
	ld  e, a
	call cd_bn_shp
	ld  a, (CD_TMP + 2)
	ld  e, a
	call cd_bn_shp
	ld  a, (CD_TMP + 3)
	ld  e, a
	call cd_bn_shp
	ld  e, CDK_TPEN_B
	call cd_bn_shp
.cd_bn_next
	pop hl
	pop de
	inc de
	inc de
	pop bc
	djnz cd_bn_line
	ret

	; D = prev, E = cur: put ((D & $55) << 1) | ((E & $AA) >> 1); D = E
.cd_bn_shp
	ld  a, d
	and $55
	add a, a
	ld  d, a
	ld  a, e
	and $AA
	rrca
	or  d
	call cd_bn_put
	ld  d, e
	ret

	; A = out -> (HL) = mask, (HL + 1) = data; HL += 32. Keeps DE.
.cd_bn_put
	push de
	ld  e, a
	ld  d, CDK_MLUT / 256
	ld  a, (de)
	ld  (hl), a
	inc hl
	cpl
	and e
	ld  (hl), a
	ld  de, 31
	add hl, de
	pop de
	ret

.cd_bn_s
	defb 0
