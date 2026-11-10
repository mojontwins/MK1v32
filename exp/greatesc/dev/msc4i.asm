	defc PLAYER_LIFE=99 ;; Find a way to solve this
; Imports
	XREF _flags
	XREF _n_pant
	XREF _on_pant
	XREF _gpx
	XREF _gpy
	XREF _tpx
	XREF _tpy
	XREF _tat
	XREF _tqt
	XREF _player
	XREF _attr_2
	XREF qtile_do
	XREF set_map_tile_do
	XREF _draw_coloured_tile
	XREF __x
	XREF __y
	XREF __t
	XREF __n
	XREF _comportamiento_tiles
	XREF _map_attr
	XREF _peta_el_beeper
	XREF _do_extern_action
	XREF draw_line_of_text
	XREF _hotspot_t
	XREF _scenery_info
	XREF __en_t
	XREF _enit
	XREF __en_x
	XREF __en_y
	XREF _script_param
; 5bit text decoder
	XREF _run_text_box
	XREF script_bytecode
; Target SPECCY
	LIB SPUpdateNow
; Exports
	XDEF _script_do
	XDEF _script_n
	XDEF _script_result
._script_n
	defw 0
._script_result
	defb 0
; Script pointer
.script
	defw 0
; Skip to next clausule
.skip
	defw 0
; Coordinate pair
.sc_x
	defb 0
.sc_y
	defb 0
; From the engine
._script_tx
	defb 0
._script_ty
	defb 0
._script_tn
	defb 0
; Control
.sc_terminado
	defb 0
._script_do
; Point to offset in script index
	ld  hl, (_script_n)
	add hl, hl
	ld  bc, script_bytecode
	add hl, bc
; Read offset
	ld  a, (hl)
	inc hl
	ld  h, (hl)
	ld  l, a
;  If zero do abort
	or  h
	ret z
; Make & store pointer
	add hl, bc
	ld  (script), hl
.script_loop
; Calculate address of next clausule
	ld  hl, (script)
	push hl
	call read_byte 		; A = clausule size
	ld  b, 0
	ld  c, a
	pop hl
	add hl, bc
	ld  (skip), hl
	cp  0xFF 			; End of section?
	ret z
; Process conditions
.script_clausule
	call read_byte 		;A = opcode
; If we get to 0xFF (THEN), jump to actions
	cp  0xFF
	jp  z, script_actions
;;; Decode OPCODE & jump to interpreter
;; OPCODE 0x01
;; IF A = B
	cp  0x01
	jr  nz, copcode_01_end
.copcode_01
	call read_vbyte
	ld  b, a
	call read_vbyte
	cp  b
	jp  nz, skip_clausule
	jp  script_clausule
.copcode_01_end
;; OPCODE 0x02
;; IF A < B
	cp  0x02
	jr  nz, copcode_02_end
.copcode_02
	call read_vbyte
	ld  c, a
	call read_vbyte
	ld  b, a
	ld  a, c
	cp  b
	jp  nc, skip_clausule
	jp  script_clausule
.copcode_02_end
;; OPCODE 0x03
;; IF A >= B
	cp  0x03
	jr  nz, copcode_03_end
.copcode_03
	call read_vbyte
	ld  c, a
	call read_vbyte
	ld  b, a
	ld  a, c
	cp  b
	jp  c, skip_clausule
	jp  script_clausule
.copcode_03_end
;; OPCODE 0x04
;; IF A <> B
	cp  0x04
	jr  nz, copcode_04_end
.copcode_04
	call read_vbyte
	ld  b, a
	call read_vbyte
	cp  b
	jp  z, skip_clausule
	jp  script_clausule
.copcode_04_end
;; OPCODE 0x23
;; IF PLAYER AT (X, Y)
	cp  0x23
	jr  nz, copcode_23_end
.opcode23
;; (gpx + 8) >> 4 != X -> exit
	ld  a, (_tpx)
	ld  c, a
	call read_vbyte
	cp  c
	jp  nz, skip_clausule
;; (gpy + 8) >> 4 != Y -> exit
	ld  a, (_tpy)
	ld  c, a
	call read_vbyte
	cp  c
	jp  nz, skip_clausule
	jp  script_clausule
.copcode_23_end
;; OPCODE 0xF0
;; TRUE
	cp  0xf0
	jr  nz, copcode_F0_end
.copcode_F0
	jp  script_clausule
.copcode_F0_end
;; UNKNOWN
	jp  script_clausule
.skip_clausule
	ld  hl, (skip)
	ld  (script), hl
	jp  script_loop
; Process actions
.script_actions
	call read_byte 		;A = opcode
