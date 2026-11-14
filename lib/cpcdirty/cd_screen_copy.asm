; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_screen_copy: a linear CD_SCRW x CD_SCRH screen image (e.g. a 64 x 192
; mkts_om superbuffer unpacked to CD_PAT) -> the screen ($C000, CD_SCRW
; bytes a line). HL = source. The 12K at 9000-BFFF is not touched; if the
; image was unpacked there, call cd_init_* again afterwards.
; (A standard 80 x 200 screen in .scr layout is better unpacked straight to
; $C000.)

	XLIB cd_screen_copy

	INCLUDE "cd_layout.inc"

.cd_screen_copy
	ld  de, $C000
	ld  a, CDK_SCRH
.cd_scp_loop
	ld  bc, CDK_SCRW
	push de
	ldir
	pop de
	ex  de, hl
	ld  bc, $0800 				; next line
	add hl, bc
	jr  nc, cd_scp_nc
	ld  bc, $C000 + CDK_SCRW 	; past the 8th line: next row (wraps)
	add hl, bc
.cd_scp_nc
	ex  de, hl
	dec a
	jr  nz, cd_scp_loop
	ret
