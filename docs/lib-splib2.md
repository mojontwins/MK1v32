# splib2 en el motor MTE MK1 v3.2 (ZX Spectrum)

Copyleft 2011, 2026 The Mojon Twins

Este documento explica, en detalle, cómo usa el motor la librería de sprites
**Sprite Pack 2.2** (`splib2`), que está en `lib/splib2/`. Está pensado para
quien vaya a escribir código propio (`dev/custom.h`, `dev/extern.h`, o
cualquier cosa que se meta en el bucle del juego) y necesite saber:

- qué funciones de splib2 están disponibles y qué hace cada una,
- cómo las usa el motor y por qué,
- qué formato tienen los datos (tiles, sprites) que splib2 espera,
- qué trampas hay (memoria, planos, formatos, prioridades).

> **Esto sólo aplica al target ZX Spectrum.** El backend de CPC (`src/dev/cpc.h`)
> no usa splib2 en absoluto: implementa sus propios sprites con la librería
> `cpcrslib-mt`. Si estás haciendo un juego para CPC, mira el capítulo 9 del
> manual en vez de este documento.

---

## 1. Qué es splib2 y por qué está aquí

splib2 (Sprite Pack 2.2, de Alvin Albrecht) es un motor de sprites "clásico"
para ZX Spectrum: mantiene una *display list* (la lista lógica de lo que hay
en pantalla), un mapa de celdas *sucias* y una lista de sprites; y en cada
actualización (`SPUpdateNow`) redibuja **sólo** las celdas marcadas como
sucias, composing el fondo (tiles) y los sprites encima.

El motor MK1 v3.2 usa splib2 para todo lo que tiene que ver con el ZX:

- la pantalla de fondo se dibuja **en la display list** de splib2, no
  directamente en la memoria de pantalla;
- los sprites (jugador, enemigos, balas) son sprites de splib2;
- el refresco de pantalla es `SPUpdateNow`;
- la entrada del teclado/mando se lee con las rutinas de splib2.

La versión que hay en el repo es una copia recortada de la distribución
original: el `Makefile.bat` de `lib/splib2/` dice qué módulos se compilan
(`sp.lst`), y **muchos módulos de splib2 no están** (hashtable, huffman,
ratón, listas enlazadas, parte del AY...). Lo que sí está es todo lo que el
motor usa.

### 1.1. Cómo se instala y cómo se enlaza

La librería se instala en el z88dk con `lib/updatelib.bat`:

```
copy splib2\splib2f.lib c:\z88dk10-orig\lib\clibs\splib2f.lib
copy splib2\spritepack.h  c:\z88dk10-orig\include
```

La línea de compilación del target ZX (`src/dev/comp.bat:62`) es:

```
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin churromain.c -DSPECCY
```

y el include de la cabecera lo hace `src/dev/speccy.h:6`:

```c
#include <spritepack.h>
```

### 1.2. Cómo se llama al ensamblador de splib2 desde el motor

sprites de splib2 son código Z80 puro: no hay envoltorio C para todo. El motor
importa los símbolos que necesita con `LIB` dentro de un bloque `#asm`
(`src/dev/speccy.h:8-24`):

```c
#asm
		LIB SPInvalidate
		LIB SPCompDListAddr
		LIB SPMoveSprAbs
		LIB SPTileArray	
		LIB SPPrintAtInv
		LIB SPUpdateNow
		LIB SPInitialize
		LIB SPNullSprPtr
		LIB SPRegisterHook
		LIB SPCreateGenericISR
		LIB SPInitIM2
		LIB SPCreateSpr
		XREF SProtatetblInitialize
		XREF SPDisplayList
#endasm
```

- `LIB xxx` **importa** el símbolo `xxx` (dirección de la rutina) para poder
  llamar a ella con `call xxx`.
- `XREF xxx` **exporta** un símbolo de splib2 para poder referenciarlo desde
  el motor. `SProtatetblInitialize` es la rutina que construye las tablas de
  rotación horizontal (la invoca `SPInitialize`); `SPDisplayList` es la
  dirección de la display list.

El resto de la API (la que sí tiene envoltorio C) se usa normalmente, con los
prototipos de `spritepack.h`.

---

## 2. Mapa de memoria (ZX)

El motor reserva la memoria de arriba de forma fija porque splib2 usa
direcciones absolutas. Esto está documentado en el comentario de
`src/dev/speccy.h:26-52`:

```
23296           isrc              (contador de frames)
23300 - 23449   map_attr          (comportamientos de la pantalla)
23450 - 23599   map_buff          (tiles de la pantalla)
23600 - 23656   en_an_*           (arrays de enemigos)
????? - 24199   STACK
24200 - ?????   BINARIO PRINCIPAL (zorg=24200)
60840 - 61439   AD_FREE           (memoria para los descriptores de sprite)
61440 - 61696   IM2 vector table  (splib2: IM2TABLE = $F000)
61697 - 61936   FREEPOOL (240 bytes)
61937 - 61948   ISR               (splib2: copia del GenericISR en $F1F1)
61952 - 65535   Horizontal Rotation Tables (splib2: SProtatetbl = $F000+$200)
```

