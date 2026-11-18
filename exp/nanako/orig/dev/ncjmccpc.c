// ncjmccpc.c

// Nanako in Classic Japanese Monster Castle 
// =========================================

// Versión para Amstrad CPC en C. ¿Me animaré a reportarlo a ZX Spectrum desde
// esta versión en C? Depende de como quede. Si al final queda demasiado igual, nol.

// Empezamos.

// Mi tercer juego para CPC.

#include "cpcrslib.h"				// Biblioteca de juego por Artaburu.
#include "cpcwyzlib.h"				// Replayer AY por WYZ, adaptado por Artaburu.
#include "sp_karakasa.h"			// Sprite de Karakasa.
#include "sp_nanako.h"				// Sprite de Nanako.
#include "sp_puf.h"					// Sprite del puf.
#include "sp_marcador_vidas.h"		// Gráficos para el marcador de vidas.
#include "fases.h"					// Mapeado del juego
#include "sound.h"					// Configuración del replayer AY.
	
/*******
 Defines
 *******/

// Teclas:

#define KEY_IZQ 		0
#define KEY_DER 		1
#define KEY_ARR 		2
#define KEY_ABA 		3
#define KEY_FIR 		4
#define KEY_RET			5
#define KEY_ESC 		6
#define KEY_MENU_S		7
#define KEY_MENU_P		8
#define KEY_MENU_R		9


// Límites de tablas, etc.

#define NUM_PERSISTENCIAS	15
#define WYZ_CANAL_EFECTOS 	2

// Durezas:

#define VACIO			0
#define OBJETO_NO_MOVIL	1
#define OBJETO_MOVIL	2

// Estados del Karakasa

#define STATE_IDLE		0
#define	STATE_WLEFT		1
#define STATE_WRIGHT	2

/****************************************************
 Estructuras, globales, externas y demás definiciones
 ****************************************************/

// Structs

typedef struct {
	unsigned char x, y, cx, cy;
	unsigned char sal, nu;
	unsigned char status, ct;
	unsigned char *next_frame;
	unsigned char step;
	char jmx;
	unsigned char activo;
	char vidas;
} PERZONAE;

// Globales

PERZONAE nanako;
PERZONAE karakasa;
unsigned char halflife = 0;
unsigned char map_buffer [384];			// 24x16, para simular las celdas de carácter del Speccy.
unsigned char tablero [384];			// 24x16, buffer para mostrar el tablero en plan fashion.
unsigned char paleta  [] = {20, 4, 24, 29, 31, 15, 5, 7, 3, 22, 18, 26, 27, 28, 12, 11};
unsigned char paletaf [] = {20, 4, 21, 23, 5, 3, 7, 24, 0, 20, 20, 20, 20, 20, 20, 20};
char jump_pressed = 0;
char down_pressed = 0;
unsigned char pufx, pufy, pufa;

// Extern globals

extern char asm_number[1];
extern unsigned char s_marco [];
extern unsigned char s_final [];
extern unsigned char cad_game_over1 [];
extern unsigned char cad_game_over2 [];
extern unsigned char cad_newlevel1 [];
extern unsigned char cad_newlevel2 [];
extern unsigned char cad_newlevel3 [];
extern unsigned char cad_title1 [];
extern unsigned char cad_title2 [];
extern unsigned char cad_title3 [];
extern unsigned char cad_title4 [];
extern unsigned char cad_title5 [];
extern unsigned char cad_title6 [];
extern unsigned char cad_title7 [];
extern unsigned char cad_title8 [];
extern unsigned char cad_title9 [];
extern unsigned char cad_title10 [];
extern unsigned char cad_redefine1 [];
extern unsigned char cad_redefine2 [];
extern unsigned char cad_redefine3 [];
extern unsigned char cad_redefine4 [];
extern unsigned char cad_redefine5 [];
extern unsigned char cad_redefine6 [];
extern unsigned char cad_redefine7 [];
extern unsigned char cad_enterpassword [];
extern unsigned char passwords [];
extern unsigned char cad_ending1 [];
extern unsigned char cad_ending2 [];

unsigned char *cad_redefines [] = {
	cad_redefine2, 
	cad_redefine3,
	cad_redefine4,
	cad_redefine5,
	cad_redefine6,
	cad_redefine7
};

// Extern data

#asm
	._asm_number 
		defb 0
	._asm_int_2
		defw 0
	._cad_game_over1
		defm ";GAME;OVER;"
		defb 0
	._cad_game_over2
		defm ";;;;;;;;;;;"
		defb 0
	._cad_title1
		defm "NANAKO;IN"
		defb 0
	._cad_title2
		defm "CLASSIC"
		defb 0
	._cad_title3
		defm "JAPANESE"
		defb 0
	._cad_title4
		defm "MONSTER"
		defb 0
	._cad_title5
		defm "CASTLE"
		defb 0
	._cad_title6
		defm "A;GAME;BY"
		defb 0
	._cad_title7
		defm "THE;MOJON;TWINS"
		defb 0
	._cad_title8
		defm "0;START;GAME"
		defb 0
	._cad_title9
		defm "1;ENTER;PASS"
		defb 0
	._cad_title10
		defm "2;REDEFINE;;"
		defb 0
	._cad_redefine1
		defm "PRESS;A;KEY;FOR"
		defb 0
	._cad_redefine2
		defm "LEFT"
		defb 0
	._cad_redefine3
		defm "RIGHT"
		defb 0
	._cad_redefine4
		defm "DROP;BOX"
		defb 0
	._cad_redefine5
		defm "TAKE;BOX"
		defb 0
	._cad_redefine6
		defm "JUMP"
		defb 0
	._cad_redefine7
		defm "RETRY"
		defb 0
	._cad_enterpassword
		defm "ENTER;PWD"
		defb 0
	._cad_newlevel1
		defm ";LEVEL;XX;"
		defb 0
	._cad_newlevel2
		defm "GOOD;LUCK>"
		defb 0
	._cad_newlevel3
		defm ";;<XXXX<;;"
		defb 0
	._passwords
		defm "lqowwj~kil}thjueh}fuqk}orwnnxwiioegylexq|em~p||jw}wuwfetwzznttlvnyzo|sgxjuwgnns|plgi~pgsmqqrutw}xe}t"
		defb 0
	._cad_ending1
		defm "CONGRATS>;NOW;MYA;IS;SAFE>"
		defb 0
	._cad_ending2
		defm "CYA;IN;HELL>"
		defb 0
#endasm

/********************
 Funciones auxiliares
 ********************/

unsigned char my_strlen (unsigned char *cad) {
	unsigned char res = 0;
	
	while (*cad != 0) {
		cad ++;
		res ++;
	}
	
	return res;
}
 
