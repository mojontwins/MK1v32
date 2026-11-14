; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_sprites_m1: the per-frame sprite pass (cd_sprites.inc) for this mode.

	XLIB cd_sprites_m1

	LIB cd_bc_m1
	LIB cd_mark_sprites

	DEFINE CDM1

	INCLUDE "cd_layout.inc"

.cd_sprites_m1
	INCLUDE "cd_sprites.inc"
