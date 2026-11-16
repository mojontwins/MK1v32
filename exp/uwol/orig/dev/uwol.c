// Jias jias - Este es como el subaquatic, que lleva en desarrollo tanto
// tiempo que el código de aquí abajo da miedo miedor.

#include <spritepack.h>

/* Extern pointers for the graphic definitions: */

#include "aplib.h"
#include "fases.h"
#include "tiles.h"
#include "sprites.h"

/* Allocate space for the stack */

#pragma output STACKPTR=26000

/* Addresses */

#define AD_FREE			60900

/* Game constants */

#define INICIAEFECTO	0xC364
#define WYZPLAYERISR	0xC021	// Esto es INICIO en el código de wyzplayer.asm
#define CARGA_CANCION	0xC05E	// 0xC063
#define SILENCIA_PLAYER	0xC043	// 0xC048

#define	CANAL_FX		1		// Canal por el que suenan los efectos de sonido

char play = 0;

void SetRAMBank() {
#asm
	.SetRAMBank
	
	
	ld	A, ($5B5C)
	and F8h
	or	B
	ld	BC, $7FFD
	ld	($5B5C), A
	out (C), A
	
#endasm
}

/* MusicISR */
#asm
defw 0	// 2 bytes libres
#endasm

void ISR(void) {
	if (play) {
		#asm
			ld b, 1
			call SetRAMBank
			call WYZPLAYERISR
			ld b, 0
			call SetRAMBank			
		#endasm
	}
}

/* Import sp_ClipStruct */

extern struct sp_Rect *sp_ClipStruct;

#asm
	LIB SPCClipStruct
._sp_ClipStruct
	defw SPCClipStruct
#endasm

struct sp_Rect spritesClipValues;
struct sp_Rect *spritesClip;

/* Manage keys and joystick at the same time: */

struct sp_UDK keys;
void *joyfunc;				// Puntero a la función de manejo seleccionada.

/* Create memory allocator for sprite routines */

void *my_malloc(uint bytes)
{
   return sp_BlockAlloc(0);
}

void *u_malloc = my_malloc;
void *u_free = sp_FreeBlock;

/* Strings printed during the game */

struct sp_PSS ps;         /* a print string struct used by sp_PrintString */

/* Data types */

typedef struct {
	int x, y;
	int vx, vy;
	char g, ax, rx;
	uchar salto, cont_salto;
	uchar *current_frame, *next_frame;
	uchar saltando;
	uchar frame, subframe, facing;
	uchar estado;
	uchar ct_estado;
} INERCIA;

#define EST_NORMAL 		 0
#define EST_NUDE 		 1
#define EST_PARP 		 2
#define EST_MUR 		 4
#define EST_EXIT 		 8
#define EST_EXIT_LEFT	16
#define EST_EXIT_RIGHT	32

INERCIA player;

char pantalla;
char visitados [55];
char lives;


// Estructura de datos para los objetos móviles:
typedef struct {
	uchar x, y;
	char vx, vy;
	uchar t1, t2;
	int rx, ry;
	uchar *current_frame, *next_frame; 
	char tipo;
} MOVILES;

MOVILES moviles[3];
struct sp_SS *sp_moviles [3];

// Almacena las monedas del nivel actual
uchar monedas[10];
uchar num_monedas;

// Para ahorrar IFs...
uchar *enem_frames [] = {	
	wolfi_1a,		wolfi_1a,
	franki_1a,		franki_1a,
	vampi_1a,		vampi_1a,
	fanti_l_1a,		fanti_r_1a
};

char flag1, flag2;
uchar playing = 1;
char level;

// Espacio para guardar 12 bloques de fondo.
uchar tile_buffer01 [8];
uchar tile_buffer02 [8];
uchar tile_buffer03 [8];
uchar tile_buffer04 [8];
uchar tile_buffer05 [8];
uchar tile_buffer06 [8];
uchar tile_buffer07 [8];
uchar tile_buffer08 [8];
uchar tile_buffer09 [8];
uchar tile_buffer10 [8];
uchar tile_buffer11 [8];
uchar tile_buffer12 [8];
uchar *tile_buffer [] =		{tile_buffer01, tile_buffer02, tile_buffer03, tile_buffer04, tile_buffer05, tile_buffer06,
							 tile_buffer07, tile_buffer08, tile_buffer09, tile_buffer10, tile_buffer11, tile_buffer12};

// Pelotingui o vestido:
//uchar diferencia;

/* WYZ music & sound calls */

extern char asm_number[1];
extern int asm_int[1];
#asm
	._asm_number	defb 	0
	._asm_int		defw	0
#endasm

uchar rand () {
	uchar res;
#asm
	ld a,r
	ld (_asm_number), a
#endasm

	res = asm_number [0];
	return res;
}

void unpack_RAM3 (unsigned int address) {
	asm_int [0] = address;
	
	#asm	
		di
		ld b, 3
		call SetRAMBank
		
		ld hl, (_asm_int)
		ld de, 16384
		call depack
		
		ld b, 0
		call SetRAMBank
		ei
	#endasm
}

void wyz_play_sound (unsigned char fx_number, unsigned char fx_channel) {
	if (!play)
		return;
	
	asm_int [0] = fx_channel + (fx_number	* 256);
	
	#asm
		di
		ld b,1
		call SetRAMBank
		ld bc, (_asm_int)
		call INICIAEFECTO
		ld b,0
		call SetRAMBank
		ei
	#endasm
}

void wyz_play_music (unsigned char song_number) {
	if (!play)
		return;
	
	asm_int [0] = song_number;

	#asm
		di
		ld b, 1
		call SetRAMBank
		ld a, (_asm_int)
		call CARGA_CANCION
		ld b, 0
		call SetRAMBank
		ei
	#endasm
}

void wyz_stop_sound ()
{
	if (!play)
		return;
	
	#asm
		di
		ld b,1
		call SetRAMBank
		call SILENCIA_PLAYER
		ld b,0
		call SetRAMBank
		ei
	#endasm
}

/* Auxiliary functions */

/*
void cortina ()
{

	//
	//   Rutina de Juan Manuel García,
	//   aparecida en MH 186, sección trucos, 
	//   página 25 
	

	#asm
		ld de, 16384
		ld hl, 16414
		ld b, 192
		xor a
		call cortina_bucle
		ld de, 22528
		ld hl, 22528
		ld b, 24
		ld a, 0;
	.cortina_bucle
		push bc
		push de
		push hl
		call cortina_paca
		pop hl
		push hl
		call cortina_paya
		pop hl
		pop de
		ld bc, 32
		add hl, bc
		ex de, hl
		add hl, bc
		ex de, hl
		pop bc
		djnz cortina_bucle
		ret
	.cortina_paca
		push de
		pop hl
		inc hl
		ld bc, 15
		ldir
		dec hl
		ld (hl), a
		ret
	.cortina_paya
		push hl
		pop de
		inc de
		ld bc, 15
		lddr
		inc hl
		ld (hl), a
		ret	
	#endasm
}
*/