unsigned char rand () {
	unsigned char res;

	#asm
	._Rand8 ld	a,0 	; Seed is usually 0
			ld	b,a
			add a,a
			add a,a
			add a,b
			inc a		; another possibility is ADD A,7
			;add a, 7
			ld (_Rand8+1),a
			;ld a,r
			
			ld (_asm_number), a
	#endasm

	res = asm_number [0];

	return res;
}

/******************
 Funciones de juego
 ******************/

void tablero_SetTile (unsigned char x, unsigned char y, unsigned char t) {
	tablero [(y << 4) + (y << 3) + x] = t;	
}

unsigned char t_plataformas [] = {
	16,17,16,17,16,17,					// 0
	18,19,18,19,18,19,
	
	36,37,36,37,36,37,					// 12
	38,39,38,39,38,39,
	
	40,41,40,41,40,41,					// 24
	42,43,42,43,42,43,
	
	44,45,48,49,44,45,					// 36
	46,47,50,51,46,47,
	
	52,53,52,53,52,53,					// 48
	54,55,54,55,54,55,
	
	48,49,44,45,48,49,					// 60
	50,51,46,47,50,51
};

void draw_platform (unsigned char x, unsigned char y, unsigned char t) {
	
	unsigned char i;
	unsigned char idx;
	
	switch (t) {
		case 0:
		case 1:
		case 2:
		case 4:	
			idx = (t << 3) + (t << 2);		// t * 8 + t * 4 = t * 12		
			break;
		case 3:
			if (((x / 6) & 1) == 0) 
				idx = 36;
			else
				idx = 60;
	}
	
	for (i = 0; i < 6; i ++) {
		tablero_SetTile (x + i, y    , t_plataformas [idx + i    ]);
		tablero_SetTile (x + i, y + 1, t_plataformas [idx + i + 6]);
		
		// Marcamos como sólido en el mapa de durezas:
		
		map_buffer [y * 24 + x + i] = OBJETO_NO_MOVIL;
		map_buffer [(y + 1) * 24 + x + i] = OBJETO_NO_MOVIL;
	}
}

void draw_supertile (unsigned char x, unsigned char y, unsigned char t) {
	t = t << 2;
	
	tablero_SetTile (x    , y    , t    );
	tablero_SetTile (x + 1, y    , t + 1);
	tablero_SetTile (x    , y + 1, t + 2);
	tablero_SetTile (x + 1, y + 1, t + 3);
}

void draw_supertile_inv (unsigned char x, unsigned char y, unsigned char super_tile) {
	super_tile = super_tile << 2;

	cpc_SetTouchTileXY (x, y, super_tile);
	cpc_SetTouchTileXY (x + 1, y, super_tile + 1);
	cpc_SetTouchTileXY (x, y + 1, super_tile + 2);
	cpc_SetTouchTileXY (x + 1, y + 1, super_tile + 3);
}

void marca_durezas (unsigned char x, unsigned char y, unsigned char d) {
	
	// Marca un objeto de 2x2 tiles en el mapa de durezas como d.
	
	map_buffer [(y << 4) + (y << 3) + x] = d;
	map_buffer [((y + 1) << 4) + ((y + 1) << 3) + x] = d;
	
	// Para marcar los móviles:
	
	if (d == 2) 
		d = 3;
	
	map_buffer [(y << 4) + (y << 3) + x + 1] = d;	
	map_buffer [((y + 1) << 4) + ((y + 1) << 3) + x + 1] = d;
}

void clear_durezas () {
	int i;
	
	for (i = 0; i < 384; i ++)
		map_buffer [i] = VACIO;
}

unsigned char get_dureza (unsigned char x, unsigned char y) {
	return (map_buffer [ (y << 4) + (y << 3) + x ]);	
}

void put_dureza (unsigned char x, unsigned char y, unsigned char d) {
	map_buffer [(y << 4) + (y << 3) + x] = d;
}

pinta_tile_fondo2 (unsigned char x, unsigned char y) {
	unsigned char t;
	
	if ((x & 1) == 0) {
		if ((y & 1) == 0) {
			t = 28;
		} else {
			t = 29;
		}
	} else {
		if ((y & 1) == 0) {
			t = 30;
		} else {
			t = 31;
		}
	}	
	
	cpc_SetTouchTileXY (x, y, t);
}

pinta_tile_fondo (unsigned char x, unsigned char y) {
	pinta_tile_fondo2 (x, y);	
	map_buffer [(y << 4) + (y << 3) + x] = VACIO;
}

void borra_fondo (unsigned char x, unsigned char y) {
	pinta_tile_fondo (x    , y    );
	pinta_tile_fondo (x + 1, y    );
	pinta_tile_fondo (x    , y + 1);
	pinta_tile_fondo (x + 1, y + 1);
}

void borra_fondo2 (unsigned char x, unsigned char y) {
	pinta_tile_fondo2 (x    , y    );
	pinta_tile_fondo2 (x + 1, y    );
	pinta_tile_fondo2 (x    , y + 1);
	pinta_tile_fondo2 (x + 1, y + 1);
}

