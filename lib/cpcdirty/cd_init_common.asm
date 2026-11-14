; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_init_common: the part of cd_init_* shared by every mode. The patterns
; must be at CD_PAT already. Clears A000 - CD_FREE (buffers, counts, map,
; stamps; what is above, the game's, is kept), builds the row tables, the
; full viewport rect and the per-row beam guard, empties the cache keys and
; starts the generation at 1. Sprite records are left to the mode.

	XLIB cd_init_common

	INCLUDE "cd_layout.inc"

.cd_init_common
	ld  hl, $A000
	ld  de, $A001
	ld  bc, CDK_FREE - $A001
	xor a
	ld  (hl), a
	ldir

	; ROWTAB: TMAP + r*30, $C000 + (CD_VPY + r) * CD_SCRW + CD_VPX * 2
	ld  ix, CD_ROWTAB
	ld  hl, CD_TMAP
	ld  de, $C000 + CDK_VPY * CDK_SCRW + CDK_VPX * 2
	ld  b, 20
.cd_ic_rowtab
	ld  (ix+0), l
	ld  (ix+1), h
	ld  (ix+2), e
	ld  (ix+3), d
	push bc
	ld  bc, 30
	add hl, bc
	ex  de, hl
	ld  bc, CDK_SCRW
	add hl, bc
	ex  de, hl
	pop bc
	inc ix
	inc ix
	inc ix
	inc ix
	djnz cd_ic_rowtab

	; ROWINT [r]: screen row CD_VPY + r has been drawn at VSYNC + 72 +
	; 8 * (row + 1) lines; interrupt n comes at VSYNC + 2 + 52 * (n - 1).
	; s = the first n whose interrupt is not before that; count from the
	; 3rd one: ROWINT [r] = s > 3 ? s - 3 : 0.
	ld  hl, CD_ROWINT
	ld  de, 72 + 8 * (CDK_VPY + 1) 	; lines for row 0
	ld  b, 20
.cd_ic_rowint
	push hl
	ld  c, 1 						; s
	ld  hl, 2 						; line of interrupt s
.cd_ic_ri_loop
	or  a
	sbc hl, de
	add hl, de
	jr  nc, cd_ic_ri_done 			; hl >= de
	push de
	ld  de, 52
	add hl, de
	pop de
	inc c
	jr  cd_ic_ri_loop
.cd_ic_ri_done
	pop hl
	ld  a, c
	sub 3
	jr  nc, cd_ic_ri_st
	xor a
.cd_ic_ri_st
	ld  (hl), a
	inc l
	ex  de, hl
	ld  de, 8
	add hl, de
	ex  de, hl
	djnz cd_ic_rowint

	; full viewport rect: c0 c1 r0 r1 at +4
	ld  hl, CD_FULLREC + 4
	ld  (hl), 0
	inc hl
	ld  (hl), 29
	inc hl
	ld  (hl), 0
	inc hl
	ld  (hl), 19

	; no cache is valid
	ld  hl, CD_KEY_SP
	ld  de, CD_KEY_SP + 1
	ld  bc, 32 + 16 - 1
	ld  (hl), 0
	ldir

	ld  a, 1
	ld  (CD_GEN), a
	ret