Las direcciones no negociables de splib2 (definidas en `lib/splib2/SPconfig.def`):

- `IM2TABLE = $F000`: tabla de vectores IM2, 257 bytes (`$F000-$F100`).
- `SProtatetbl = $F000`, pero el espacio realmente ocupado es `$F200-$FFFF`
  (4 KB): son las tablas que permiten el desplazamiento horizontal de los
  sprites de 0 a 7 píxeles.
- El *GenericISR* se copia a `$F1F1` (`61937`), que es justo donde el motor
  quiere su rutina de interrupción.

El motor define sus constantes en `src/dev/speccy.h:54-62`:

```c
#define STACK_SIZE 			64
#define NUMBLOCKS 			40
#define STACK_ADDR 			61936
#define AD_FREE				61440-(NUMBLOCKS*15)
```

`AD_FREE` es la memoria que se le da al asignador de bloques de splib2 (ver
§5.2). **Si añades sprites, tienes que subir `NUMBLOCKS`** (§9).

---

## 3. El modelo interno de splib2 (lo mínimo que hay que saber)

### 3.1. La display list

`SPDisplayList` es un array de 4 bytes por celda de pantalla (32x24 = 768
celdas, 3072 bytes):

- byte 0: **atributo** (PAPER/INK/BRIGHT/FLASH),
- byte 1: **número de tile** (índice en `SPTileArray`),
- bytes 2-3: puntero (big endian) al primer *char struct* de sprite de esa
  celda, o 0 si no hay ninguno.

`SPCompDListAddr` calcula la dirección de la celda (fila, columna):

```
entrada: A = fila (0..23), C = columna (0..31)
salida : HL = dirección de la celda en la display list
```

El motor la usa mucho: `draw_coloured_tile` escribe directamente en la display
list (`speccy.h:466-549`), y `draw_rectangle` borra el puntero de sprite de
las celdas que cubre (`speccy.h:894-949`), lo que además elimina cualquier
sprite que estuviera encima.

### 3.2. Las celdas sucias

`SPDirtyChars` es un mapa de bits (96 bytes: 32/8 x 24) que marca qué celdas
hay que redibujar. `SPUpdateNow` sólo repinta esas celdas: por eso el motor
usa `sp_PrintAtInv` (que además marca la celda como sucia) o llama a
`SPInvalidate` cuando cambia algo.

### 3.3. `SPTileArray`

`SPTileArray` son 512 bytes: 256 gráficos + 256 paletas. Es la tabla que
traduce "número de tile" → gráfico real en la pantalla:

- `SPTileArray[tile]` = gráfico de 8x8 que se pinta,
- `SPTileArray[256+tile]` = paleta (en modo Spectrum no se usa; el motor
  deja esos bytes a 0).

El motor carga ahí el `tileset.bin` (generado por `ts2bin.exe`), que es
justamente la lista "tile → gráfico" (`speccy.h:171-190`):

```c
	#asm
			ld  b, 0
			ld  hl, SPTileArray
			ld  de, _tileset
		.load_tileset_loop
			ld  (hl), e          ; gráfico del tile
			inc h
			ld  (hl), d          ; paleta = 0
			dec h
			inc hl
			inc de              ; siguiente byte del tileset.bin
			... (8 bytes por tile)
			djnz load_tileset_loop
	#endasm
```

### 3.4. Sprites y *char structs*

Un sprite de splib2 (`struct sp_SS`, 14 bytes) es una lista de columnas, y
cada columna es una lista de *char structs* (`struct sp_CS`, 14 bytes), uno
por cada **fila de caracteres** del sprite:

```c
struct sp_SS {
   uchar row, col, height, width, hor_rot, ver_rot;
   uchar *first, *last_col, *last;
   uchar plane, type;
};

struct sp_CS {
   uchar *next_in_spr;   /* big endian!! */
   uchar *prev;          /* big endian!! */
   uchar spr_attr;       /* tipo (bits 7-6) | plano (bits 5-0) */
   uchar *left_graphic;  /* datos de la columna izquierda */
   uchar *graphic;       /* puntero al bitdef de esta columna */
   uchar hor_rot;        /* página de la tabla de rotación */
   uchar colour;         /* atributo (no se usa en modo Spectrum) */
   uchar *next;          /* big endian!! */
   uchar unused;
};
```

- `plane` (0..63): **menor plano = más delante**. El motor usa plano 1 para
  el jugador y las balas, y plano 2 para los enemigos, de modo que el jugador
  se ve por delante de los enemigos.
- `type`: `sp_MASK_SPRITE` (0x00), `sp_OR_SPRITE` (0x40), `sp_XOR_SPRITE`
  (0x80), `sp_LOAD_SPRITE` (0xC0).