void fade_out () {
	char i;
	for (i = 0; i < 8; i ++) {
		#asm 
			ld   e, 3               ; 3 tercios 
			ld   hl, 22528          ; aquí empiezan los atributos 
			halt                    ; esperamos retrazo. 
		.fade_out_bucle  
			ld   a, (hl )           ; nos traemos el atributo actual 
		 	
			ld   d, a               ; tomar atributo 
			and  7                  ; aislar la tinta 
			jr   z, ink_done        ; si vale 0, no se decrementa 
			dec  a                  ; decrementamos tinta 
		.ink_done 
			ld   b, a               ; en b tenemos ahora la tinta ya procesada. 
			
			ld   a, d               ; tomar atributo 
			and  56                 ; aislar el papel, sin modificar su posiciÃ³n en el byte 
			jr   z, paper_done      ; si vale 0, no se decrementa 
			sub  8                  ; decrementamos papel restando 8 
		.paper_done 
			ld   c, a               ; en c tenemos ahora el papel ya procesado. 
			ld   a, d  
			and  192                ; nos quedamos con bits 6 y 7 (BRIGHT y FLASH) 
			or   c                  ; añadimos paper 
			or   b                  ; e ink, con lo que recompuesto el atributo 
			ld   (hl),a             ; lo escribimos, 
			inc  l                  ; e incrementamos el puntero. 
			jr   nz, fade_out_bucle ; continuamos hasta acabar el tercio (cuando L valga 0) 
			inc  h                  ; siguiente tercio 
			dec  e 
			jr   nz, fade_out_bucle ; repetir las 3 veces 
		#endasm 
	}
}

/*
void guapi_cls () {
	// El primer programa de más de 5 lineas en Z80 ASM de na_th_an 
	#asm
		ld a, 8
	
	.repitatodo
		ld c, a		; Salvamos el contador de "repitatodo" en 'c'
	
		ld hl, 16384
		ld a, 12
	
	.bucle
		ld b, a		; Salvamos el contador de "bucle" en 'b'
		ld a, 255
	
	.bucle1
		sla (hl)
		inc hl
		dec a
		jr nz, bucle1
			
		ld a, 255
	.bucle2
		srl (hl)
		inc hl
		dec a
		jr nz, bucle2
			
		ld a, b		; Restituimos el contador de "bucle" a 'a'
		dec a
		jr nz, bucle
	
		ld a, c		; Restituimos el contador de "repitatodo" a 'a'
		dec a
		jr nz, repitatodo
	#endasm
	// Estoy llorando 
}
*/

unsigned char attr (char x, char y) {	
	char *res = (char *) (22528 + x + (y << 5));
	return (*res);
}

/* Game functions */

char vidaextra;
void draw_lives (char lives) {
	sp_PrintAtInv (1, 6, 71, 99);
	sp_PrintAtInv (1, 7, vidaextra && (player.estado & EST_PARP) && (player.ct_estado & 1) ? 71 : 69, 89 + (lives / 10));
	sp_PrintAtInv (1, 8, vidaextra && (player.estado & EST_PARP) && (player.ct_estado & 1) ? 71 :69, 89 + (lives % 10));
}

void draw_score (unsigned char score) {
	sp_PrintAtInv (1, 24, 71, 99);
	sp_PrintAtInv (1, 25, 69, 89 + (score / 100));
	sp_PrintAtInv (1, 26, 69, 89 + (score % 100) / 10);
	sp_PrintAtInv (1, 27, 69, 89 + (score % 10));
}

void draw_total_score (unsigned int total_score) {
	sp_PrintAtInv (23, 17, 4, 122);
	sp_PrintAtInv (23, 18, 4, 123);
	sp_PrintAtInv (23, 19, 4, 124);
	sp_PrintAtInv (23, 20, 4, 125);
	sp_PrintAtInv (23, 21, 4, 103);
	sp_PrintAtInv (23, 22, 7, 99);
	sp_PrintAtInv (23, 23, 5, 89 + total_score / 10000);
	sp_PrintAtInv (23, 24, 5, 89 + (total_score % 10000) / 1000);
	sp_PrintAtInv (23, 25, 5, 89 + (total_score % 1000) / 100);
	sp_PrintAtInv (23, 26, 5, 89 + (total_score % 100) / 10);
	sp_PrintAtInv (23, 27, 5, 89 + (total_score % 10));
}

void draw_time (char time) {
	sp_PrintAtInv (1, 12, 68, 100);
	sp_PrintAtInv (1, 13, 68, 101);
	sp_PrintAtInv (1, 14, 68, 102);
	sp_PrintAtInv (1, 15, 68, 103);
	sp_PrintAtInv (1, 16, 71, 99);
	sp_PrintAtInv (1, 17, 69, 89 + (time / 100));
	sp_PrintAtInv (1, 18, 69, 89 + (time % 100) / 10);
	sp_PrintAtInv (1, 19, 69, 89 + (time % 10));
}

void draw_level (char level) {
	sp_PrintAtInv (23, 4, 4, 104);
	sp_PrintAtInv (23, 5, 4, 103);
	sp_PrintAtInv (23, 6, 4, 105);
	sp_PrintAtInv (23, 7, 4, 103);
	sp_PrintAtInv (23, 8, 4, 104);	
	sp_PrintAtInv (23, 9, 7, 99);
	sp_PrintAtInv (23, 10, 5, 89 + (level /100));
	sp_PrintAtInv (23, 11, 5, 89 + (level % 10));
}