void draw_screen (char pant) {
	int idx = pant * 21;
	unsigned char platforms;
	unsigned char xy;
	unsigned char tile, oldtile;
	unsigned char x, y;
	unsigned char i, j;
	
	// Borramos el mapa de durezas
	
	clear_durezas ();
	
	// Fondo
	
	for (y = 0; y < 16; y += 2)
		for (x = 0; x < 24; x += 2) {
			tablero_SetTile (x    , y    , 28);
			tablero_SetTile (x + 1, y    , 29);
			tablero_SetTile (x    , y + 1, 30);
			tablero_SetTile (x + 1, y + 1, 31);
		}
	
	// Desconchones
		
	for (i = 0; i < 5; i ++) {
		x = (rand () % 12) << 1;
		y = (rand () % 8) << 1;
		
		tablero_SetTile (x    , y    , 32);
		tablero_SetTile (x + 1, y    , 33);
		tablero_SetTile (x    , y + 1, 34);
		tablero_SetTile (x + 1, y + 1, 35);
	}
	
	// Plataformas
	
	platforms = mapa [idx];
	idx ++;
	
	x = 0; y = 0;
	
	for (i = 0; i < 8; i ++) {
		if (platforms & (1 << i))
			draw_platform (x, y, pant % 5);
		
		x += 6; 
		
		if (x == 24) {
			x = 0;
			y += 8;
		}
	}
	
	// Objetos no móviles.
	
	oldtile = 4;
	
	for (i = 0; i < 10; i ++) {
		do {
			tile = rand () % 3;
			if (tile == 2)
				tile = 3;
		} while (tile == oldtile);
		
		oldtile = tile;
			
		xy = mapa [idx ++];
		
		if (xy) {
			draw_supertile ((xy & 15) << 1, (xy >> 4) << 1, tile);
			marca_durezas ((xy & 15) << 1, (xy >> 4) << 1, OBJETO_NO_MOVIL);
		}
	}
	
	// Objetos móviles.
	
	for (i = 0; i < 10; i ++) {
		xy = mapa [idx ++];
		
		if (xy) {
			draw_supertile ((xy & 15) << 1, (xy >> 4) << 1, 2);
			marca_durezas ((xy & 15) << 1, (xy >> 4) << 1, OBJETO_MOVIL);
		}
	}
	
	// Invalido y muestro los datos en el tilemap.
	
	cpc_ResetTouchedTiles ();
	//cpc_ShowTileMap (0);
	
	// Colocamos a Nanako
	// Las coordenadas son al píxel
	
	nanako.x = 176;
	nanako.y = 112;
	
	while (map_buffer [(nanako.y >> 3) * 24 + 22] != VACIO) {
		nanako.y -= 16;
	}
	
	// Ahora colocamos a Karakasa, que deberá aparecer (si es posible)
	// en el centro de una de las ocho plataformas, si no hay nada.
	
	// Colocación al azar de Karakasa.
	// Se supone que habrá al menos un sitio posible por nivel,
	// o si no tendremos un bonito bucle infinito :-P
	
	// Como el generador de números aleatorios es una verdadera mierda, hay que hacer
	// un parchetón para al menos colocar al bicho...
	
	// Si pasan 10 iteraciones, intentamos colocar al karakasa "probando huecos".
	
	karakasa.activo = 0;
	i = 0;
	while (!karakasa.activo) {
		karakasa.y = 48 + (rand () & 1) * 64;
		karakasa.x = 16 + (rand () & 3) * 48;
		
		if (get_dureza (karakasa.x >> 3, karakasa.y >> 3) == VACIO && (karakasa.y == 112 || (karakasa.y == 48 && get_dureza (karakasa.x >> 3, 1 + (karakasa.y >> 3)) != VACIO)))
			karakasa.activo = 1;
		
		// Si esto se obceca y se prueban más de 20 posiciones no válidas,
		// salimos y colocamos de otra forma.
				
		i ++;
		if (i == 20)
			break;
	}
	
	if (!karakasa.activo) {
		// Este algoritmo va probando de forma secuencial y lo coloca donde pueda
		
		karakasa.activo = 1;
		karakasa.y = 48;
		karakasa.x = 16;
		
		while (	karakasa.activo && (get_dureza (karakasa.x >> 3, karakasa.y >> 3) != VACIO || (	karakasa.y == 48 && get_dureza (karakasa.x >> 3, 2 + (karakasa.y >> 3)) == VACIO))) {
			karakasa.x += 48;
			if (karakasa.x == 208) {
				karakasa.x = 16;
				karakasa.y += 64;
				if (karakasa.y == 176) 
					karakasa.activo = 0;
			}	
		}			
	}
	
	// Okay, very nice!
}

void unveil_tablero () {
	// Muestra el tablero en plan cortinilla.
	// Se supone que la pantalla está "tapada" con el tile 56
	
	char x, y, yy;
	
	for (y = 0; y < 16; y ++) {
		for (x = 0; x < 24; x ++) {
			yy = 15 - y;
			cpc_SetTouchTileXY (x, yy,  tablero [ (yy << 4) + (yy << 3) + x ]);
		}
			
		cpc_UpdScr ();
		cpc_ShowTouchedTiles ();
		cpc_ResetTouchedTiles ();			
	}
	
}

void borra_pantalla () {
	unsigned char i, j;
	
	cpc_ResetTouchedTiles ();	
	
	for (i = 0; i < 16; i ++) {
		for (j = 0; j < 24; j ++)
			cpc_SetTouchTileXY (j, i, 56);
			
		cpc_UpdScr ();
		cpc_ShowTouchedTiles ();
		cpc_ResetTouchedTiles ();	
	}
}

void draw_vidas () {
	unsigned char x = 32;
	unsigned char i;
	
	for (i = 0; i < 5 - nanako.vidas; i ++) {
		cpc_PutTile2x8 ( (unsigned int *) (sp_marcador_vidas [0]), (unsigned char *) (x), (unsigned char *) (169));
		x += 2;
	}
	
	for (i = 0; i < nanako.vidas; i ++) {
		cpc_PutTile2x8 ( (unsigned int *) (sp_marcador_vidas [1]), (unsigned char *) (x), (unsigned char *) (169));
		x += 2;
	}
}

unsigned char levelbox [] = {
	80,  81,  82,  83,  84,  84,  84,  84,  85,  86,  87,  88,
	89,  90,  57,  57,  57,  57,  57,  57,  57,  57,  91,  92,
	93,  57,  57,  57,  57,  57,  57,  57,  57,  57,  57,  94,
	93,  57,  57,  57,  57,  57,  57,  57,  57,  57,  57,  94,
	93,  57,  57,  57,  57,  57,  57,  57,  57,  57,  57,  94,
	95,  96,  57,  57,  57,  57,  57,  57,  57,  57,  97,  98,
	99, 100, 101, 102, 103, 103, 103, 103, 104, 105, 106, 107
};

void draw_levelbox (unsigned char x, unsigned char y) {
	unsigned char i, j;
	
	for (i = 0; i < 7; i ++)
		for (j = 0; j < 12; j ++)
			cpc_SetTile (x + j, y + i, levelbox [12 * i + j]);
}

