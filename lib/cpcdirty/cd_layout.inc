; cpcdirty - dirty cell renderer for the Amstrad CPC (no superbuffer)
; Copyleft 2026 The Mojon Twins
;
; cd_layout.inc - fixed memory layout (the 12K at 9000-BFFF) and the
; constants every module takes from the game (see cpcdirty.h).
;
;   9000 CD_PAT      256 patterns x 16 bytes (2 bytes x 8 lines)
;   A000 CD_ROWBUF   30 cells, one every 32 bytes (the row being composed)
;   A400 CD_BUCKET   20 x 32: dirty columns of each row, $FF sentinel
;   A700 CD_ROWD     20 per-row dirty counts
;   A800 CD_ROWINT   20: interrupts after the 3rd one of a frame by which the
;                    beam has drawn row r
;   A900 CD_ROWSN    20: sprites crossing row r
;   AA00 CD_ROWSPR   20 x 8: low bytes of their CDK_SPRT records
;   AB00 CD_ROWTAB   20 x 4: TMAP + r*30 (word), screen address of row r (word)
;   AB80 ...         variables
;   AC00 CD_TMAP     30 x 20 pattern numbers
;   AE60 ...         more variables (AE58-AFFF is free otherwise)
;   then, from the game (they depend on the mode): CDK_CACHE, CDK_SPRT,
;   CDK_MLUT (masked modes), CDK_DIRTY, CDK_FREE.

	defc CD_PAT 		= $9000
	defc CD_ROWBUF 		= $A000
	defc CD_BUCKET 		= $A400
	defc CD_ROWD 		= $A700
	defc CD_ROWINT 		= $A800
	defc CD_ROWSN 		= $A900
	defc CD_ROWSPR 		= $AA00
	defc CD_ROWTAB 		= $AB00
	defc CD_CUR_R 		= $AB80
	defc CD_FL_BK 		= $AB82
	defc CD_ROW_D 		= $AB84
	defc CD_ROW_SCR 	= $AB86
	defc CD_SAVESP 		= $AB88
	defc CD_OR_K 		= $AB8A
	defc CD_OR_3COL 	= $AB8B
	defc CD_OR_ENTRY 	= $AB8C
	defc CD_MR_R 		= $AB8E
	defc CD_GEN 		= $AB8F
	defc CD_INTC0 		= $AB90
	defc CD_FULLREC 	= $AB98
	defc CD_CELLREC 	= $ABA4
	defc CD_OR_K2 		= $ABAC
	defc CD_OR_JP 		= $ABAD 	; jp <the mode's row composer>, set by cd_init_*
	defc CD_TMP 		= $ABB0 	; 8 bytes
	defc CD_TMAP 		= $AC00

	defc CD_KEY_SP 		= $AE60 	; frame of each cache (16 words)
	defc CD_KEY_PH 		= $AE80 	; phase of each cache (16 bytes)
	defc CD_SPR_EMPTY 	= $AE90 	; word: the frame that means "hidden"
	defc CD_BLANK 		= $AE92 	; word: what pattern 0 cells are filled with
	defc CD_GEOMP 		= $AE96 	; word: geometry table (planar mode 0)

	defc CD_RECSZ 		= 16 		; sprite record size (CDK_SPRT)

	; From the game (cpcdirty.h writes them):
	XREF CDK_NSPR 		; sprites (records in the sprite list), 1-16
	XREF CDK_SPR_LIST 	; sprite list: CDK_NSPR x 16 bytes, +0 frame (word),
						; +8 x, +9 y (pixels of the mode, screen relative)
	XREF CDK_VPX 		; viewport position, chars (2 bytes) from the left
	XREF CDK_VPY 		; viewport position, char rows from the top
	XREF CDK_SCRW 		; bytes per screen line: 64 (MK1, CRTC R1 = 32) or 80
	XREF CDK_SCRH 		; screen lines (cd_screen_copy): 192 or 200
	XREF CDK_CACHE 		; sprite caches
	XREF CDK_SPRT 		; sprite records, page aligned
	XREF CDK_DIRTY 		; 30 x 20 generation stamps
	XREF CDK_FREE 		; first free byte above