unsigned char move () {
	uchar xx, yy;
	uchar x, y;
	uchar i;

	i = (joyfunc) (&keys); // Leemos del teclado

	/* Primera prueba del motor de movimiento de inercia para z88dk/splib2
	   Vamos a ver si esto no explota tó */

	/* Por partes. Primero el movimiento vertical. La ecuación de movimien-
	   to viene a ser, en cada ciclo:

	   1.- vy = vy + g
	   2.- y = y + vy

	   O sea la velocidad afectada por la gravedad. 
	   Para no colarnos con los nmeros, ponemos limitadores:
	*/

	if (player.vy < 512/*256*/)
		player.vy += player.g;
	else
		player.vy = 512/*256*/;

	player.y += player.vy;
	if (player.y < 0) 
		player.y = 0;
	
	/* El problema es que no es tan fácil... Hay que ver si no nos chocamos.
	   Si esto pasa, hay que "recular" hasta el borde del obstáculo.

	   Por eso miramos el signo de vy, para que los cálculos sean más sencillos.
	   De paso vamos a precalcular un par de cosas para que esto vaya más rápido.
	*/

	x = player.x >> 6;				// dividimos entre 64 para pixels, y luego entre 8 para tiles.
	y = player.y >> 6;
	xx = x >> 3;
	yy = y >> 3;

	if (player.vy < 0) { 			// estamos ascendiendo
		if (player.y >= 1024)
			if (attr (xx, yy) > 63 || attr (xx + 1, yy) > 63 || ((x & 7) != 0 && attr (xx + 2, yy) > 63)) {
				// paramos y ajustamos:
				player.vy = 0;
				player.y = (yy + 1) << 9;
			}
	} else if (player.vy > 0) { 	// estamos descendiendo
		if (attr (xx, yy + 2) > 63 || attr (xx + 1, yy + 2) > 63 || ((x & 7) != 0 && attr (xx + 2, yy + 2) > 63))
		{
			// paramos y ajustamos:
			player.vy = 0;
			player.y = yy << 9;
		}
	}

	/* Salto: El salto se reduce a dar un valor negativo a vy. Esta es la forma más
	   sencilla. Sin embargo, para más control, usamos el tipo de salto "mario bros".
	   Para ello, en cada pulsación dejaremos decrementar vy hasta un mínimo, y de-
	   tectando que no se vuelva a pulsar cuando estemos en el aire. Juego de banderas ;)
	*/

	if ( ((i & sp_UP) == 0 || (i & sp_FIRE) == 0) && player.vy == 0 && player.saltando == 0 && (attr (xx, yy + 2) > 63 || attr (xx + 1, yy + 2) > 63 || ((x & 7) != 0 && attr (xx + 2, yy + 2) > 63))) {
		player.saltando = 1;
		player.cont_salto = 0;
		wyz_play_sound (7, CANAL_FX);

		// Para que el inicio del salto sea más potente: (a veces conviene):
		// player.vy -= (player.salto << 1);
	}

	if ( ((i & sp_UP) == 0 || (i & sp_FIRE) == 0) && player.saltando ) {
		player.vy -= (player.salto + 48/*16*/ - (player.cont_salto>>1));
		if (player.vy < -256/*128*/) player.vy = -256/*128*/;

		player.cont_salto ++;
		if (player.cont_salto == 8)
			player.saltando = 0;
	}

	if ( (i & sp_UP) != 0 && (i & sp_FIRE) != 0)
		player.saltando = 0;
	
	// ------ ok con el movimiento vertical.

	/* Movimiento horizontal:

	   Mientras se pulse una tecla de dirección, 
	   
	   x = x + vx
	   vx = vx + ax

	   Si no se pulsa nada:

	   x = x + vx
	   vx = vx - rx
	*/

	if ( ! ((i & sp_LEFT) == 0 || (i & sp_RIGHT) == 0))
		if (player.vx > 0) {
			player.vx -= player.rx;
			if (player.vx < 0)
				player.vx = 0;
		} else if (player.vx < 0) {
			player.vx += player.rx;
			if (player.vx > 0)
				player.vx = 0;
		}

	if ((i & sp_LEFT) == 0)
		if (player.vx > -192) {
			player.facing = 0;
			player.vx -= player.ax;
		}

	if ((i & sp_RIGHT) == 0)
		if (player.vx < 192) {
			player.vx += player.ax;
			player.facing = 1;
		}

	player.x = player.x + player.vx;

	/* Ahora, como antes, vemos si nos chocamos con algo, y en ese caso
	   paramos y reculamos */

	y = player.y >> 6;
	x = player.x >> 6;
	yy = y >> 3;
	xx = x >> 3;

	if (player.y >= 1024)
		if (player.vx < 0) {
			if (attr (xx, yy) > 63 || attr (xx, yy + 1) > 63 || ((y & 7) != 0 && attr (xx, yy + 2) > 63)) {
				// paramos y ajustamos:
				player.vx = 0;
				player.x = (xx + 1) << 9;
			}
		} else {
			if (attr (xx + 2, yy) > 63 || attr (xx + 2, yy + 1) > 63 || ((y & 7) != 0 && attr (xx + 2, yy + 2) > 63)) {
				// paramos y ajustamos:
				player.vx = 0;
				player.x = xx << 9;
			}
		}

	// Calculamos el frame que hay que poner:

	if ( ! (player.estado & EST_NUDE) ) {
		if (player.vy != 0) {
			if (player.facing == 0)
				player.next_frame = uwol_l_3a;
			else
				player.next_frame = uwol_r_3a;
		} else {
			if (player.vx == 0) {
				if (player.facing == 0)
					player.next_frame = uwol_l_2a;
				else
					player.next_frame = uwol_r_2a;
			} else {
				player.subframe ++;
				if (player.subframe == 4) {
					player.subframe = 0;
					player.frame = (player.frame + 1) & 3;
				}
				if (player.facing == 0) {
					if (player.frame == 1 || player.frame == 2)
						player.next_frame = uwol_l_2a;
					else if (player.frame == 0)
						player.next_frame = uwol_l_1a;
					else if (player.frame == 3)
						player.next_frame = uwol_l_3a;
				} else {
					if (player.frame == 1 || player.frame == 2)
						player.next_frame = uwol_r_2a;
					else if (player.frame == 0)
						player.next_frame = uwol_r_1a;
					else if (player.frame == 3)
						player.next_frame = uwol_r_3a;
				}
			}	
		}
	} else {
		if (player.vy != 0) {
			if (player.facing == 0)
				player.next_frame = uwolpelot_l_3a;
			else
				player.next_frame = uwolpelot_r_3a;
		} else {
			if (player.vx == 0) {
				if (player.facing == 0)
					player.next_frame = uwolpelot_l_2a;
				else
					player.next_frame = uwolpelot_r_2a;
			} else {
				player.subframe ++;
				if (player.subframe == 4) {
					player.subframe = 0;
					player.frame = (player.frame + 1) & 3;
				}
				if (player.facing == 0) {
					if (player.frame == 1 || player.frame == 2)
						player.next_frame = uwolpelot_l_2a;
					else if (player.frame == 0)
						player.next_frame = uwolpelot_l_1a;
					else if (player.frame == 3)
						player.next_frame = uwolpelot_l_3a;
				} else {
					if (player.frame == 1 || player.frame == 2)
						player.next_frame = uwolpelot_r_2a;
					else if (player.frame == 0)
						player.next_frame = uwolpelot_r_1a;
					else if (player.frame == 3)
						player.next_frame = uwolpelot_r_3a;
				}
			}
		}
	}
	
	// Wrap around
	
	if (player.x <= 1088)
		player.x = 14270;
	
	if (player.x >= 14272)		player.x = 1090;
	
	// Morir en un pit
	
	if (player.y > 10752) {
		player.next_frame = uwolmuete_1a;
		player.estado |= EST_MUR;
		player.vy = - 7 * (player.salto);
		player.y = 10750;
		lives --;
		if (lives >= 0)	draw_lives (lives);
	}
	
	// Saliendo de la fase
	
	if (!(i & sp_DOWN))
		if ( player.estado & EST_EXIT ) {
			if ( player.x > 2204 && player.x < 2880 )
				player.estado |= EST_EXIT_LEFT;
			if ( player.x > 12480 && player.x < 13120 )
				player.estado |= EST_EXIT_RIGHT;
		}
}