- `graphic` apunta al *bitdef* de la columna: los datos de los píxeles.
- `left_graphic` apunta a los datos de la columna de la izquierda; es lo que
  permite el desplazamiento horizontal de 0..7 píxeles (los píxeles que
  "se salen" por la izquierda se toman de la columna vecina).

El motor, como es habitual, usa estas estructuras a mano en ensamblador
(`fix_sprites`, `speccy.h:1248-1302`), así que **cualquier cambio en la
estructura de splib2 rompe el motor**.

---

## 4. El flujo de un frame en el motor

En cada frame (`src/dev/mainloop.h`), el motor:

1. mueve los enemigos y el jugador;
2. llama a `render_all_sprites()` (`speccy.h:1035`), que llama a
   `SPMoveSprAbs` para cada sprite (jugador, enemigos, balas);
3. espera a que se acumulen los *faps* del frame (el ISR cuenta en `isrc`);
4. llama a `SPUpdateNow` (`mainloop.h:348`), que repinta las celdas sucias.

Y al cambiar de pantalla, `draw_scr` (`engine.h:2798`) redibuja el fondo
completo en la display list.

---

## 5. La API de splib2 que usa el motor, función por función

### 5.1. Inicialización e interrupciones

#### `SPInitIM2`

- **Qué hace**: prepara la tabla de vectores IM2 (257 bytes en `$F000`) con
  todos los vectores apuntando al manipulador por defecto que se le pase, y
  selecciona el modo de interrupción IM2.
- **Parámetros**: `BC` = dirección del ISR por defecto.
- **Uso**: `speccy.h:149-150`

```c
			di 
			ld  bc, 0xf1f1 	// el GenericISR, copiado aquí abajo
			call SPInitIM2
```

- **Ojo**: hay que llamarlo con las interrupciones deshabilitadas (el motor
  hace `di` antes y `ei` al final de todo el bloque).

#### `SPCreateGenericISR`

- **Qué hace**: copia el ISR genérico de splib2 a la dirección que se le pase
  en `DE`. Ese ISR ejecuta una cadena de *hooks*; si un hook deja el carry
  puesto, la cadena se corta y no se ejecutan los hooks siguientes.
- **Parámetros**: `DE` = dirección donde colocar el ISR.
- **Salida**: `DE` = dirección justo past el ISR copiado.
- **Uso**: `speccy.h:152-153` → `ld de, 0xf1f1; call SPCreateGenericISR`.
  El sitio está elegido para que el ISR quepa en el hueco `$F101-$F1FF` que
  el motor deja libre.

#### `SPRegisterHook`

- **Qué hace**: añade un *hook* a la lista de hooks del ISR genérico del
  vector indicado.
- **Parámetros**: `L` = número de vector (255 en el motor), `BC` = dirección
  del hook.
- **Uso**: `speccy.h:155-157`

```c
			ld  l, 255
			ld  bc, _ISR
			call SPRegisterHook
```

- El hook del motor es `ISR()` (`speccy.h:137-142`), que sólo incrementa el
  contador `isrc` (23296). Ese contador es el que usa `mainloop.h:332-341`
  para esperar el número de faps del frame (`MIN_FAPS_PER_FRAME`).

#### `SPInitialize`

- **Qué hace**: (1) rellena la display list con el tile por defecto, (2)
  marca toda la pantalla como sucia, (3) construye las tablas de rotación
  horizontal.
- **Parámetros**: `D` = paleta del tile por defecto, `E` = número del tile
  por defecto.
- **Uso**: `speccy.h:159-160` → `ld de, 0` (tile 0, paleta 0).

### 5.2. Memoria: el asignador de bloques

#### `sp_AddMemory(queue, number, size, addr)`

- **Qué hace**: añade `number` bloques de `size` bytes a la cola `queue`,
  a partir de la dirección `addr`. Cada bloque ocupa `size + 1` bytes (el byte
  extra es un identificador oculto de la cola).
- **Parámetros**: `queue` (0..4), `number`, `size` (bytes), `addr`.
- **Salida**: `HL` = dirección justo past la memoria reservada.
- **Uso**: `speccy.h:165`

```c
	sp_AddMemory (0, NUMBLOCKS, 14, AD_FREE);
```

Es decir: 40 bloques de 14 bytes (más su byte de ID = 15 bytes cada uno) en
`AD_FREE` (60840), que es justo la cola 0 del asignador.

#### `sp_BlockAlloc(queue)`

- **Qué hace**: reserva un bloque de la cola indicada.
- **Salida**: puntero al bloque, o `NULL` si no hay memoria.
- splib2 usa este asignador **por dentro** para crear sprites: cada `sp_SS`
  y cada `sp_CS` son 14 bytes, y splib2 los pide con `u_malloc`.

#### `u_malloc` / `u_free`

splib2 no sabe cómo reservar memoria: usa dos punteros globales que el
programa debe definir (`spritepack.h:31-32`). El motor los engancha al
asignador de bloques (`speccy.h:91-93`):