; If we get to 0xFF (END), jump to next clausule
	cp  0xFF
	jp  z, script_loop
;;; Decode OPCODE & jump to interpreter
;; OPCODE 0x00
;; FLAGS[N] = V
	cp  0x00
	jr  nz, aopcode_00_end
.aopcode_00
	call read_i_v		; HL -> FLAGS[N], A -> V
	ld  (hl), a
	jp  script_actions
.aopcode_00_end
;; OPCODE 0x01
;; FLAGS[N] += V
	cp  0x01
	jr  nz, aopcode_01_end
.aopcode_01
	call read_i_v		; HL -> FLAGS[N], A -> V
	ld  b, (hl)
	add b
	ld  (hl), a
	jp  script_actions
.aopcode_01_end
;; OPCODE 0x02
;; FLAGS[N] -= V
	cp  0x02
	jr  nz, aopcode_02_end
.aopcode_02
	call read_i_v		; HL -> FLAGS[N], A -> V
	ld  b, a
	ld  a, (hl)
	sub b
	ld  (hl), a
	jp  script_actions
.aopcode_02_end
;; OPCODE 0x20
;; SET TILE (X, Y) = T
	cp  0x20
	jr  nz, aopcode_20_end
.aopcode_20
	call read_x_y
	call read_vbyte
	ld  (__t), a
	ld  b, 0
	ld  c, a
	ld  hl, _comportamiento_tiles
	add hl, bc
	ld  a, (hl)
	ld  (__n), a
	ld  a, (sc_x)
	ld  (__x), a
	ld  c, a
	ld  a, (sc_y)
	ld  (__y), a
	call set_map_tile_do
	jp  script_actions
.aopcode_20_end
;; OPCODE 0x22
;; DECOS XY T XY T ... 0xFF
	cp  0x22
	jr  nz, aopcode_22_end
.aopcode_22
	call read_byte
	cp  0xff
	jr  z, aopcode_22_end
	ld  (__t), a
	ld  b, 0
	ld  c, a
	ld  hl, _comportamiento_tiles
	add hl, bc
	ld  a, (hl)
	ld  (__n), a
	call read_byte
	ld  b, a
	and 0xf
	ld  c, a
	ld  (__x), a
	ld  a, b
	srl a
	srl a
	srl a
	srl a
	ld  (__y), a
	call set_map_tile_do
	jr  aopcode_22
.aopcode_22_end
;; OPCODE 0x50
;; PRINT TILE (X, Y) = N
	cp  0x50
	jr  nz, aopcode_50_end
.aopcode_50
	call read_vbyte
	ld  h, 0
	ld  l, a
	push hl
	call read_vbyte
	ld  h, 0
	ld  l, a
	push hl
	call read_vbyte
	ld  h, 0
	ld  l, a
	push hl
	call _draw_coloured_tile
	pop bc
	pop bc
	pop bc
	jp  script_actions
.aopcode_50_end
;; OPCODE 0x6D
;; WARP TO N, X, Y
	cp  0x6D
	jr  nz, aopcode_6D_end
.aopcode_6D
	call read_vbyte
	ld  (_n_pant), a
	call read_vbyte
	sla a
	sla a
	sla a
	sla a
	ld  (_gpx), a
	call read_vbyte
	sla a
	sla a
	sla a
	sla a
	ld  (_gpy), a
	jp  script_actions
.aopcode_6D_end
;; OPCODE 0xE0
;; SOUND N
	cp  0xE0
	jr  nz, aopcode_E0_end
.aopcode_E0
	call read_vbyte
	ld  h, 0
	ld  l, a
	call _peta_el_beeper
	jp  script_actions
.aopcode_E0_end
;; OPCODE 0xE3
;; TEXT L <CHARS> 0
	cp  0xE3
	jr  nz, aopcode_E3_end
.aopcode_E3
	call read_byte 			; String length
	ld  b, 0
	ld  c, a
	add hl, bc 				; Move after the string
	push hl
	ld  hl, (script)
	call draw_line_of_text
	pop hl
	ld  (script), hl 		; Get past the string
	jp script_actions
.aopcode_E3_end
;; OPCODE 0xE4
;; EXTERN N M
	cp  0xE4
	jr  nz, aopcode_E4_end
.aopcode_E4
	call read_x_y
	ld  a, (sc_x)
	ld  h, 0
	ld  l, a
	push hl
	ld  a, (sc_y)
	ld  h, 0
	ld  l, a
	push hl
	call _do_extern_action
	pop bc
	pop bc
	jp script_actions