void death_sequence () {
	if (player.vy < 1024/*256*/)
		player.vy += player.g;
	else
		player.vy = 1024/*256*/;

	player.y += player.vy;
	
	// Esto acaba cuando el jugador sale de la pantalla:
	
	if (player.y > 11264) {
		player.estado = EST_PARP;
		player.x = 32 << 6;
		player.y = 144 << 6;
		player.vx = player.vy = 0;
		player.ct_estado = 32;						// Tiempo de inmunidad.
		flag1 = 3;
		playing = 0;
	}
}

void draw_tile (char x, char y, unsigned char c, unsigned char tile) {
	sp_PrintAtInv (y, x, c, tile);
	if (x<27) sp_PrintAtInv (y, 1 + x, c, tile + 1);
	if (y<21) sp_PrintAtInv (1 + y, x, c, tile + 2);
	if (x<27 && y<21) sp_PrintAtInv (1 + y, 1 + x, c, tile + 3);
}

// Atributo de cada tile
//uchar attr_tiles[8] = {70, 66, 67, 68, 66, 71, 65, 69};

uchar attr_tiles_ext [32] = {
	68, 68, 69, 69,
	67, 67, 70, 70,
	66, 66, 67, 67,
	69, 69, 71, 69,
	69, 69, 71, 71,
	68, 68, 71, 70,
	71, 71, 70, 70,
	70, 70, 68, 68	
};

void draw_tile_ext (char x, char y, char c, char tile) {
	char add = c << 2;
	sp_PrintAtInv (y, x, attr_tiles_ext[add], tile);
	if (x<27) sp_PrintAtInv (y, 1 + x, attr_tiles_ext[add+1], tile + 1);
	if (y<21) sp_PrintAtInv (1 + y, x, attr_tiles_ext[add+2], tile + 2);
	if (x<27 && y<21) sp_PrintAtInv (1 + y, 1 + x, attr_tiles_ext[add+3], tile + 3);
}

void draw_screen (char r_pant) {
	/* Esta función lee del array de pantallas y pinta la pantalla que
	   se le pasa en el argumento.
	*/
	
	struct sp_Rect rectangulo;
	char x,y,i,j;
	char l,n,v,t1,t2,t;
	char n_pant;
	unsigned char bd_atr, bd_pap, bd_ink;
	
	// Sí, amigos, somos unos trampucheros. No hay 55 pantallas. Hay 45
	// y se repiten 10.
	
	n_pant = r_pant % 45;
	
	// Qué malos somos.
	
	// Calculamos el atributo de fondo correctamente de forma que INK > PAPER
	// si esto no ocurre (puede pasar en las fases repetidas), se cambia.
	
	bd_atr = (level == 10) + (fases[n_pant].descriptor & 63);
	bd_ink = bd_atr & 7;
	bd_pap = bd_atr >> 3;
	if (bd_ink < bd_pap) {
		i = bd_pap;
		bd_pap = bd_ink;
		bd_ink = i;
	}
	bd_atr = bd_ink + (bd_pap << 3);
	
	// Ea, ahora el Anju se queda contento :-P
	
	// Calculamos el tile de fondo según la altura:
	if (level < 4)
		t = 1;
	else if (level < 7)
		t = 5;
	else if (level < 9)
		t = 9;
	else
		t = 13;
	
	// Primero el fondo
	for (y = 0; y < 10; y++)
		for (x = 0; x < 12; x++) {
			draw_tile (4 + (x<<1), 2 + (y<<1), bd_atr, t);
		}

	// Objetos 
	for (i = 0; i < 10; i ++) {
		if (fases[n_pant].obj[i] != 0) {
			x = (unsigned char) (fases[n_pant].obj[i] >> 12) & 15;
			y = (unsigned char) (fases[n_pant].obj[i] >> 8) & 15;
			l = (unsigned char) (fases[n_pant].obj[i] >> 4) & 15;
			n = (unsigned char) (fases[n_pant].obj[i] >> 1) & 7;

			if ( (fases[n_pant].obj[i] & 1) == 0 ) {
				// plataforma horizontal
				for (j = x; j < x + l; j ++)
					draw_tile (5 + (j<<1), 3 + (y<<1), bd_atr, 16 + t);
				for (j = x; j < x + l; j ++)
					draw_tile_ext (4 + (j<<1), 2 + (y<<1), n, 33 + (n<<2));
			} else {
				// plataforma vertical
				for (j = y; j < y + l; j ++)
					draw_tile (5 + (x<<1), 3 + (j<<1), bd_atr, 16 + t);
				for (j = y; j < y + l; j ++)
					draw_tile_ext (4 + (x<<1), 2 + (j<<1), n, 33 + (n<<2));
			}
		}
	}
		
	// Monedas:
	// Act 20091201 :: Sólo pintamos monedas si la habitación NO ha sido visitada :-)
	
	if (!visitados [r_pant]) {
		num_monedas = 0;
	
		rectangulo.width = rectangulo.height = 2;
	
		for ( i = 0; i < 10; i ++) {
			// Primero copiamos los valores en nuestra estructura
			monedas [i] = fases [n_pant].coin [i];
			
			if (monedas [i] != 0) {
				// Y ahora dibujamos
				x = (unsigned char) (fases [n_pant].coin [i] >> 4);
				y = (unsigned char) (fases [n_pant].coin [i] & 15);
	
				// Antes de pintar las monedas, salvamos lo que hay debajo.
				rectangulo.row_coord = 2 + (y << 1);
				rectangulo.col_coord = 4 + (x << 1);
				sp_GetTiles (&rectangulo, tile_buffer [i]);		
	
				draw_tile (4 + (x<<1), 2 + (y<<1), bd_atr, 64 + t);
				
				num_monedas ++;
			}
		}	
	}
	
	// Cargamos ahora los objetos móviles
	for (i = 0; i < 3; i ++) {
		if (fases [n_pant].movil [i] == 0)		// Enemigo desactivado.
			moviles [i].tipo = -1;
		else {	
			v = (unsigned char) (fases [n_pant].movil [i] & 1);
			n = (unsigned char) (fases [n_pant].movil [i] >> 1) & 7;
			y = (unsigned char) (fases [n_pant].movil [i] >> 4) & 15;
			t1 = (unsigned char) (fases [n_pant].movil [i] >> 12) & 15;
			t2 = (unsigned char) (fases [n_pant].movil [i] >> 8) & 15;
			
			
			moviles [i].x = moviles [i].t1 = 32 + (t1 << 4);
			moviles [i].t2 = 32 + (t2 << 4);
			moviles [i].y = 16 + (y << 4);
			moviles [i].vx = v + 1;
			moviles [i].tipo = n;
			
			if (n != 3)
				moviles [i].next_frame = enem_frames [ 1 + (n << 1) ];

			sp_MoveSprAbs (sp_moviles [i], spritesClip, 0, 0,0,0,0);
		}
	}
}