```c
void *my_malloc(uint bytes) { return sp_BlockAlloc(0); }
void *u_malloc = my_malloc;
void *u_free = NULL; //sp_FreeBlock;
```

- `u_free` está a `NULL` porque **el motor nunca destruye sprites**: los
  sprites se crean una vez al arrancar y viven hasta el final de la partida.
  Si necesitas destruir un sprite, tendrías que implementar `u_free` (con
  `sp_FreeBlock`) y usar `sp_DeleteSpr`.

### 5.3. Fondo: la display list

#### `sp_PrintAtInv(row, col, colour, udg)`

- **Qué hace**: escribe en la display list el par (atributo, número de tile)
  de una celda y marca esa celda como sucia.
- **Parámetros**: `row` (0..23), `col` (0..31), `colour` (atributo ZX),
  `udg` (número de tile, índice en `SPTileArray`).
- **Uso en el motor**:
  - `draw_coloured_tile` con sombras automáticas (`speccy.h:282-285`,
    `304-318`): escribe la mitad sombreada o normal del metatile.
  - `draw_text` / `draw_2_digits` / `draw_line_of_text`
    (`speccy.h:445`, `648`, `659`, `713`): el texto del juego.
  - `print_hex` (debug, `speccy.h:1612`).

- Ojo: `draw_coloured_tile` **no** usa `sp_PrintAtInv` en el caso normal
  (sin sombras): escribe directamente en la display list con
  `SPCompDListAddr`, porque tiene que pintar un metatile 2x2 entero
  (`speccy.h:464-549`).

#### `SPCompDListAddr(row, col)`

- **Qué hace**: devuelve la dirección de la celda en la display list.
- **Uso**: `speccy.h:469` (pintado del metatile) y `speccy.h:921`
  (`draw_rectangle`).

#### `SPInvalidate(row, col, row2, col2)`

- **Qué hace**: marca un rectángulo de celdas como sucias (para que
  `SPUpdateNow` las repinte).
- **Parámetros**: `B` = fila superior, `C` = columna izquierda, `D` = fila
  inferior, `E` = columna derecha, `IY` = rectángulo de recorte (clip).
- **Uso**:
  - `set_map_tile` (`speccy.h:539-548`), que invalida el metatile 2x2 que
    acaba de cambiar, con el clip `fsClipStruct` (pantalla completa).
  - `draw_rectangle` (`speccy.h:908-909`).

#### `SPUpdateNow`

- **Qué hace**: repinta todas las celdas sucias: pone los tiles del fondo y
  encima los sprites que solapen esas celdas.
- **Uso**: `mainloop.h:30` (antes de la pantalla de título),
  `mainloop.h:348` (cada frame), `pantallas.h:40,61,85,100,147` (las
  pantallas fijas) y `engine.h:2981`.

- Detalle importante: el motor **no** llama a `SPUpdateNow` el frame en que
  cambia la pantalla (`mainloop.h:343-349` compara `n_pant` con `on_pant`),
  porque ese caso lo lleva `draw_scr`.

### 5.4. Sprites

#### `sp_CreateSpr(type, rows, graphic, plane)`

- **Qué hace**: crea un sprite. Reserva el `sp_SS` (14 bytes) y un `sp_CS`
  por cada fila de caracteres, e inicializa la primera columna con el
  *bitdef* `graphic`.
- **Parámetros**:
  - `type`: `sp_MASK_SPRITE` (por defecto), `sp_OR_SPRITE`, `sp_XOR_SPRITE`,
    `sp_LOAD_SPRITE`.
  - `rows`: **número de filas de caracteres** del sprite (3 = 24 px de alto).
  - `graphic`: puntero al bitdef de la primera columna.
  - `plane`: 0..63, menor = más delante.
- **Salida**: puntero al `sp_SS`, o `NULL` si no hay memoria.
- **Uso**: `speccy.h:196-219` (jugador y enemigos) y `speccy.h:251`
  (balas).

```c
	#ifdef NO_MASKS
		sp_player = sp_CreateSpr (NO_MASKS, 3, sprite_2_a, 1);
	#else
		sp_player = sp_CreateSpr (sp_MASK_SPRITE, 3, sprite_2_a, 1);
	#endif
```

- El jugador y los enemigos se crean con `rows = 3` (16x24 como máximo) y
  **3 columnas** (la tercera es virtual, ver más abajo). Con eso un sprite
  "de 16x16" ocupa 1 + 3x3 = 10 bloques del asignador.

#### `sp_AddColSpr(sprite, graphic)`

- **Qué hace**: añade una columna de 8 píxeles al sprite. Reserva un `sp_CS`
  por fila y apunta el `left_graphic` de la nueva columna a los datos de la
  columna anterior (es lo que permite el desplazamiento horizontal).
- **Uso**: `speccy.h:198-199` (jugador), `205-206` (enemigos), `252`
  (balas).

