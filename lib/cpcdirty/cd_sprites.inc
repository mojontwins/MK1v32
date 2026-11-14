; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_sprites.inc: the per-frame sprite pass, assembled once per mode by
; cd_sprites_m1.asm, cd_sprites_p0.asm and cd_sprites_n0.asm (CDM1 / CDP0 /
; CDN0).
; For every sprite of the list: visible? (frame not CD_SPR_EMPTY, inside
; the viewport) -> its record (x byte, y, shift flag), and its cache rebuilt
; if the frame or the phase changed (mode 1 and nibble mode 0: the shift;
; planar mode 0: byte parity + shift). Then cd_mark_sprites.
; Planar mode 0 sprites can go up out of the viewport (they are cut at its
; top edge), not down out of it (hidden).
	push iy
	ld  ix, CDK_SPR_LIST
	ld  iy, CDK_SPRT
	ld  hl, CD_KEY_SP
	ld  (cd_cs_ksp), hl
	ld  hl, CD_KEY_PH
	ld  (cd_cs_kph), hl
	ld  b, CDK_NSPR
.cd_cs_loop
	push bc
	; frame
	ld  l, (ix+0)
	ld  h, (ix+1)
	ld  de, (CD_SPR_EMPTY)
	or  a
	sbc hl, de
	jp  z, cd_cs_hide
	add hl, de
	ld  (cd_cs_sp0), hl
	IF CDM1
	ld  a, (ix+8)
	sub CDK_VPX * 8
	jp  c, cd_cs_hide
	cp  225
	jp  nc, cd_cs_hide
	ld  e, a 					; px
	ld  a, (ix+9)
	sub CDK_VPY * 8
	jp  c, cd_cs_hide
	cp  145
	jp  nc, cd_cs_hide
	ld  (iy+1), a 				; y
	ld  a, e
	and 3
	ld  c, a 					; s
	ld  a, e
	srl a
	srl a
	ld  (iy+0), a 				; xb
	ld  b, 0 					; (parity: not needed)
	ld  a, c 					; phase = s
	ENDIF
	IF CDN0
	ld  a, (ix+8)
	sub CDK_VPX * 4
	jp  c, cd_cs_hide
	cp  113
	jp  nc, cd_cs_hide
	ld  e, a 					; px
	ld  a, (ix+9)
	sub CDK_VPY * 8
	jp  c, cd_cs_hide
	cp  145
	jp  nc, cd_cs_hide
	ld  (iy+1), a 				; y
	ld  a, e
	and 1
	ld  c, a 					; s
	ld  a, e
	srl a
	ld  (iy+0), a 				; xb
	ld  b, 0 					; (parity: not needed)
	ld  a, c 					; phase = s
	ENDIF
	IF CDP0
	ld  a, (ix+8)
	sub CDK_VPX * 4
	jp  c, cd_cs_hide
	cp  113
	jp  nc, cd_cs_hide
	ld  e, a 					; px
	ld  a, (ix+9)
	sub CDK_VPY * 8
	jr  nc, cd_cs_ypos
	; above the viewport: skip the lines out of it
	neg
	cp  (iy+11)
	jp  nc, cd_cs_hide
	ld  c, a
	ld  a, (iy+11)
	sub c
	ld  (iy+10), a 				; lines drawn
	xor a
	ld  (iy+1), a 				; y
	ld  a, c
	add a, a
	add a, (iy+13)
	ld  (iy+2), a
	ld  a, (iy+14)
	adc a, 0
	ld  (iy+3), a 				; cache + skipped lines
	jr  cd_cs_xdo
.cd_cs_ypos
	ld  (iy+1), a 				; y
	ld  c, a
	ld  a, (iy+11)
	ld  (iy+10), a 				; lines drawn = h
	add a, c
	jp  c, cd_cs_hide
	cp  161
	jp  nc, cd_cs_hide
	ld  a, (iy+13)
	ld  (iy+2), a
	ld  a, (iy+14)
	ld  (iy+3), a
.cd_cs_xdo
	ld  a, e
	and 1
	ld  c, a 					; s
	ld  a, e
	srl a
	ld  (iy+0), a 				; xb
	and 1
	ld  b, a 					; b0
	add a, a
	or  c 						; phase = b0 * 2 + s
	ENDIF
	; rec [8]: 1 if the sprite takes one more byte
	ld  e, a
	ld  a, c
	or  a
	jr  z, cd_cs_s0
	ld  a, 1
.cd_cs_s0
	ld  (iy+8), a
	set 0, (iy+9)
	; same frame and phase as the cache? (key_ph [i], key_sp [i])
	ld  hl, (cd_cs_kph)
	ld  a, (hl)
	cp  e
	jr  nz, cd_cs_build
	ld  hl, (cd_cs_ksp)
	ld  a, (hl)
	inc hl
	ld  h, (hl)
	ld  l, a
	ld  de, (cd_cs_sp0)
	or  a
	sbc hl, de
	jr  z, cd_cs_next
	ld  hl, (cd_cs_kph)
.cd_cs_build
	ld  (hl), e
	ld  hl, (cd_cs_ksp)
	ld  de, (cd_cs_sp0)
	ld  (hl), e
	inc hl
	ld  (hl), d
	IF CDP0
	ld  a, (iy+11)
	ld  (cd_bc_h), a
	add a, a
	dec a
	ld  (cd_bc_st), a
	ld  a, (iy+12)
	ld  (cd_bc_w), a
	ENDIF
	ld  hl, (cd_cs_sp0)
	ld  e, (iy+13)
	ld  d, (iy+14)
	push ix
	push iy
	IF CDM1
	call cd_bc_m1 				; HL frame, DE cache, C s
	ENDIF
	IF CDN0
	call cd_bc_n0 				; HL frame, DE cache, C s
	ENDIF
	IF CDP0
	call cd_bc_p0 				; HL frame, DE cache, B b0, C s
	ENDIF
	pop iy
	pop ix
	jr  cd_cs_next
.cd_cs_hide
	res 0, (iy+9)
.cd_cs_next
	ld  de, 16 				; sprite list stride
	add ix, de
	ld  de, CD_RECSZ
	add iy, de
	ld  hl, (cd_cs_ksp)
	inc hl
	inc hl
	ld  (cd_cs_ksp), hl
	ld  hl, (cd_cs_kph)
	inc hl
	ld  (cd_cs_kph), hl
	pop bc
	dec b
	jp  nz, cd_cs_loop
	pop iy
	jp  cd_mark_sprites

.cd_cs_sp0
	defw 0

.cd_cs_ksp
	defw 0
.cd_cs_kph
	defw 0
	