char abs (char x) {
	return x < 0 ? -x : x;
}

void move_moviles () {
	// Esto mueve los tres móviles (en caso de estar definidos, o sea, con tipo != 0
	
	uchar i;
	
	for (i = 0; i < 3; i ++)
		if ( moviles [i].tipo == 3) {
			// Fanty
			if (player.x > moviles [i].rx && moviles [i].vx < 120) {
				moviles [i].vx +=4;
				moviles [i].next_frame = enem_frames [ 1 + (moviles [i].tipo << 1) ];
			} else if (player.x < moviles [i].rx && moviles [i].vx > -120) {
				moviles [i].vx -=4;
				moviles [i].next_frame = enem_frames [ moviles [i].tipo << 1 ];
			}
			
			if (player.y > moviles [i].ry && moviles [i].vy < 120)
				moviles [i].vy +=4;
			else if (player.y < moviles [i].ry && moviles [i].vy > -120)
				moviles [i].vy -=4;
			
			moviles [i].rx += moviles [i].vx;
			moviles [i].ry += moviles [i].vy;
			
			moviles [i].x = (unsigned char) (moviles [i].rx >> 6);
			moviles [i].y = (unsigned char) (moviles [i].ry >> 6);
		} else if ( moviles [i].tipo > -1 && !( player.estado & EST_EXIT )) {
			// Franky o Vampy o Wolfi
			moviles [i].x += moviles [i].vx;
			if ( moviles [i].x <= moviles [i].t1) {
				moviles [i].vx = abs (moviles [i].vx);
				moviles [i].next_frame = enem_frames [ 1 + (moviles [i].tipo << 1) ];
			}
			
			if ( moviles [i].x >= moviles [i].t2) {
				moviles [i].vx = -abs (moviles [i].vx);
				moviles [i].next_frame = enem_frames [ moviles [i].tipo << 1 ];
			}
		}
}

void todo_a_negro () {
	unsigned char *origin;

	origin = (unsigned char *) 22528;
	while (origin < 23296) {
		*origin = 0;
		origin ++;
	}
}

unsigned char j, x, y, xx, yy, xt, yt;
char i;
char idx, fantact;
struct sp_SS *sp_prueba;
char *timer;
unsigned char *wyz_music_flag;
unsigned char *allpurposepuntero;
uchar maincounter = 0;
uchar n_pant;
unsigned int total_score;
unsigned int total_coins;
char time;
uchar xcami, ycami;
struct sp_Rect rectangulo;
uchar prueba[8];
char bonus1, bonus2;