```c
		sp_player = sp_CreateSpr (sp_MASK_SPRITE, 3, sprite_2_a, 1);
		sp_AddColSpr (sp_player, sprite_2_b);
		sp_AddColSpr (sp_player, sprite_2_b);	// dummy, se sobrescribe luego
```

#### La "tercera columna virtual" (`fix_sprites`)

El motor crea **tres** columnas por sprite, pero la tercera no dibuja nada:
`fix_sprites` (`speccy.h:1248-1302`) recorre la lista de char structs del
sprite y pone `SPNullSprPtr` como puntero gráfico de los que están a partir
del sexto (índice 6 = la tercera columna).

- `SPNullSprPtr` es un bloque de 8 bytes "vacío" (`0,255,0,255,...`): la
  librería lo reconoce y salta ese char struct al dibujar (y al mover), así
  que la columna no produce nada.
- El recorrido de `fix_sprites` es **a mano, en ensamblador**, sobre la
  estructura `sp_CS`: mira el byte `spr_attr` (offset 4) y el puntero
  `graphic` (offset 7).

#### `SPMoveSprAbs` / `sp_MoveSprAbs`

- **Qué hace**: mueve un sprite a una posición absoluta.
- **Registros** (la versión ensamblador es la que usa el motor):

```
IX = sprite (sp_SS)
BC = desplazamiento de bitdef para la animación (0 = sin animación)
H  = fila nueva (en caracteres)
L  = columna nueva (en caracteres)
D  = desplazamiento horizontal en píxeles (0..7)
E  = desplazamiento vertical en píxeles (0..7)
IY = rectángulo de recorte (clip)
```

- Lo que hace internamente:
  - si las coordenadas de celda cambian, invalida el área que ocupaba el
    sprite y lo reinserta (o lo saca) de la lista de sprites de la display
    list, respetando el orden por planos;
  - recalcula los punteros gráficos de todos los char structs aplicando el
    desplazamiento vertical y el de animación;
  - si el sprite se sale del clip, no lo dibuja.

- **Uso en el motor**:
  - `render_this_enemy` (`speccy.h:951-1032`): `call SPMoveSprAbs` en la
    línea 1016.
  - `render_all_sprites` (`speccy.h:1035-1198`): jugador en la línea 1122,
    balas en la 1170, y las llamadas para "esconder" los sprites inactivos
    (1193, 1208, 1230, 1243).
  - La espada (`ENABLE_SWORD`), línea 1243.

- El motor calcula la posición así (ejemplo del jugador,
  `speccy.h:1100-1122`):

```c
			ld  a, (_rdy)		// rdy = píxeles
			srl a
			srl a
			srl a
			add VIEWPORT_Y		// fila en caracteres
			ld  h, a 

			ld  a, (_rdx)
			srl a
			srl a
			srl a
			add VIEWPORT_X		// columna en caracteres
			ld  l, a 
			
			ld  a, (_rdx)
			and 7				// píxel horizontal
			ld  d, a 

			ld  a, (_rdy)
			and 7				// píxel vertical
			ld  e, a 
			
			call SPMoveSprAbs
```

  O sea: el motor trabaja en píxeles, splib2 en caracteres + resto.

#### Los rectángulos de recorte (clip)

El motor define dos rectángulos en ensamblador (`speccy.h:109-112`):

```c
#asm
	.fsClipStruct defb 0, 24, 0, 32
	.vpClipStruct defb VIEWPORT_Y, VIEWPORT_Y + 20, VIEWPORT_X, VIEWPORT_X + 30
#endasm
```

- `fsClipStruct`: pantalla completa (se usa para invalidar).
- `vpClipStruct`: el *viewport* del juego (por defecto 30x20 celdas en
  (1,2)): es el clip que se pasa a `SPMoveSprAbs`, de modo que los sprites
  que se salen del viewport no se dibujan.

El formato es: fila superior, fila inferior, columna izquierda, columna
derecha (más 1).

> La variable C `spritesClipValues` (`speccy.h:106-107`) es un `struct
> sp_Rect` que **no se usa** en el backend ZX: el motor pasa directamente
> `vpClipStruct` en `IY`.

### 5.5. Entrada

#### `sp_JoyKeyboard(keys)`, `sp_JoyKempston`, `sp_JoySinclair1`, `sp_JoySinclair2`

- **Qué hacen**: leen el joystick/teclado y devuelven un byte con los cinco
  botones: `F111RLDU`, **activos a nivel bajo** (1 = NO pulsado).
- **Parámetro**: un `struct sp_UDK` con los códigos de teclado (sólo para
  `sp_JoyKeyboard`).
- **Uso**: el motor guarda el puntero a la función en `joyfunc`
  (`speccy.h:75`) y la llama en `pad_read` (`speccy.h:754-760`):

```c
void pad_read (void) {
	pad_this_frame = pad1;
	pad1 = pad0 = (joyfunc) (&keys); 

	// Keys held this frame
	pad_this_frame = (~pad_this_frame) | pad1;
}
```