void mueve_karakasa () {
	// Paradigma del movimiento aleatorio coñaso.
	// Además tiene que hacer no traspasables los tiles que toque su presiosura de cabesa.
	
	unsigned char action;
	unsigned char x, y;
	
	// Decidimos qué hacer:
	
	/*
	action = rand () % 37; // %28;
	if (action < 4)
		karakasa.status = STATE_WLEFT;
	else if (action < 8)
		karakasa.status = STATE_WRIGHT;
	else if (action < 12)
		karakasa.status = STATE_IDLE;
	*/
	
	action = rand () & 15;
	
	if (action < 4) {
		action = rand () % 9;
		if (action < 2)
			karakasa.status = STATE_WLEFT;	
		else if (action < 4)
			karakasa.status = STATE_WRIGHT;
		else
			karakasa.status = STATE_IDLE;
	}
		
	// Si action >= 12, el estado no cambiará y seguirá haciendo lo mismo.
	
	// Desmarco los tiles a la cabeza del Karakasa.
	
	x = karakasa.x >> 3;
	y = karakasa.y >> 3;
	
	switch (karakasa.status) {
		case STATE_IDLE:
			if (karakasa.next_frame == sp_karakasa_1)
				karakasa.next_frame = sp_karakasa_0;
			else if (karakasa.next_frame == sp_karakasa_3)
				karakasa.next_frame = sp_karakasa_2;

			break;
		case STATE_WLEFT:
		
			if (!karakasa.step)
				karakasa.next_frame = sp_karakasa_0;
			else
				karakasa.next_frame = sp_karakasa_1;
			
			karakasa.ct = !karakasa.ct;
			if (karakasa.ct)
				karakasa.step = !karakasa.step;
			
			if (karakasa.x > 0) {
				if ((karakasa.x & 7) == 0) {
					if (get_dureza (x - 1, y - 1) == VACIO && get_dureza (x - 1, y) == VACIO && get_dureza (x - 1, y + 1) == VACIO && get_dureza (x - 1, y + 2) != VACIO) {
						karakasa.x -= 2;
					}
				} else {
					karakasa.x -= 2;
				}
			}
			break;
		case STATE_WRIGHT:
			if (!karakasa.step)
				karakasa.next_frame = sp_karakasa_2;
			else
				karakasa.next_frame = sp_karakasa_3;
			
			karakasa.ct = !karakasa.ct;
			if (karakasa.ct)
				karakasa.step = !karakasa.step;
			
			if (karakasa.x < 176) {
				if ((karakasa.x & 7) == 0) {
					if (get_dureza (x + 2, y - 1) == VACIO && get_dureza (x + 2, y) == VACIO && get_dureza (x + 2, y + 1) == VACIO && get_dureza (x + 2, y + 2) != VACIO) {
						karakasa.x += 2;
					}
				} else {
					karakasa.x += 2;
				}
			}
	}
}