unsigned char game (unsigned char n_pant) {
	unsigned char salida; 
	unsigned int key_y;
	unsigned char aux;
		
	key_y = sp_LookupKey('y');
	
	flag1 = 0;
			
	player.x = 32 << 6;
	player.y = 144 << 6;
	player.vy = 0;
	player.g = 32; //8;
	player.vx = 0;
	player.ax = 24; //8;
	player.rx = 32; //8;
	player.salto = 64;
	player.cont_salto = 1;
	player.saltando = 0;
	player.frame = 0;
	player.subframe = 0;
	player.facing = 1;
	player.estado = EST_NORMAL;
	
	todo_a_negro ();
	draw_screen (n_pant);
	
	time = num_monedas * 2;
	
	draw_lives (lives);
	draw_time (time);
	//draw_level (level);
	draw_level (level);
	draw_score (total_coins);
	draw_total_score (total_score);
		
	draw_tile (4, 0, 71, 106);
	draw_tile (22, 0, 70, 110);	

	maincounter = 0;
	playing = 1;
	fantact = 0;
	vidaextra = 0;
	
	// Calculamos el tile de fondo según la altura:
	if (level < 4)
		wyz_play_music (1);
	else if (level < 7)
		wyz_play_music (2);
	else if (level < 9)
		wyz_play_music (3);
	else
		wyz_play_music (4);
		
	// Si la habitación ya fue visitada, mostramos las salidas y al fantasmilla.
	if (visitados [n_pant]) {
		num_monedas = 0;
		time = 0;
	}
	
	while (playing) {
		maincounter ++;		// Como es un uchar, irá siempre de 0 a 255, con ciclos potencias de 2.
		
		if ( !(maincounter & 31)) {
			if (time > 0) {
				time --;
				draw_time (time);
			} else {
				// ¡Cuando se acaba el tiempo sale el fanti!
				if (time == 0) {
					wyz_stop_sound ();
					wyz_play_music (0);
					
					time --;
					
					// Lógica: buscamos el primer enemigo "vacío". De no haberlo,
					// se sustituye el último enemigo por fanti.
					
					idx = 2; 
					
					for ( i = 0; i < 3; i ++)
						if (moviles [i].tipo == -1) {
							idx = i;
							moviles [idx].x = 127;
							moviles [idx].y = 87;
						}
						
					// Borramos el sprite:
					sp_MoveSprAbs (sp_moviles [idx], spritesClip, 0,0,0,0,0);
					sp_DeleteSpr (sp_moviles [idx]);
					moviles [idx].rx = moviles[idx].x << 6;
					moviles [idx].ry = moviles[idx].y << 6;

					// Creamos a fanti:
					sp_moviles [idx] = sp_CreateSpr(sp_MASK_SPRITE, 3, fanti_r_1a, 2, TRANSPARENT);
					sp_AddColSpr (sp_moviles [idx], fanti_r_1b, TRANSPARENT);
					sp_AddColSpr (sp_moviles [idx], fanti_r_1c, TRANSPARENT);

					fantact = 1;
	
					//sp_MoveSprAbs (sp_moviles [idx], spritesClip, 0, moviles [idx].y >> 3, moviles [idx].x >> 3, moviles [idx].x & 7, moviles [idx].y & 7);
					moviles [idx].current_frame = moviles [idx].next_frame = fanti_r_1a;
					moviles [idx].tipo = 3;
					
					// ¡listo!
				}
			}
		}
		
		sp_UpdateNow();
		
		/* Move sprites */

		if ( !(player.estado & EST_MUR) )
			move ();
		else {
			salida = 2;
			death_sequence ();
		}
		
		move_moviles ();

		x = player.x >> 6;
		y = player.y >> 6;

		/* Collisions & game mechanics: */
		
		if ( player.estado & EST_PARP ) {
			// Duración de la inmunidad:
			player.ct_estado --;
			if (player.ct_estado == 0) {
				player.estado = player.estado & (~EST_PARP);
				vidaextra = 0;
			}
		}
		
		if ( !( player.estado & EST_PARP ) && !( player.estado & EST_MUR ) )		// Colisión con enemigos:
			for ( i = 0; i < 3; i ++ )
				if ( moviles [i].tipo > -1 && ( !( player.estado & EST_EXIT ) || (fantact && i == idx) )  ) {
					if (y >= moviles [i].y - 14 && y <= moviles [i].y + 14 && x >= moviles [i].x - 14 && x <= moviles [i].x + 14) {
						if ( !(player.estado & EST_NUDE) ) {
							player.estado |= EST_NUDE;
							player.estado |= EST_PARP;					// Parpadeamos inmunes.
							player.ct_estado = 32;						// Tiempo de inmunidad.
							
							if (time >= 0) {
								do {
									xcami = 32 + ((rand () % 12) << 4);
									ycami = 16 + (1 + (rand () % 8) << 4);
								} while ( attr ( xcami >> 3, ycami >> 3) > 63 );
								
								rectangulo.row_coord = ycami >> 3;
								rectangulo.col_coord = xcami >> 3;
								sp_GetTiles ( &rectangulo, tile_buffer [11] );
								draw_tile (xcami >> 3, ycami >> 3, 22, 85);
							} else {
								xcami = ycami = 0;
							}

							// El jugador "rebota" una poca.
							player.vy = - 4 * (player.salto);
							player.vx = ((player.x >> 6) - moviles [i].x) * 16;
						} else {
							player.vy = - 7 * (player.salto);
							lives --;

							if (lives >= 0)
								draw_lives (lives);

							player.estado |= EST_MUR;
							player.next_frame = uwolmuete_1a;
						}
						wyz_play_sound (1, CANAL_FX);
						moviles [i].vx =- moviles [i].vx;			// El enemigo se da media vuelta.
					}
				}

		if ( player.estado & EST_NUDE )		// Colisión con la camiseta
			if (y >= ycami - 14 && y <= ycami + 15 && x >= xcami - 15 && x <= xcami + 15) {
				wyz_play_sound (3, CANAL_FX);
				total_score += 15;
				draw_total_score (total_score);
				
				player.estado &= (~EST_NUDE);
				player.estado |= EST_PARP;
				player.ct_estado = 32;
				
				//draw_tile (xcami >> 3, ycami >> 3, fases[n_pant].descriptor & 63, 1 + (fases[n_pant].descriptor >> 4));
				rectangulo.row_coord = ycami >> 3;
				rectangulo.col_coord = xcami >> 3;
				sp_PutTiles ( &rectangulo, tile_buffer [11] );
				sp_Invalidate(&rectangulo, &rectangulo);
			}
			
		if ( num_monedas > 0) {
			// Vemos si colisionamos con alguna moneda.
			for (i = 0; i < 10; i ++)
				if (monedas [i] != 0) {
					xt = (unsigned char) (monedas [i] >> 4) << 1;
					yt = (unsigned char) (monedas [i] & 15) << 1;
					
					xx = 32 + (xt << 3);
					yy = 16 + (yt << 3);

					if (y >= yy - 10 && y <= yy + 10 && x >= xx - 10 && x <= xx + 10) {
						wyz_play_sound (5, CANAL_FX);
						
						monedas [i] = 0;
						num_monedas --;
						total_coins ++;
						draw_score (total_coins);
						total_score ++;
						draw_total_score (total_score);
						rectangulo.row_coord = 2 + yt;
						rectangulo.col_coord = 4 + xt ;
						sp_PutTiles ( &rectangulo, tile_buffer [i] );
						sp_Invalidate(&rectangulo, &rectangulo);
					}
				}
		} else {
			// Si se acaban las monedas, presentamos las salidas.
			if ( !(player.estado & EST_EXIT) ) {
				if ( attr ( 5, 19) < 64 && attr ( 6, 19) < 64)
					draw_tile ( 5, 20, 86, level < 10 ? 114 : 144 );
				if ( attr (25, 19) < 64 && attr (26, 19) < 64)
					draw_tile (25, 20, 86, level < 10 ? 118 : 144 );
					
				player.estado |= EST_EXIT;

				// ¡Saca a todo el mundo de aquí!
				for ( i = 0; i < 3; i ++ )
					if (!fantact || i != idx)
						sp_MoveSprAbs (sp_moviles [i], spritesClip, 0, 0,0,0,0);
			}
		}

		// Retardo para quitar cafeína cuando no hay bishos.
		if ( player.estado & EST_EXIT )
			for (i = 0; i < 126; i ++)
				xt ++;

		// Salir IZQ/DER
		if ( player.estado & EST_EXIT_LEFT ) {
			salida = 0;
			playing = 0;
		} else if ( player.estado & EST_EXIT_RIGHT ) {
			salida = 1;
			playing = 0;
		}
		
		// Cheat (Remove)
		if (sp_KeyPressed (key_y))  {
			num_monedas = 0;
		}
		
		// Vidas extra a los 1000 y a los 2500
		aux = 0;
		if (total_score >= 1000 && !bonus1) {
			aux = 1;
			bonus1 = 1;
		}
		if (total_score >= 2500 && !bonus2) {
			aux = 1;
			bonus2 = 1;
		}
		if (aux) {
			player.estado |= EST_PARP;
			lives ++;
			vidaextra = 1;
			player.ct_estado = 32;
			wyz_play_sound (3, CANAL_FX);
		}
		if (vidaextra) 
			draw_lives (lives);
		
		/* Render */
		
		if ( !(player.estado & EST_PARP) || !(maincounter & 1) )
			sp_MoveSprAbs (sp_prueba, spritesClip, player.next_frame - player.current_frame, y >> 3, x >> 3, x & 7, y & 7);
		else
			sp_MoveSprAbs (sp_prueba, spritesClip, player.next_frame - player.current_frame, 0,0,0,0);	// Parpadeo :o)
		
		player.current_frame = player.next_frame;
		
		if ( !(player.estado & EST_EXIT) )
			for ( i = 0; i < 3; i ++ )
				if ( moviles [i].tipo > -1) {
					sp_MoveSprAbs (sp_moviles [i], spritesClip, moviles [i].next_frame - moviles [i].current_frame, moviles [i].y >> 3, moviles [i].x >> 3, moviles [i].x & 7, moviles [i].y & 7);
					moviles [i].current_frame = moviles [i].next_frame;
				}
		
		if ( (player.estado & EST_EXIT) && fantact) {
			sp_MoveSprAbs (sp_moviles [idx], spritesClip, moviles [idx].next_frame - moviles [idx].current_frame, moviles [idx].y >> 3, moviles [idx].x >> 3, moviles [idx].x & 7, moviles [idx].y & 7);
			moviles [idx].current_frame = moviles [idx].next_frame;
		}
	}
	
	wyz_stop_sound ();

	fade_out ();
	
	todo_a_negro ();

	if (fantact) {
		// Al carao fanti.
		sp_MoveSprAbs (sp_moviles [idx], spritesClip, 0,0,0,0,0);
		sp_DeleteSpr (sp_moviles [idx]);

		// Creamos sprite normal:
		sp_moviles [idx] = sp_CreateSpr(sp_MASK_SPRITE, 2, wolfi_1a, 2, TRANSPARENT);
		sp_AddColSpr (sp_moviles [idx], wolfi_1b, TRANSPARENT);
		sp_AddColSpr (sp_moviles [idx], wolfi_1c, TRANSPARENT);			

		moviles [idx].current_frame = wolfi_1a;
	}
	
	// Tenemos que sacar los sprites del viewport
	
	sp_MoveSprAbs (sp_prueba, spritesClip, 0,0,0,0,0);
	if ( (player.estado & EST_EXIT) && fantact)
		sp_MoveSprAbs (sp_moviles [idx], spritesClip, 0,0,0,0,0);

	if ( flag1 == 0 || flag1 == 1 )
		total_score += (100 + 5 * time);
	
	for (i = 0; i < 3; i ++) 
		sp_MoveSprAbs (sp_moviles [i], spritesClip, 0, 0,0,0,0);
	
	if (lives < 0) 
		salida = 3;
		
	return salida;
}

