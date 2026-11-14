; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_init_m1: mode 1 (4 pens shared by tiles and sprites, pen 0 of the
; sprites is transparent: masked through CD_MLUT).
; In: HL = the "hidden" sprite frame (__FASTCALL__). The patterns must be at CD_PAT.
; Caches: CD_NSPR x 160 bytes from CD_CACHE (5 byte columns x 16 lines x
; (mask, data), columns 32-aligned).

	XLIB cd_init_m1

	LIB cd_init_common
	LIB cd_or_mask
	LIB cd_bc_m1
	XREF cd_bc_curs
	XREF CDK_MLUT
	XREF CDK_BLANK_W

	INCLUDE "cd_layout.inc"

.cd_init_m1
	push hl
	call cd_init_common
	pop hl
	ld  (CD_SPR_EMPTY), hl
	ld  hl, CDK_BLANK_W
	ld  (CD_BLANK), hl

	; sprite records: 16 x 16 (4 bytes), 160 byte caches
	ld  ix, CDK_SPRT
	ld  hl, CDK_CACHE
	ld  de, 160
	ld  b, CDK_NSPR
.cd_im1_sprt
	ld  (ix+2), l
	ld  (ix+3), h
	ld  (ix+13), l
	ld  (ix+14), h
	ld  (ix+10), 16
	ld  (ix+11), 16
	ld  (ix+12), 4
	add hl, de
	push de
	ld  de, CD_RECSZ
	add ix, de
	pop de
	djnz cd_im1_sprt

	; CD_MLUT [b] = AND mask of byte b: 0 bits for pixels not pen 0
	ld  hl, CDK_MLUT
.cd_im1_mlut
	ld  a, l
	ld  c, a
	rrca
	rrca
	rrca
	rrca
	or  c
	and $0F
	ld  b, a
	rlca
	rlca
	rlca
	rlca
	or  b
	cpl
	ld  (hl), a
	inc l
	jr  nz, cd_im1_mlut

	ld  a, $FF 					; no shifts patched yet
	ld  (cd_bc_curs), a

	; the flush composes rows with cd_or_mask
	ld  a, $C3
	ld  (CD_OR_JP), a
	ld  hl, cd_or_mask
	ld  (CD_OR_JP + 1), hl
	ret
