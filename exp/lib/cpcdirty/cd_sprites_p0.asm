; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_sprites_p0: the per-frame sprite pass (cd_sprites.inc) for this mode.

	XLIB cd_sprites_p0

	LIB cd_bc_p0
	LIB cd_mark_sprites
	XREF cd_bc_h
	XREF cd_bc_w
	XREF cd_bc_st

	DEFINE CDP0

	INCLUDE "cd_layout.inc"

.cd_sprites_p0
	INCLUDE "cd_sprites.inc"