void mueve_nanako () {
	unsigned char x, y, falling;
	unsigned char ox, oy;
	int precalc;
	
	if (!cpc_TestKey (KEY_FIR))
		jump_pressed = 0;	
		
	// Poner/quitar pieza
	
	x = nanako.x >> 3;
	y = nanako.y >> 3;
			
	if (cpc_TestKey (KEY_ABA)) {		
		if (!nanako.status) {
			if (nanako.y < 112) {
				ox = 24;	// ¡Qué guarro soy!
				oy = y + 2;
				if (get_dureza (x - 1, y + 2) == OBJETO_MOVIL) {
					ox = x - 1;
				} else if (get_dureza (x, y + 2) == OBJETO_MOVIL) {
					ox = x;
				} else if (get_dureza (x + 1, y + 2) == OBJETO_MOVIL) {
					ox = x + 1;
				}
				
				if (ox < 24) {
					cpc_WyzStartEffect (WYZ_CANAL_EFECTOS, 1);
					nanako.status = 1;
					borra_fondo (ox, oy);
					cpc_UpdScr ();
					
					pufx = ox << 3;
					pufy = oy << 3;
					pufa = 1;
				}
			}	
		}
	} else if (cpc_TestKey (KEY_ARR)) {
		if (nanako.y > 15) {
			if (nanako.status) {
				if ((nanako.x & 7) == 0) {
					if (get_dureza (x, y - 1) == VACIO && get_dureza (x + 1, y - 1) == VACIO) {
						if ((nanako.y & 7) > 0)
							y ++;
						nanako.y -= 16;
						borra_fondo2 (pufx >> 3, pufy >> 3);
						draw_supertile_inv (x, y, 2);
						cpc_UpdScr ();
						marca_durezas (x, y, OBJETO_MOVIL);
						nanako.status = 0;
						pufx = x << 3;
						pufy = y << 3;
						pufa = 1;
						cpc_WyzStartEffect (WYZ_CANAL_EFECTOS, 2);
					}
				} else {
					if (get_dureza (x, y - 1) == VACIO && get_dureza (x + 1, y - 1) == VACIO && get_dureza (x + 2, y - 1) == VACIO) {
						nanako.y -= 16;
						if ((nanako.y & 7) > 0)
							y ++;
						if ((nanako.x & 7) < 4) {
							borra_fondo2 (pufx >> 3, pufy >> 3);
							draw_supertile_inv (x, y, 2);
							cpc_UpdScr ();
							marca_durezas (x, y, OBJETO_MOVIL);
							nanako.status = 0;
							pufx = x << 3;
							pufy = y << 3;
							pufa = 1;
							cpc_WyzStartEffect (WYZ_CANAL_EFECTOS, 2);
						} else {
							borra_fondo2 (pufx >> 3, pufy >> 3);
							draw_supertile_inv (x + 1, y, 2);
							cpc_UpdScr ();
							marca_durezas (x + 1, y, OBJETO_MOVIL);
							nanako.status = 0;
							pufx = (x + 1) << 3;
							pufy = y << 3;
							pufa = 1;
							cpc_WyzStartEffect (WYZ_CANAL_EFECTOS, 2);
						}
					}
				}
				
				// Vamos a reajustar por si hemos dejado la caja sin estar alineados 
				// verticalmente a tile:
				/*
				if (nanako.status == 0 && (nanako.y & 7) != 0) {
					while ( get_dureza (x, (nanako.y >> 3) + 2) != VACIO || 
							get_dureza (x + 1, (nanako.y >> 3) + 2) != VACIO || 
							get_dureza (x + 2, (nanako.y >> 3) + 2) != VACIO) {
						nanako.y --;
					}
				}
				*/
			}
		}
	}	
				
	if (!nanako.sal) {
		
		x = nanako.x >> 3;
		y = nanako.y >> 3;
		falling = 0;
		
		// ¿Caemos?
		
		if (nanako.y < 112) {
			if ((nanako.y & 7) == 0) {
				if ((nanako.x & 7) == 0) {
					if (get_dureza (x, y + 2) == VACIO && get_dureza (x + 1, y + 2) == VACIO) {
						nanako.y += 2;
						falling = 1;
					}
				} else {
					if (get_dureza (x, y + 2) == VACIO && get_dureza (x + 1, y + 2) == VACIO && get_dureza (x + 2, y + 2) == VACIO) {
						nanako.y += 2;
						falling = 1;
					}
				}
			} else {
				nanako.y += 2;
				falling = 1;
			}
		}
		
		// Tenemos que permitir que Nanako camine sobre Karakasa.
		// Esta es la mejor forma que se me ocurre de hacerlo:
		
		if (falling) {
			if (nanako.x >= karakasa.x - 15 && nanako.x <= karakasa.x + 15)
				if (nanako.y >= karakasa.y - 24 && nanako.y <= karakasa.y - 20) {
					falling = 0;
					nanako.y = karakasa.y - 24;
					y = nanako.y >> 3;
				}
		}
		
		// ¡Y funciona bastante bien! :-D
		
		if (!falling) {
			
			// Andar izquierda / derecha
			
			if (cpc_TestKey (KEY_IZQ)) {
				if (!nanako.step)
					nanako.next_frame = sp_nanako_0;
				else
					nanako.next_frame = sp_nanako_1;
				
				nanako.ct = !nanako.ct;
				if (nanako.ct)
					nanako.step = !nanako.step;
				nanako.jmx = 0;
					
				if (nanako.x > 0) {
					if ((nanako.x & 7) == 0) {
						//if ((nanako.y & 7) == 0) {
							if (get_dureza (x - 1, y) == VACIO && get_dureza (x - 1 , y + 1) == VACIO) {
								nanako.x -= 2;
							}
						/*} else {
							if (get_dureza (x - 1, y) == VACIO && get_dureza (x - 1 , y + 1) == VACIO && get_dureza (x - 1 , y + 2) == VACIO) {
								nanako.x -= 2;
							}	
						}*/
					} else {
						nanako.x -= 2;	
					}
				}
			} else if (cpc_TestKey (KEY_DER)) {
				if (!nanako.step)
					nanako.next_frame = sp_nanako_2;
				else
					nanako.next_frame = sp_nanako_3;
				
				nanako.ct = !nanako.ct;
				if (nanako.ct)
					nanako.step = !nanako.step;
				nanako.jmx = 1;
				
				if (nanako.x < 176) {
					if ((nanako.x & 7) == 0) {
						//if ((nanako.y & 7) == 0) {
							if (get_dureza (x + 2, y) == VACIO && get_dureza (x + 2, y + 1) == VACIO) {
								nanako.x += 2;
							}
						/*} else {
							if (get_dureza (x + 2, y) == VACIO && get_dureza (x + 2, y + 1) == VACIO && get_dureza (x + 2, y + 2) == VACIO) {
								nanako.x += 2;
							}	
						}*/
					} else {
						nanako.x += 2;	
					}
				}
			}
			
			// Salto
			
			if (cpc_TestKey (KEY_FIR) && !jump_pressed) {
				nanako.x = (nanako.x >> 1) << 1;
				nanako.sal = 1;
				nanako.nu = 0;	
				jump_pressed = 1;
				cpc_WyzStartEffect (WYZ_CANAL_EFECTOS, 0);
			}
			
			
		}
	} else {
		x = nanako.x >> 3;
		y = nanako.y >> 3;
		
		nanako.nu ++;
		if (nanako.nu == 8)
			nanako.sal = 0;
			
		// Movimiento vertical
		if (nanako.y > 0) {
			if ((nanako.y & 7) == 0) {
				if ((nanako.x & 7) == 0) {	
					if (get_dureza (x, y - 1) == VACIO && get_dureza (x + 1, y - 1) == VACIO) {
						nanako.y -= 2;
					}
				} else {
					if (get_dureza (x, y - 1) == VACIO && get_dureza (x + 1, y - 1) == VACIO && get_dureza (x + 2, y - 1) == VACIO) {
						nanako.y -= 2;
					}
				}
			} else {
				nanako.y -= 2;
			}
		}
				
		y = nanako.y >> 3;
		
		// Movimiento horizontal
		if (!nanako.jmx) {
			nanako.next_frame = sp_nanako_1;
			if (nanako.x > 0) {
				if ((nanako.x & 7) == 0) {
					if ((nanako.y & 7) == 0) {
						if (get_dureza (x - 1, y) == VACIO && get_dureza (x - 1 , y + 1) == VACIO) {
							nanako.x -= 2;
						}
					} else {
						if (get_dureza (x - 1, y) == VACIO && get_dureza (x - 1 , y + 1) == VACIO && get_dureza (x - 1 , y + 2) == VACIO) {
							nanako.x -= 2;
						}	
					}
				} else {
					nanako.x -= 2;	
				}
			}
		} else {
			if (nanako.x < 176) {
				nanako.next_frame = sp_nanako_2;
				if ((nanako.x & 7) == 0) {
					if ((nanako.y & 7) == 0) {
						if (get_dureza (x + 2, y) == VACIO && get_dureza (x + 2, y + 1) == VACIO) {
							nanako.x += 2;
						}
					} else {
						if (get_dureza (x + 2, y) == VACIO && get_dureza (x + 2, y + 1) == VACIO && get_dureza (x + 2, y + 2) == VACIO) {
							nanako.x += 2;
						}	
					}
				} else {
					nanako.x += 2;	
				}
			}
		}
		
		x = nanako.x >> 3;
		
		if ((nanako.x & 7) == 0) {
			if (get_dureza (x, y + 2) != VACIO || get_dureza (x + 1, y + 2) != VACIO)
				nanako.sal = 0;
		} else {
			if (get_dureza (x, y + 2) != VACIO || get_dureza (x + 1, y + 2) != VACIO || get_dureza (x + 2, y + 2) != VACIO)
				nanako.sal = 0;
		}
	}
}

void set_text_inks (unsigned char i1,unsigned char i2,unsigned char i3,unsigned char i4) {
	// Definimos el color del texto
	// Color Valor
	// 0	0
	// 1	128
	// 2	8
	// 3	136
	// 4	32
	// 5	160
	// 6	40
	// 7	168
	// 8	2
	// 9	130
	// 10	10
	// 11	138
	// 12	34
	// 13	162
	// 14	42
	// 15	170		
	cpc_SetInkGphStr ((unsigned char *) (0), (unsigned char *) (i1));
	cpc_SetInkGphStr ((unsigned char *) (1), (unsigned char *) (i2));
	cpc_SetInkGphStr ((unsigned char *) (2), (unsigned char *) (i3));
	cpc_SetInkGphStr ((unsigned char *) (3), (unsigned char *) (i4));
}

char decode_password (unsigned char *pass) {
	unsigned char i, j;
	unsigned char idx = 25;
	unsigned char found;
	
	// Primero le hago el xor al password que recibí
	
	for (i = 0; i < 4; i ++)
		pass [i] ^= 63;
		
	// Y ahora lo busco en la ristra, para devolver un nº de nivel 0-24 si está o 25 si no.
	// El if es una ristra desenrollada, es más feo programáticamente hablando, pero
	// va follao y es lo que importa.
	
	for (i = 0; i < 100; i += 4) {
		if (	passwords [i    ] == pass [0] &&
				passwords [i + 1] == pass [1] &&
				passwords [i + 2] == pass [2] &&
				passwords [i + 3] == pass [3] ) {
			idx = (i >> 2);
			break;
		}
	}
	
	return idx;
}