- Los bits son: `sp_FIRE` = 0x80, `sp_RIGHT` = 0x08, `sp_LEFT` = 0x04,
  `sp_DOWN` = 0x02, `sp_UP` = 0x01 (`spritepack.h:45-49`).
- El motor invierte la polaridad: `pad_this_frame` tiene un bit a **0**
  cuando la tecla se ha pulsado *en este frame* (flanco de bajada), y a 1 en
  cualquier otro caso. Por eso el motor testea
  `(pad_this_frame & sp_FIRE) == 0` para "tecla de acción recién pulsada"
  (`mainloop.h:383-389`, `engine.h:1968`).

- La tabla de teclas por defecto (`speccy.h:68-74`) usa el `struct sp_UDK`:

```c
struct sp_UDK keys = {
	0x017f, // .fire
	0x01df, // .right
	0x02df, // .left
	0x01fd, // .down
	0x01fb	// .up
};
```

- `select_controls` (`speccy.h:880-892`) deja que el jugador elija mando con
  tres teclas (`key_1`, `key_2`, `key_3`) y asigna la función correspondiente
  a `joyfunc`.

#### `sp_KeyPressed(scancode)`

- **Qué hace**: comprueba si una tecla está pulsada, usando un código de
  barrido de teclado.
- **Parámetro**: código de 16 bits: `(fila << 8) | máscara`.
- **Salida**: 1 si está pulsada, 0 si no.
- **Uso**: `mainloop.h:383` (tecla M para el scripting) y `mainloop.h:486`
  (teclas de depuración).

---

## 6. El formato de los datos de sprite

Este es el punto donde más gente se equivoca, así que léelo con calma.

### 6.1. El *bitdef*

Los datos de un sprite son una secuencia de bytes, uno por cada **línea de
píxeles** de una columna de 8 píxeles de ancho. splib2 espera **dos bytes por
línea**: el gráfico y la máscara.

- En un sprite de tipo `sp_MASK_SPRITE`, la píxel se calcula como
  `(pantalla & máscara) | gráfico`: los bits a 1 de la máscara dejan pasar el
  fondo (sprite transparente), los bits a 0 lo sustituyen.
- En un sprite de tipo `sp_OR_SPRITE` (y `XOR`, `LOAD`), **la máscara se
  ignora** y sólo se hace `pantalla | gráfico`.

El *bitdef* completo de una columna de un sprite de 2 filas de caracteres
(16 px de alto) son 2 x 16 = 32 bytes: 16 líneas por fila de caracteres,
2 bytes por línea.

Y hay un detalle importante: **cada columna de un sprite tiene su propio
bitdef**, y las columnas se añaden con `sp_AddColSpr`. El desplazamiento
vertical de 0..7 píxeles se consigue desplazando el puntero al bitdef, y
para que eso no se salga del dato, splib2 espera que el bitdef tenga **8
líneas de relleno** al final (8 bytes en modo OR, 16 en modo máscara).

### 6.2. El generador: `sprcnv.exe`

`src/utils/sprcnv.exe` convierte el PNG de sprites en el `speccy/sprites.h`.
Lo llama `comp.bat:24`:

```
..\utils\sprcnv.exe ..\gfx\%om%\sprites.png speccy\sprites.h
```

El PNG de sprites (`gfx/speccy/sprites.png`) tiene celdas de 32x16 píxeles
por sprite. Cada celda contiene:

- los **16 píxeles de la izquierda**: el gráfico del sprite,
- los **16 píxeles de la derecha**: la máscara del sprite.

El generador produce tres etiquetas por sprite:

- `._sprite_N_a`: la primera columna (8 px) con gráfico y máscara,
- `._sprite_N_b`: la segunda columna (8 px),
- `._sprite_N_c`: relleno (24 líneas de `0,255`), que el motor usa como
  columna "virtual".

Y produce 16 sprites por bloque: `sprite_1` ... `sprite_16`.

Los sprites extra (bala, explosión, sprite vacío) están a mano en
`src/dev/speccy/extrasprites.h`, escritos a mano en el mismo formato.

### 6.3. Los "sprite cells": la tabla de frames

El motor no guarda un índice de frame: guarda **punteros al bitdef**.

- `player.current_frame` / `player.next_frame` (offsets 15 y 17 de la
  estructura `INERCIA`).
- `en_an_current_frame[]` / `en_an_next_frame[]` para los enemigos.

Estos punteros se sacan de una tabla, `sprite_cells[]`, que por defecto es
(`speccy.h:122-131`):

```c
	unsigned char *sprite_cells [] = {
		sprite_1_a, sprite_2_a, sprite_3_a, sprite_4_a,
		...
		sprite_13_a, sprite_14_a, sprite_15_a, sprite_16_a
	};
```

y que se puede sustituir por la tuya activando `CUSTOM_SPRITE_CELLS` en
`config.h` y creando `dev/custom_sprite_cells.h` (mira
`games/ababol/dev/custom_sprite_cells.h` como ejemplo).