void piramide (char npant) {
	unsigned char l;
	char c;
	
	wyz_play_music (6);
	
	// Pintamos todo:
	sp_ClearRect(sp_ClipStruct, 0, ' ', sp_CR_TILES);
	
	// Logo "UWOL"
	for (i = 0; i < 8; i ++) {
		sp_PrintAtInv (2, 12 + i, 67, 126 + i);
		sp_PrintAtInv (3, 12 + i, 67, 134 + i);
	}
	
	// Pirámide
	j = 15;
	y = 5;
	l = 0;
	
	for (i = 0; i < 55; i ++) {
		if (i == l * (l + 1) / 2) {
			j -= 2; 
			x = j;
			y ++;
			l ++;
		}
		
		if (n_pant == i)
			c = 94;
		else if (visitados [i])
			c = 86;
		else 
			c = 78;
		
		sp_PrintAtInv (y, x, c, 142);
		sp_PrintAtInv (y, x + 1, c, 143);
		
		x += 2;		
	}	
	
	// Info stats
	sp_PrintAtInv (19, 10, 4, 122);
	sp_PrintAtInv (19, 11, 4, 123);
	sp_PrintAtInv (19, 12, 4, 124);
	sp_PrintAtInv (19, 13, 4, 125);
	sp_PrintAtInv (19, 14, 4, 103);
	sp_PrintAtInv (19, 15, 7, 99);
	sp_PrintAtInv (19, 16, 5, 89 + total_score / 10000);
	sp_PrintAtInv (19, 17, 5, 89 + (total_score % 10000) / 1000);
	sp_PrintAtInv (19, 18, 5, 89 + (total_score % 1000) / 100);
	sp_PrintAtInv (19, 19, 5, 89 + (total_score % 100) / 10);
	sp_PrintAtInv (19, 20, 5, 89 + (total_score % 10));
	
	draw_tile (3, 18, 71, 106);
	sp_PrintAtInv (19, 5, 71, 99);
	sp_PrintAtInv (19, 6, 69, 89 + (lives / 10));
	sp_PrintAtInv (19, 7, 69, 89 + (lives % 10));
	
	draw_tile (23, 18, 71, 110);
	sp_PrintAtInv (19, 25, 71, 99);
	sp_PrintAtInv (19, 26, 69, 89 + (total_coins / 100));
	sp_PrintAtInv (19, 27, 69, 89 + (total_coins % 100) / 10);
	sp_PrintAtInv (19, 28, 69, 89 + (total_coins % 10));
	
	// Visible:
	sp_UpdateNow ();
	
	// Esperamos a que se pulse alguna tecla de control
	do {
		i = (joyfunc) (&keys);	
	} while ( (i & sp_UP) != 0 && (i & sp_DOWN) != 0 && (i & sp_LEFT) != 0 && (i & sp_RIGHT) != 0 && (i & sp_FIRE) != 0);
	
	wyz_stop_sound ();
	
	// Bonito
	fade_out ();
}

char espera_activa (int espera) {
	
	// jL
	
	// Esta función espera un rato o hasta que se pulse una tecla.
	// Si se pulsa una tecla, devuelve 0
	
	// Esta función sólo funciona en Spectrum.
	// en CPC no hay una interrupción cada 20ms, asín que esto no
	// sirve "pa ná".
	
	char res = 1;
	int i;
	
	for (i = 0; i < espera && res; i ++) {
		#asm
			halt
		#endasm
		if (sp_GetKey ()) 
			res = 0;
	}
	
	return res;
}

