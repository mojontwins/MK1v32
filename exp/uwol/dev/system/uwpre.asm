; Uwol (CPC) - tape: the first file (firmware, RUN""), at 0400: loads the
; loader (raw) at BA00 and runs it. sizes.inc: LOADER_SIZE.

	org $0400

	defc COLORES_CARGA = $445F

	INCLUDE "sizes.inc"

	di
	ld   ix, $BA00
	ld   de, LOADER_SIZE
	call cpct_miniload_asm
	jp   $BA00

	INCLUDE "miniload.inc"