.aopcode_E4_end
;; OPCODE 0xE6
;; TEXT BOX LSB MSB
	cp  0xE6
	jr  nz, aopcode_E6_end
.aopcode_E6
	call read_addr
	call _run_text_box
	jp script_actions
.aopcode_E6_end
;; OPCODE 0xF0
;; WIN GAME
	cp  0xf0
	jr  nz, aopcode_F0_end
.aopcode_F0
	ld  a, 1
	ld  (_script_result), a
	ret
.aopcode_F0_end
;; OPCODE 0xF1
;; GAME OVER
	cp  0xf1
	jr  nz, aopcode_F1_end
.aopcode_F1
	ld  a, 2
	ld  (_script_result), a
	ret
.aopcode_F1_end
;; OPCODE 0xF2
;; BREAK
	cp  0xf2
	jr  nz, aopcode_F2_end
.aopcode_F2
	ret
.aopcode_F2_end
;; OPCODE 0xF4
;; CALL lsb msb
	cp  0xf4
	jr  nz, aopcode_F4_end
.aopcode_F4
	call read_addr 	; Subroutine offset in HL
	ex  de, hl 		; Subroutine offset in DE
; Save script pointer
	ld  hl, (script)
	push hl
; Calculate new script ponter
	ld  hl, script_bytecode
	add hl, de
	ld  (script), hl
; Call this interpreter recursively!
	call script_loop
; Restore script pointer
	pop hl
	ld  (script), hl
	jp script_actions
.aopcode_F4_end
;; UNKNOWN
	jp script_actions
;; Reads a byte from pointer, inc pointer, return value in A
.read_byte
	ld  hl, (script)
	ld  a, (hl)
	inc hl
	ld  (script), hl
	ret
;; Reads a value (may be recursive flag), inc pointer, return value in A
;; New flags encoding is $FF means next value is flag (can be $FF, etc)
.read_vbyte
	call read_byte
	cp  0xff
	jr  z, read_vbyte_rec
	ret
.read_vbyte_rec
	call read_vbyte
; NPANT RVALUE
	cp  0xFE
	jr  nz, rvb_set_n_pant_done
	ld  a, (_n_pant)
	ret
.rvb_set_n_pant_done
; LIFE RVALUE
	cp  0xF9
	jr  nz, rvb_set_player_life_done
	ld  a, (_player + 29) 	; player.life MSB
	ret
.rvb_set_player_life_done
; TX RVALUE
	cp  0xF8
	jr  nz, rvb_set_tx_done
	ld  a, (_tpx)
	ret
.rvb_set_tx_done
; TY RVALUE
	cp  0xF7
	jr  nz, rvb_set_ty_done
	ld  a, (_tpy)
	ret
.rvb_set_ty_done
; TN RVALUE
	cp  0xF6
	jr  nz, rvb_set_tile_done
	ld  a, (_tqt)
	ret
.rvb_set_tile_done
; EN_T RVALUE
	cp  0xEE
	jr  nz, rvb_set_en_t_done
	ld  a, (__en_t)
	ret
.rvb_set_en_t_done
; TILE AT EXPRESION RVALUE
	cp 0xE8
	jr  nz, rvb_set_script_tile_at_done
	push bc
; Read two rvalues
	call read_x_y
	ld  a, (sc_x)
	ld  c, a
	ld  a, (sc_y)
	call qtile_do
	ld  a, l
	pop bc
	ret
.rvb_set_script_tile_at_done
	ld  d, 0
	ld  e, a
	ld  hl, _flags
	add hl, de
	ld  a, (hl)
	ret
.read_x_y
	call read_vbyte
	ld  (sc_x), a
	call read_vbyte
	ld  (sc_y), a
	ret
.read_addr
	call read_byte
	ld  c, a
	call read_byte
	ld  h, a
	ld  l, c
	ret
;; Read flag index and value, returns pointer in HL and value in A.
.read_i_v
	call read_vbyte  		; Read flag index
	ld  c, a
	call read_vbyte 		; Read value
	ld  (sc_y), a
	ld  a, c  				; C = flag index
; PY LVALUE
	cp  0xFC
	jr  nz, riv_set_gpy_done
	ld  hl, _gpy
	jr  read_i_v_cont
.riv_set_gpy_done
; LIFE LVALUE
	cp  0xF9
	jr  nz, riv_set_player_life_done
	ld  hl, _player + 29	; player.life LSB
	jr  read_i_v_cont
.riv_set_player_life_done
	ld  b, 0 				; BC = flag index
	ld  hl, _flags
	add hl, bc 				; HL -> FLAGS [X]
.read_i_v_cont
	ld  a, (sc_y) 			; A = value
	ret
