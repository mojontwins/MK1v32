; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_pal_p0: planar mode 0 palette. HL = background inks, DE = sprite inks,
; both 16-entry hardware ink tables (mkts_om mode=pals): pen p = bg * 4 + s
; shows spr [s] if s, else bg [bg * 4] (so a pal_cnvtilesN.png gives the
; background inks and pal_cnvspr.png the sprite ones). The border gets
; bg [0].

	XLIB cd_pal_p0

.cd_pal_p0
	ld  (cd_pp0_bg), hl
	ld  (cd_pp0_spr), de
	ld  e, 0
.cd_pp0_loop
	ld  a, e
	and 3
	jr  z, cd_pp0_bgp
	ld  hl, (cd_pp0_spr)
	jr  cd_pp0_get
.cd_pp0_bgp
	ld  a, e
	ld  hl, (cd_pp0_bg)
.cd_pp0_get
	add a, l
	ld  l, a
	jr  nc, cd_pp0_nc
	inc h
.cd_pp0_nc
	ld  a, (hl)
	or  $40
	ld  bc, $7F00
	out (c), e
	out (c), a
	inc e
	ld  a, e
	cp  16
	jr  nz, cd_pp0_loop
	ld  hl, (cd_pp0_bg)
	ld  a, (hl)
	or  $40
	ld  c, 16
	out (c), c
	out (c), a
	ret
.cd_pp0_bg
	defw 0
.cd_pp0_spr
	defw 0
