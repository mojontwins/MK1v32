; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_intc: the beam clock. The game's ISR increments it once per interrupt
; (300 Hz): "ld hl, cd_intc / inc (hl)" (with LIB cd_intc). It lives here,
; not in the 12K at 9000-BFFF: screens are unpacked there while the ISR runs.

	XLIB cd_intc

.cd_intc
	defb 0