void espera_activa (unsigned int times) {
	unsigned int i;
	
	for (i = 0; i < times; i ++) {
		#asm
			halt
		#endasm
		if (cpc_AnyKeyPressed ())
			break;	
	}
}

void backdrop () {
	unsigned char x, y;
	
	for (y = 0; y < 16; y += 2)
		for (x = 0; x < 24; x += 2) {
			cpc_SetTile (x    , y    , 28);
			cpc_SetTile (x + 1, y    , 29);
			cpc_SetTile (x    , y + 1, 30);
			cpc_SetTile (x + 1, y + 1, 31);
		}
		
	cpc_ShowTileMap (0);	
}

void clear_palette (void) {
	unsigned char i;
	
	for (i = 0; i < 16; i ++)
		cpc_SetColour (i, 20);	
}

void set_palette (unsigned char *pal) {
	
	// Establece la paleta del juego.
	
	unsigned char i;
	
	for (i = 0; i < 16; i ++) 
		cpc_SetColour (i, pal [i]);
}

void redefine (void) {
	unsigned char i;

	backdrop ();
	
	set_text_inks (0, 160, 32, 128);
	cpc_PrintGphStrXY ((unsigned int *) (cad_redefine1), (unsigned char *) (23), (unsigned char *) (40));
	
	// Primero, borramos por si acaso hay colisiones:
	
	for (i = 0; i < 6; i ++)
		cpc_AssignKey (i, 0);
	
	// Y ahora dejamos que el usuario redefina.
	set_text_inks (0, 32, 170, 136);
	
	for (i = 0; i < 6; i ++) {
		cpc_PrintGphStrXY ((unsigned int *) (cad_redefines [i]), (unsigned char *) (38 - my_strlen (cad_redefines [i])), (unsigned char *) (56 + (i << 4)));
		cpc_RedefineKey (i);
		while (cpc_TestKey (i));
	}
}

char enter_password (void) {
	unsigned char res;
	unsigned char *pass = "AAAA";
	unsigned char terminado = 0;
	unsigned char x;
	
	for (x = 0; x < 4; x ++)
		pass [x] = 65;
		
	x = 0;
	
	backdrop ();
	
	draw_levelbox (6, 5);
	cpc_ShowTileMap (0);
	
	set_text_inks (0, 160, 32, 128);
	cpc_PrintGphStrXY ((unsigned int *) (cad_enterpassword), (unsigned char *) (29), (unsigned char *) (88));
	
	set_text_inks (0, 32, 170, 136);
	while (!cpc_TestKey (KEY_FIR)) {
			
		cpc_PrintGphStrXY ((unsigned int *) (pass)  , (unsigned char *) (34), (unsigned char *) (104));
		cpc_PutTile2x8 ( (unsigned int *) (sp_marcador_vidas [1]), (unsigned char *) (36 + (x << 1)), (unsigned char *) (112));
		
		if (cpc_TestKey (KEY_IZQ)) {
			cpc_PutTile2x8 ( (unsigned int *) (sp_marcador_vidas [0]), (unsigned char *) (36 + (x << 1)), (unsigned char *) (112));
			if (x == 0)
				x = 3;
			else
				x --;
				
			while (cpc_TestKey (KEY_IZQ));
		} else if (cpc_TestKey (KEY_DER)) {
			cpc_PutTile2x8 ( (unsigned int *) (sp_marcador_vidas [0]), (unsigned char *) (36 + (x << 1)), (unsigned char *) (112));
			x ++;
			if (x > 3)
				x = 0;
				
			while (cpc_TestKey (KEY_DER));
		}
		
		if (cpc_TestKey (KEY_ARR)) {
			pass [x] ++;
			if (pass [x] > 90)
				pass [x] = 65;
				
			while (cpc_TestKey (KEY_ARR));	
		} else if (cpc_TestKey (KEY_ABA)) {
			pass [x] --;
			if (pass [x] < 65)
				pass [x] = 90;
				
			while (cpc_TestKey (KEY_ABA));	
		}
		
	}
	
	while (cpc_TestKey (KEY_FIR));
	
	res = decode_password (pass);
	
	if (res > 24)
		res = 0;
		
	return res;	
}

void draw_menu (void) {
	backdrop ();
	draw_levelbox (6, 0);
	cpc_ShowTileMap (0);

	set_text_inks (0, 168, 2, 128);
	
	cpc_PrintGphStrXY ((unsigned int *) (cad_title1), (unsigned char *) (29), (unsigned char *) (40));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title2), (unsigned char *) (31), (unsigned char *) (48));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title3), (unsigned char *) (30), (unsigned char *) (56));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title4), (unsigned char *) (31), (unsigned char *) (64));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title5), (unsigned char *) (32), (unsigned char *) (72));
	
	set_text_inks (0, 42, 168, 162);
	
	cpc_PrintGphStrXY ((unsigned int *) (cad_title6), (unsigned char *) (29), (unsigned char *) (144));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title7), (unsigned char *) (23), (unsigned char *) (152));
	
	set_text_inks (0, 160, 32, 128);
	
	cpc_PrintGphStrXY ((unsigned int *) (cad_title8), (unsigned char *) (26), (unsigned char *) (96));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title9), (unsigned char *) (26), (unsigned char *) (112));
	cpc_PrintGphStrXY ((unsigned int *) (cad_title10), (unsigned char *) (26), (unsigned char *) (128));	
}

unsigned char menu (void) {
	unsigned char res = 0;
	unsigned char terminado = 0;
	
	// Descomprimimos la canción en el buffer de tiles
	cpc_UnExo ((unsigned int *) (song_0), (unsigned int *) (0x0100));
	cpc_WyzLoadSong (0);
	cpc_WyzSetPlayerOn ();
	
	draw_menu ();
	
	while (!terminado) {
		if (cpc_TestKey (KEY_MENU_S)) {
			terminado = 1;
		} else if (cpc_TestKey (KEY_MENU_P)) {
			res = enter_password ();
			terminado = 1;
		} else if (cpc_TestKey (KEY_MENU_R)) {
			redefine ();
			draw_menu ();
		}
	}
	
	cpc_WyzSetPlayerOff ();
		
	borra_pantalla ();
	
	return res;
}

void game_over (void) {
	unsigned char i, j, k;
	
	// Game over text y pulsar una tecla.
	set_text_inks (0, 42, 168, 162);
	cpc_PrintGphStrXY ((unsigned int *) (cad_game_over2), (unsigned char *) (29), (unsigned char *) (88));
	cpc_PrintGphStrXY ((unsigned int *) (cad_game_over1), (unsigned char *) (29), (unsigned char *) (96));
	cpc_PrintGphStrXY ((unsigned int *) (cad_game_over2), (unsigned char *) (29), (unsigned char *) (104));
	
	espera_activa (1500);
}

