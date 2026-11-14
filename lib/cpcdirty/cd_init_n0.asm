; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_init_n0: nibble mode 0 (16 pens shared by tiles and sprites; the
; sprites' transparent pen, CD_TPEN, is masked through CD_MLUT, so tiles can
; still use that pen).
; In: HL = the "hidden" sprite frame (__FASTCALL__). The patterns must be at CD_PAT.
; Caches: CD_NSPR x 160 bytes from CD_CACHE (5 byte columns x 16 lines x
; (mask, data), columns 32-aligned), as in mode 1.

	XLIB cd_init_n0

	LIB cd_init_common
	LIB cd_or_mask
	XREF CDK_MLUT
	XREF CDK_BLANK_W
	XREF CDK_TPEN_B 		; a byte with both pixels in the transparent pen

	INCLUDE "cd_layout.inc"

.cd_init_n0
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
.cd_in0_sprt
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
	djnz cd_in0_sprt

	; CD_MLUT [b] = AND mask of byte b: pixel 0 (bits $AA) and pixel 1
	; (bits $55) kept (1s) where they are in the transparent pen
	ld  hl, CDK_MLUT
.cd_in0_mlut
	ld  a, l
	xor CDK_TPEN_B
	ld  c, a
	ld  b, 0
	and $AA
	jr  nz, cd_in0_p1
	ld  b, $AA
.cd_in0_p1
	ld  a, c
	and $55
	ld  a, b
	jr  nz, cd_in0_st
	or  $55
.cd_in0_st
	ld  (hl), a
	inc l
	jr  nz, cd_in0_mlut

	; the flush composes rows with cd_or_mask
	ld  a, $C3
	ld  (CD_OR_JP), a
	ld  hl, cd_or_mask
	ld  (CD_OR_JP + 1), hl
	ret