El índice dentro de la tabla es:

- para el jugador: `player.frame` (`engine.h:2102-2114`),
- para los enemigos: `en_an_base_frame[enit] + en_an_frame[enit]`
  (`engine.h:2763-2796`).

---

## 7. El truco del desplazamiento: cómo se anima un sprite

Este es el mecanismo más importante para entender cómo se anima una cosa en
el motor, y no es obvious.

Cuando el frame de un sprite cambia, el motor **no** reconstruye el sprite:
le pasa a `SPMoveSprAbs` un **desplazamiento** `BC` que se suma a los
punteros gráficos de todos los char structs. Es decir: "mueve el sprite 32
bytes más allá en el bitdef".

El cálculo está en `speccy.h:1093-1098` (jugador):

```c
			ld  hl, (_player + 17)		// player.next_frame
			ld  de, (_player + 15)		// player.current_frame
			or  a
			sbc hl, de				// hl = next_frame - current_frame
			ld  b, h
			ld  c, l					// BC = desplazamiento de animación
```

y el mismo cálculo para los enemigos en `speccy.h:970-990`.

**Consecuencia**: los bitdefs de los frames de un mismo sprite tienen que
estar en memoria en el orden correcto y con el mismo "paso". En la práctica:

- cada sprite N del PNG genera un bloque de memoria de tamaño fijo
  (`_a` + `_b` + `_c`),
- y los bloques de los sprites N y N+1 son contiguos,

de modo que `sprite_(N+1)_a - sprite_N_a` es siempre el mismo valor, y el
desplazamiento sirve **para todas las columnas a la vez** (columna 1 →
columna 1 del frame nuevo, columna 2 → columna 2 del frame nuevo).

Por eso, en ZX, **cada frame de animación es un sprite distinto del PNG**
(`sprite_1_a`, `sprite_2_a`, `sprite_3_a`...), y no los "b" y "c" de un mismo
sprite. Si mezclas frames de sprites distintos con pasos distintos, la
animación se descuadra.

---

## 8. Recetas: cómo escribir tu propio código con splib2

### 8.1. Crear un sprite propio

1. Añade el sprite al PNG (`gfx/speccy/sprites.png`), en una celda de 32x16,
   con el gráfico a la izquierda y la máscara a la derecha.
2. Regenera `speccy/sprites.h` (lo hace `comp.bat`).
3. Crea el sprite en `system_init()` (o donde toque):

```c
	sp_miSprite = sp_CreateSpr (sp_MASK_SPRITE, 3, sprite_5_a, 1);
	sp_AddColSpr (sp_miSprite, sprite_5_b);
	sp_AddColSpr (sp_miSprite, sprite_5_b);   // columna virtual
```

4. **Suma los bloques al presupuesto**: `NUMBLOCKS` (en `speccy.h:59`).
   Cada sprite de 3 filas y 3 columnas cuesta 1 + 9 = 10 bloques; uno de
   8x8 (2 filas, 2 columnas) cuesta 5.
5. Muévelo cada frame con `sp_MoveSprAbs` o con el mismo patrón en
   ensamblador que usa el motor.

### 8.2. Animar un sprite propio

- Pon los frames en el `sprite_cells[]` (o en tu `custom_sprite_cells.h`).
- Guarda el puntero al frame en `player.next_frame` / `en_an_next_frame[]`.
- El motor se encarga del resto: calcula el desplazamiento y lo pasa a
  `SPMoveSprAbs`.

### 8.3. Escribir en pantalla

Hay dos formas:

- **`sp_PrintAtInv(row, col, colour, tile)`**: la forma fácil. Escribe en la
  display list y marca la celda como sucia.
- **Escribir en la display list a mano**: lo que hace el motor con los
  metatiles. Usa `SPCompDListAddr` para obtener la dirección de la celda y
  escribe: atributo, número de tile, y **deja intactos los dos bytes del
  puntero de sprite** (si los tocas, borras los sprites de esa celda).

Si escribes directamente en la memoria de pantalla (`16384`), el próximo
`SPUpdateNow` lo borra: la pantalla *real* de splib2 es la display list.

### 8.4. Invalidar una zona

`SPInvalidate(B, C, D, E)` con `IY` = clip. Si has cambiado algo en la
display list a mano, llama a `SPInvalidate` para que se repinte.

### 8.5. Entrada

Usa `sp_KeyPressed(scancode)` para teclas sueltas, o el mecanismo del motor
(`pad0`, `pad1`, `pad_this_frame`) para el juego. Los bits de los botones son
`sp_FIRE`, `sp_UP`, `sp_DOWN`, `sp_LEFT`, `sp_RIGHT`.

---

## 9. Trampas y límites