void final (void) {
	clear_palette ();
	cpc_UnExo ((unsigned int *) (s_final), (unsigned int *) (0xc000));
	set_palette (paletaf);
	
	// Descomprimimos la canción en el buffer de tiles
	cpc_UnExo ((unsigned int *) (song_0), (unsigned int *) (0x0100));
	cpc_WyzLoadSong (0);
	cpc_WyzSetPlayerOn ();
	
	set_text_inks (0, 40, 160, 168);
	cpc_PrintGphStrXY ((unsigned int *) (cad_ending1), (unsigned char *) (14), (unsigned char *) (184));
	
	espera_activa (1500);
	
	cpc_UnExo ((unsigned int *) (s_final), (unsigned int *) (0xc000));
	cpc_PrintGphStrXY ((unsigned int *) (cad_ending2), (unsigned char *) (26), (unsigned char *) (184));
	
	espera_activa (32767);
	
	cpc_WyzSetPlayerOff ();
	
	clear_palette ();
	cpc_UnExo ((unsigned int *) (s_marco), (unsigned int *) (0xc000));
	set_palette (paleta);
}

void halt_me (void) {
	#asm
		halt				;unas esperas para que se actualicen los colores
		halt
		halt
		halt
		halt
	#endasm
}

void interrupciones(void) {
	// Manejador de interrupciones por Artaburu
	
	#asm
		di
		ld hl,($0039)
		ld (_int_original),HL   //guardo el salto original

		ld HL,_interrupcion
		ld ($0039),HL
		ei
		jp term
	._int_original  
		defw 0
	._interrupcion
		ei
		ret
	.term
	#endasm
}

// *****************************************************************
// *****************************************************************
// *****************************************************************
// *****************************************************************
// *****************************************************************

void main (void) {
	unsigned char i, j, x, y;
	unsigned char flag1 = 0;
	unsigned char flag2 = 0;
	unsigned char dying;
	unsigned char pant, npant;
	unsigned char changing, win, aux;

	struct sprite { 			// estructura mínima para usar la librería de dibujar sprites capturando el fondo.
		int sp0;				// 2 bytes	01
		int sp1;				// 2 bytes	23
		int coord0; 			// 2 bytes	45	 posición en superbuffer
		int coord1; 			// 2 bytes	67	 posición anterior en superbuffer
		unsigned char cx, cy;	// 2 bytes	89   coordenadas nuevas
		unsigned char ox, oy;	// 2 bytes	AB   coordenadas actuales
		unsigned char move; 	// 1 byte    C    si NO es 0, el sprite se mueve.
		unsigned char move1;	// 1 byte    D    si NO es 0, el sprite se mueve.
		// Si es 8 en cuanto se mueva se pone a 0 y ya no se mueve más hasta nueva orden.
	} ;

	struct sprite sp_nanako;
	struct sprite sp_karakasa;
	struct sprite sp_puf;
	
	interrupciones ();
	halt_me ();	
	clear_palette ();
	halt_me ();
	cpc_SetMode (0);

	// Inicializamos el player de wyz:
	
	cpc_WyzInitPlayer ((int) (wyz_sound_table), (int) (wyz_pattern_table), (int) (wyz_effects_table), (int) (wyz_song_table));
	
	// Batería por el canal C:
	
	cpc_WyzConfigurePlayer (0);
	
	// Ponemos el buffer en un sitio adecuado:
	
	cpc_WyzSetBuffer (0x9200);
	
	// UEAH
	
	cpc_SetColour (16, 20);
	
	// Redefinimos las teclas
	
	cpc_AssignKey (KEY_IZQ, 0x4101);		// CUR LEFT
	cpc_AssignKey (KEY_DER, 0x4002);		// CUR RIGHT
	cpc_AssignKey (KEY_ARR, 0x4001);		// CUR UP
	cpc_AssignKey (KEY_ABA, 0x4004);		// CUR DOWN
	cpc_AssignKey (KEY_FIR, 0x4580);		// SPACE
	cpc_AssignKey (KEY_ESC, 0x4804);		// ESC
	cpc_AssignKey (KEY_RET, 0x4604);		// R
	
	// Menú:
	
	cpc_AssignKey (KEY_MENU_S, 0x4401);		// 0 START
	cpc_AssignKey (KEY_MENU_P, 0x4801);		// 1 PASSWORD
	cpc_AssignKey (KEY_MENU_R, 0x4802);		// 2 REDEFINE
	
	// Bucle del juego
	
	cpc_UnExo ((unsigned int *) (s_marco), (unsigned int *) (0xc000));
	set_palette (paleta);
	
	sp_nanako.sp0 = (int) (sp_nanako_0);
	sp_nanako.sp1 = (int) (sp_nanako_0);
	sp_nanako.move = 1;
	
	sp_karakasa.sp0 = (int) (sp_karakasa_0);
	sp_karakasa.sp1 = (int) (sp_karakasa_0);
	sp_karakasa.move = 1;
	
	sp_puf.sp0 = (int) (sp_puf_0);
	sp_puf.sp1 = (int) (sp_puf_0);
	sp_puf.move = 1;
	
	while (1) {
		pant = menu ();
		nanako.vidas = 5;
		flag2 = 0;
			
		while (!flag2) {
			for (y = 0; y < 16; y ++)
				for (x = 0; x < 24; x ++)
					cpc_SetTile (x, y, 56);
			
			draw_levelbox (6, 5);
					
			cpc_ShowTileMap (0);
			
			// Descomprimimos la canción en el buffer de tiles
			cpc_UnExo ((unsigned int *) (song_2), (unsigned int *) (0x0100));
			cpc_WyzLoadSong (2);
						
			// Presentar fase y esperar un tantito
			
			cad_newlevel1 [7] = 48 + (pant / 10);
			cad_newlevel1 [8] = 48 + (pant % 10);
	
			for (i = 0; i < 4; i ++)
				cad_newlevel3 [3 + i] = passwords [(pant << 2) + i] ^ 63;
			
			set_text_inks (0, 168, 2, 128);
			cpc_PrintGphStrXY ((unsigned int *) (cad_newlevel1), (unsigned char *) (28), (unsigned char *) (80));
			cpc_PrintGphStrXY ((unsigned int *) (cad_newlevel3), (unsigned char *) (28), (unsigned char *) (112));
			
			set_text_inks (0, 42, 168, 162);
			cpc_PrintGphStrXY ((unsigned int *) (cad_newlevel2), (unsigned char *) (28), (unsigned char *) (96));		
			
			cpc_WyzSetPlayerOn ();
			
			espera_activa (500);
			
			cpc_WyzSetPlayerOff ();
			
			cpc_ResetTouchedTiles ();	
		
			nanako.sal = 0;
			nanako.nu = 0;
			nanako.next_frame = sp_nanako_0;
			nanako.step = 0;
			nanako.jmx = -2;
			nanako.status = 0;
			nanako.ct = 0;
			
			karakasa.next_frame = sp_karakasa_0;
			
			pufx = pufy = pufa = 0;
			
			// Descomprimimos la canción en el buffer de tiles
			
			cpc_UnExo ((unsigned int *) (song_1), (unsigned int *) (0x0100));
			cpc_WyzLoadSong (1);
						
			draw_screen (pant);
			draw_vidas ();
			unveil_tablero ();
	
			dying = 0;
			flag1 = 0;
			
			sp_nanako.ox = sp_nanako.cx = nanako.x >> 2;
			sp_nanako.oy = sp_nanako.cy = nanako.y;
			
			sp_karakasa.ox = sp_karakasa.cx = karakasa.x >> 2;
			sp_karakasa.oy = sp_karakasa.cy = karakasa.y - 8;
			
			cpc_ResetTouchedTiles ();
			
			cpc_WyzSetPlayerOn ();
			
			while (!flag1) {
				
				if (!dying)
					mueve_nanako ();
				
				mueve_karakasa ();	
				
				// Colisión nanako <-> karakasa
				
				if (!dying && ( (nanako.y >= karakasa.y - 15 && nanako.y <= karakasa.y + 15 && nanako.x >= karakasa.x - 11 && nanako.x <= karakasa.x + 11) || cpc_TestKey (KEY_RET)) ) {
					dying = 1;
					pufx = nanako.x;
					pufy = nanako.y;
					pufa = 1;
					cpc_WyzStartEffect (WYZ_CANAL_EFECTOS, 3);
				}
				
				// ESC
				
				if (cpc_TestKey (KEY_ESC)) {
					dying = 1;
					pufx = nanako.x;
					pufy = nanako.y;
					pufa = 1;
					nanako.vidas = 0;
				}	
				
				// Cambios en la estructura de sprites:
				// (coordenadas, frame, etcétera)
		
				sp_nanako.sp0 = (int) (nanako.next_frame);
				sp_nanako.cx = nanako.x >> 2;
				sp_nanako.cy = nanako.y;
				
				sp_karakasa.sp0 = (int) (karakasa.next_frame);
				sp_karakasa.cx = karakasa.x >> 2;
				sp_karakasa.cy = karakasa.y - 8;
				
				if (pufa) {
					switch (pufa) {
						case 4:
							sp_puf.sp0 = (int) (sp_puf_0);
							break;
						case 8:
							sp_puf.sp0 = (int) (sp_puf_1);
							break;
						case 12:
							sp_puf.sp0 = (int) (sp_puf_2);
					}
					sp_puf.ox = sp_puf.cx = pufx >> 2;
					sp_puf.oy = sp_puf.cy = pufy;
				}
				
				// Condición de pasar pantalla:
				
				if (nanako.sal == 1 && nanako.y == 0) {
					// Pasar pantallas	
					pant ++;
					flag1 = 1;
					cpc_WyzSetPlayerOff ();
				}
				
				// Render
				
				cpc_PutSpTileMap ((int) (sp_karakasa));	
					
				if (!dying)
					cpc_PutSpTileMap ((int) (sp_nanako));
				if (pufa)
					cpc_PutSpTileMap ((int) (sp_puf));
				
				cpc_UpdScr ();
				
				cpc_PutTrSpTileMap2b ((int) (sp_karakasa));
				if (!dying)
					cpc_PutTrSpTileMap2b ((int) (sp_nanako));
				if (pufa)
					cpc_PutTrSpTileMap2b ((int) (sp_puf));
				
				// Puf
				
				if (pufa) {						
					pufa ++;
					if (pufa == 16) {
						pufa = 0;
						if (dying) {
							espera_activa (100);
							if (nanako.vidas == 0) {
								cpc_WyzSetPlayerOff ();
								game_over ();
								flag2 = 1;
							} else {
								cpc_WyzSetPlayerOff ();
								nanako.vidas --;
							}
							
							flag1 = 1;
						}	
					}
				}	
					
				// cpc_SetColour (16, rand () & 27);
				
				cpc_ShowTouchedTiles ();
				cpc_ResetTouchedTiles ();
					
			}
			
			borra_pantalla ();
			
			if (pant == 25) {
				final ();
				flag2 = 1;
			}
		}
	}
}

