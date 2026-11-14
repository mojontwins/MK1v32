; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_init_p0: planar mode 0 (split palette: pen = bg * 4 + spr; patterns use
; pen bits 2-3, byte mask $33; sprites pen bits 0-1, $CC, ORed unmasked).
; In: HL = the "hidden" sprite frame, DE = geometry table or 0. The patterns
; must be at CD_PAT.
;
; Without a table every slot is 16 lines x 4 bytes (16 game pixels) and its
; cache is 96 bytes (3 columns of 32, 32-aligned since CD_CACHE is), one
; after the other from CD_CACHE.
; With a table (h, w for every slot: h = 8 to 32 lines, w = 2 or 4 bytes),
; a cache is w / 2 + 1 columns of 2h bytes and never crosses a 256 byte page
; (the OR loop steps with inc l): a new page is started when one wouldn't
; fit. They must end by CD_SPRT; that is not checked.

	XLIB cd_init_p0

	LIB cd_init_common
	LIB cd_or_plain

	INCLUDE "cd_layout.inc"

.cd_init_p0
	push de
	push hl
	call cd_init_common
	pop hl
	ld  (CD_SPR_EMPTY), hl
	ld  hl, 0
	ld  (CD_BLANK), hl
	pop hl
	ld  (CD_GEOMP), hl

	ld  ix, CDK_SPRT
	ld  de, CDK_CACHE
	ld  b, CDK_NSPR
.cd_ip0_sprt
	push bc
	ld  a, h
	or  l
	jr  nz, cd_ip0_geom
	; default: 16 lines, 4 bytes, 96 bytes
	ld  c, 16
	ld  b, 4
	jr  cd_ip0_set
.cd_ip0_geom
	ld  c, (hl) 				; h
	inc hl
	ld  b, (hl) 				; w
	inc hl
.cd_ip0_set
	ld  (ix+10), c
	ld  (ix+11), c
	ld  (ix+12), b
	push hl
	; size = (w / 2 + 1) * 2h
	ld  a, b
	srl a
	inc a
	ld  b, a
	ld  hl, 0
	ld  a, c
	add a, a
.cd_ip0_mul
	add a, l
	ld  l, a
	adc a, h
	sub l
	ld  h, a
	ld  a, c
	add a, a
	djnz cd_ip0_mul
	; HL = size. With a table, start a new page if it doesn't fit in this one
	ld  a, (CD_GEOMP)
	ld  c, a
	ld  a, (CD_GEOMP + 1)
	or  c
	jr  z, cd_ip0_place
	ld  a, e
	add a, l
	ld  c, a
	ld  a, 0
	adc a, h 					; A:C = (DE & $FF) + size
	jr  z, cd_ip0_place 		; < $100: fits
	dec a
	or  c
	jr  z, cd_ip0_place 		; = $100: fits exactly
	ld  e, 0
	inc d
.cd_ip0_place
	ld  (ix+2), e
	ld  (ix+3), d
	ld  (ix+13), e
	ld  (ix+14), d
	ex  de, hl
	add hl, de
	ex  de, hl 					; DE = next cache
	push de
	ld  de, CD_RECSZ
	add ix, de
	pop de
	pop hl
	pop bc
	djnz cd_ip0_sprt

	; the flush composes rows with cd_or_plain
	ld  a, $C3
	ld  (CD_OR_JP), a
	ld  hl, cd_or_plain
	ld  (CD_OR_JP + 1), hl
	ret
