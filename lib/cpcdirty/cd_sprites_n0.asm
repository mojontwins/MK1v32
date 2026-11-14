; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_sprites_n0: the per-frame sprite pass (cd_sprites.inc) for this mode.

	XLIB cd_sprites_n0

	LIB cd_bc_n0
	LIB cd_mark_sprites

	DEFINE CDN0

	INCLUDE "cd_layout.inc"

.cd_sprites_n0
	INCLUDE "cd_sprites.inc"