#asm
	._s_marco
		BINARY "s_marco.bin"
	._s_final
		BINARY "s_final.bin"
#endasm

/*

	Kiss from a rose
	(Seal)

	There used to be a graying tower alone on the sea.
	You became the light on the dark side of me.
	Love remained a drug that's the high and not the pill.

	But did you know,
	That when it snows,
	My eyes become large and
	The light that you shine can be seen.
	
	Baby,
	I compare you to a kiss from a rose on the gray.
	Ooh,
	The more I get of you,
	The stranger it feels, yeah.
	And now that your rose is in bloom.
	A light hits the gloom on the gray.
	
	There is so much a man can tell you,
	So much he can say.
	You remain,
	My power, my pleasure, my pain, baby
	To me you're like a growing addiction that I can't deny.
	Won't you tell me is that healthy, baby?
	
	But did you know,
	That when it snows,
	My eyes become large and the light that you shine can be seen.
	
	Baby,
	I compare you to a kiss from a rose on the gray.
	Ooh, the more I get of you
	The stranger it feels, yeah
	Now that your rose is in bloom.
	A light hits the gloom on the gray,
	
	I've been kissed by a rose on the gray,
	I've been kissed by a rose
	I've been kissed by a rose on the gray,
	...And if I should fall along the way
	I've been kissed by a rose
	...been kissed by a rose on the gray.
	
	There is so much a man can tell you,
	So much he can say.
	You remain
	My power, my pleasure, my pain.
	To me you're like a growing addiction that I can't deny, yeah
	Won't you tell me is that healthy, baby.
	
	But did you know,
	That when it snows,
	My eyes become large and the light that you shine can be seen.
	
	Baby,
	I compare you to a kiss from a rose on the gray.
	Ooh, the more I get of you
	The stranger it feels, yeah
	Now that your rose is in bloom,
	A light hits the gloom on the gray.
	
	Yes I compare you to a kiss from a rose on the gray
	Ooh, the more I get of you
	The stranger it feels, yeah
	And now that your rose is in bloom
	A light hits the gloom on the gray
	
	Now that your rose is in bloom,
	A light hits the gloom on the gray.

*/

