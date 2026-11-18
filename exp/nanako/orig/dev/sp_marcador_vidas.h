extern unsigned char sp_marcador_vidas_0 [];
extern unsigned char sp_marcador_vidas_1 [];

unsigned char *sp_marcador_vidas [] = {
	sp_marcador_vidas_0,
	sp_marcador_vidas_1
};

#asm

._sp_marcador_vidas_0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0

._sp_marcador_vidas_1
	defb $4A, $A
	defb $87, $82
	defb $C1, $80
	defb $F, $A
	defb $C1, $80
	defb $87, $82
	defb $4A, $A
	defb $0, $0

#endasm