- **Memoria**: `NUMBLOCKS = 40` da para el jugador (10) + 3 enemigos (30) y
  nada más. Si activas `PLAYER_CAN_FIRE` (3 balas = 15 bloques más) o
  `ENABLE_SWORD`, **sube `NUMBLOCKS`** y sube `AD_FREE` en consecuencia
  (`AD_FREE` se calcula solo: `61440-(NUMBLOCKS*15)`). Si te quedas sin
  memoria, `sp_CreateSpr` devuelve `NULL` y el motor peta al mover el sprite.
- **El sprite no tiene color propio**: en modo Spectrum el atributo de la
  celda lo pone el tile de fondo; el sprite se dibuja con la tinta de esa
  celda. Si quieres que un sprite no se vea sobre un tile, ponle el bit 7
  (FLASH) en el atributo: con `FLASHASFG = 1` en `SPconfig.def`, splib2 no
  dibuja sprites sobre celdas con el bit 7 del atributo.
- **El orden de dibujo es por planos**: menor plano = delante. Jugador y
  balas van en el plano 1, los enemigos en el 2.
- **El clip manda**: los sprites fuera del `vpClipStruct` no se dibujan
  (aunque estén "en pantalla" en otras coordenadas).
- **`SPUpdateNow` es lo único que pinta**: si escribes en la pantalla a
  mano, sobrevive sólo hasta el siguiente `SPUpdateNow`.
- **El texto y los tiles usan la display list**: `draw_text`,
  `draw_2_digits` y `draw_line_of_text` (este último es el que usa el
  intérprete MSC4 para el `TEXT BOX`) escriben con `SPPrintAtInv`.
- **La ISR**: si añades otro hook de splib2 (o tocas el vector IM2),
  recuerda que el motor ya usa el vector 255 para su contador de frames.
- **No toques las estructuras**: `sp_SS` y `sp_CS` se recorren a mano en
  ensamblador (`fix_sprites`), con offsets hard-coded.
- **El `sp_Rect` de C no es el clip**: el clip en ZX es un `struct` de 4
  bytes en ensamblador (`vpClipStruct`), no un `struct sp_Rect`.

---

## 10. Índice rápido

- `SPInitIM2` — prepara la tabla de vectores IM2 y selecciona IM2.
  `speccy.h:149`.
- `SPCreateGenericISR` — copia el ISR genérico (para encadenar hooks).
  `speccy.h:152`.
- `SPRegisterHook` — añade un hook al ISR genérico del vector 255.
  `speccy.h:155`.
- `SPInitialize` — display list vacía, pantalla sucia, tablas de rotación.
  `speccy.h:159`.
- `SPTileArray` — tabla tile → gráfico. `speccy.h:171`.
- `sp_AddMemory` — da 40 bloques de 14 bytes al asignador. `speccy.h:165`.
- `sp_BlockAlloc` / `u_malloc` — asignador de bloques. `speccy.h:91`.
- `sp_CreateSpr` — crea un sprite. `speccy.h:197`, `203`, `215`, `251`.
- `sp_AddColSpr` — añade una columna. `speccy.h:198`, `204`, `252`.
- `SPMoveSprAbs` — mueve el sprite. `speccy.h:1016`, `1122`, `1170`,
  `1193`, `1208`, `1230`, `1243`.
- `SPNullSprPtr` — columna virtual. `speccy.h:1290`.
- `SPPrintAtInv` / `sp_PrintAtInv` — escribe tile + atributo en la display
  list. `speccy.h:282`, `445`, `648`, `659`, `713`.
- `SPCompDListAddr` — dirección de una celda de la display list.
  `speccy.h:469`, `922`.
- `SPInvalidate` — marca celdas como sucias. `speccy.h:548`, `909`.
- `SPUpdateNow` — repinta las celdas sucias. `mainloop.h:30`, `348`;
  `pantallas.h:40,61,85,100,147`; `engine.h:2981`.
- `sp_JoyKeyboard` / `sp_JoyKempston` / `sp_JoySinclair1` — lectura de
  mandos. `speccy.h:168`, `885-888`.
- `sp_KeyPressed` — tecla suelta. `mainloop.h:383`, `486`.

### Ficheros relacionados

- `lib/splib2/` — la librería (fuente en ensamblador + `spritepack.h`).
- `lib/splib2/SPconfig.def` — la configuración con la que se compiló la
  librería (`NOFLICKER = 1`, `COMPRESS = 0`, `FLASHASFG = 1`,
  `IM2TABLE = $F000`, `SProtatetbl = $F000`, `NUMQUEUES = 5`).
- `lib/splib2/readme` — el README original de Sprite Pack 2.2.
- `src/dev/speccy.h` — el backend ZX: aquí está todo el uso de splib2.
- `src/dev/speccy/sprites.h` — los datos de los sprites (generado).
- `src/dev/speccy/extrasprites.h` — bala, explosión y sprite vacío.
- `src/utils/sprcnv.bas` — el generador de `sprites.h`.
- `misc/zx-splib2-demo/` — ejemplo standalone que usa splib2 sin el motor.