unsigned char kempston_is_attached;

void keyorkemps (void) {
	// Detecta keyboard o kempston y vuelve.
	
	char terminado = 0;
	
	while (!terminado) {
		if (kempston_is_attached) {
			i = sp_JoyKempston ();
			if ((i & sp_FIRE) == 0) {
				joyfunc = sp_JoyKempston;
				terminado = 1;
			}
		}
		i = sp_JoyKeyboard (&keys);
		if ((i & sp_FIRE) == 0) {
			joyfunc = sp_JoyKeyboard;
			terminado = 1;
		}
	}		
}

/* Main */
void main (void) {
	char fl, show, song;
	unsigned int scraddrr3;
	rectangulo.width = rectangulo.height = 2;

	// By Jon Needle:
	
	#asm
		halt
		in	a, (31)
		inc a
		ld	(_kempston_is_attached), a
	#endasm
	
	/* Substitute BASICs ISR for this thingo: */
	#asm
		di
	#endasm
	
	sp_InitIM2(0xf1f1);						// Does nothing :)
	sp_CreateGenericISR(0xf1f1); 
	sp_RegisterHook(255, ISR);			// Registramos el hook - Gracias Utopian!
	
	#asm 
		ei 
	#endasm 

	play = 1;
	
	/* Initialization stuff */
	
	sp_Initialize (7, 0);
	sp_Border (BLACK);
	sp_AddMemory(0, 50, 14, AD_FREE);		// Allocate for 66 blocks of memory
	ps.flags  = sp_PSS_INVALIDATE;
    ps.bounds = sp_ClipStruct;

	keys.up    = sp_LookupKey('q');
	keys.down  = sp_LookupKey('a');
	keys.left  = sp_LookupKey('o');
	keys.right = sp_LookupKey('p');
	keys.fire  = sp_LookupKey(' ');
	
	joyfunc = sp_JoyKeyboard;

	/* dummy tiles */

	allpurposepuntero = game_tiles;
	for (j = 0; j < 148; j++) {
		sp_TileArray (j, allpurposepuntero);
		allpurposepuntero += 8;
	}

	/* Clip structure -- define it as small as possible! */

	spritesClipValues.row_coord = 2;
	spritesClipValues.col_coord = 4;
	spritesClipValues.height = 20;
	spritesClipValues.width = 24;
	spritesClip = &spritesClipValues;

	/* Create sprites */

	sp_prueba = sp_CreateSpr (sp_MASK_SPRITE, 3, uwol_r_2a, 1, TRANSPARENT);
	sp_AddColSpr (sp_prueba, uwol_r_2b, TRANSPARENT);
	sp_AddColSpr (sp_prueba, uwol_r_2c, TRANSPARENT);
	player.current_frame = player.next_frame = uwol_r_2a;
	
	for ( i = 0; i < 3; i ++) {
		sp_moviles [i] = sp_CreateSpr(sp_MASK_SPRITE, 2, wolfi_1a, 2, TRANSPARENT);
		sp_AddColSpr (sp_moviles [i], wolfi_1b, TRANSPARENT);
		sp_AddColSpr (sp_moviles [i], wolfi_1c, TRANSPARENT);	
		moviles [i].current_frame = wolfi_1a;
	}

	// Pochintro:
	
	unpack_RAM3 (49152);
	wyz_play_sound (7, CANAL_FX);
	espera_activa (200);
	fade_out ();
	unpack_RAM3 (49375);
	wyz_play_sound (7, CANAL_FX);
	espera_activa (200);
	fade_out ();
	unpack_RAM3 (50595);
	wyz_play_sound (7, CANAL_FX);
	espera_activa (500);
	fade_out ();
	
	while (1) {
		
		// Aquí título
		
		unpack_RAM3 (52732);
		wyz_play_music (5);
		keyorkemps ();
		wyz_stop_sound ();
		fade_out ();
	
		// Empezamos	
		n_pant = 0;
		level = 1;
		total_score = 0;
		total_coins = 0;
		lives = 3;
		flag1 = 0;
		fl = 0;
		bonus1 = 0;
		bonus2 = 0;
		
		// player.estado = 0;
		
		for (i = 0; i < 55; i ++ )
			visitados [i] = 0;
		
		while (fl < 3) {
			piramide (n_pant);
			fl = game (n_pant);
			
			show = 0;
			
			if (fl == 0) {
				visitados [n_pant] = 1;		// Marcamos como visitada la pantalla act.
				n_pant = n_pant + level;
				level ++;
			} else if (fl == 1) {
				visitados [n_pant] = 1;		// Marcamos como visitada la pantalla act.
				n_pant = n_pant + level + 1;
				level ++;
			} else if (fl == 3) {
				scraddrr3 = 54338; // Game over.
				show = 1;
				song = 7;
			}
			
			if (level == 11) {
				// Salimos de la pirámide, vamos a calcular si salimos de verdad o no.	
				if (total_coins < 256) {
					scraddrr3 = 54511; // Fin bad
					n_pant = 0;
					level = 1;
					show = 1;
					song = 8;
				} else {
					fl = 4;
					scraddrr3 = 55758; // Fin good
					show = 1;
					song = 9;
				}
			}
			
			if (show) {
				unpack_RAM3 (scraddrr3);
				
				wyz_play_music (song);
				espera_activa (fl == 3 ? 300 : 1500);
				if (fl != 4) wyz_stop_sound ();
				
				fade_out ();
			}
		}
		
		if (fl == 4) {
			// The end!	
			unpack_RAM3 (57455);
			wyz_play_sound (7, CANAL_FX);
			espera_activa (1500);
			wyz_stop_sound ();
			fade_out ();
		}
	}
}

/*

Scary Monsters (And Super Creeps)
David Bowie

She had an horror of rooms
she was tired - you can't hide beat
When I looked in her eyes they were blue but nobody home

She could've been a killer 
if she didn't walk the way she do, and she do
She opened strange doors that we'd never close again

She began to wail 
jealousies scream
Waiting at the light - know what I mean

[CHORUS (twice)]
Scary monsters, super creeps
Keep me running, running scared

She asked me to stay
and I stole her room
She asked for my love
and I gave her a dangerous mind
Now she's stupid in the street
and she can't socialise
Well I love the little girl
and I'll love her till the day she dies

She wails
Jimmy's guitar sound 
jealousies scream
Waiting at the light know what I mean

[CHORUS (twice)]

[CHORUS (twice)]

Run, Run, Run [ad lib]

*/