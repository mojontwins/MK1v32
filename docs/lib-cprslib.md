# cpcrslib-mt en el motor MTE MK1 v3.2 (Amstrad CPC)

Copyleft 2011, 2026 The Mojon Twins

Este documento explica, en detalle, cómo usa el motor la librería
**CPCRSLIB "Mojon Tilemap"** (`cpcrslib-mt`), que está en `lib/cpcrslib-mt/`.
Está pensado para quien vaya a escribir código propio (`dev/custom.h`,
`dev/extern.h`, o cualquier cosa que se meta en el bucle del juego) y necesite
saber:

- qué hace la librería y cómo está recortada,
- qué contrato tiene con el motor (qué exporta cada lado),
- cómo se dibuja una pantalla, cómo se actualiza y qué son las "celdas
  tocadas",
- qué son las 35 rutinas de sprite y cuál tienes que usar,
- cómo se leen las teclas,
- qué trampas hay (unidades de coordenadas, modos, LUTs, clipping).

> Contrasta con [`lib-splib2.md`](lib-splib2.md), que es el documento
> equivalente para el target ZX Spectrum. Los dos backends son **completamente
> distintos**: el ZX usa splib2 (display list + sprites con máscaras), el CPC
> usa cpcrslib-mt (nametable + superbuffer + celdas sucias).

---

## 1. Qué es cpcrslib-mt

`cpcrslib-mt` es una **recortada** de la CPCRSLIB original (Raúl Simarro /
Artaburu, 2007) modificada por na_th_an para encajar con el modelo del motor
MTE MK1 v3.2:

- **Lo que se ha quitado**: todo lo que no es tilemap ni *dirty cells*. No
  hay gestión de teclado (`cpc_AssignKey`, `cpc_ScanKeyboard`...), ni
  impresión, ni gráficos genéricos, ni el gestor de tilemap completo, ni
  actualizadores alternativos de pantalla. Tampoco hay *hashtable*, ni
  sprites genéricos fuera del modelo de tilemap.
- **Lo que se ha cambiado**: la "lista de tiles tocados" (pares X,Y) se ha
  sustituido por un **bitfield** de 96 bytes (1 bit por celda de 32x24),
  igual que hace splib2 con su `SPDirtyChars`. La marca ACME ("Actualización
  de Código Mojona Eructo").
- **Lo que se ha añadido**: las rutinas de sprite al pixel (las `*Px*`), que
  dibujan sprites de cualquier posición con rotación de bytes mediante LUTs.

Quedan 50 módulos (la lista está en `lib/cpcrslib-mt/cpcrslib.lst`):

- **Núcleo**: `cpc_ShowTileMap`, `cpc_UpdScr`, `cpc_ShowTouchedTiles`,
  `cpc_ResetTouchedTiles`, `cpc_UpdTileTable`, `cpc_InvalidateRect`,
  `cpc_KeysData`, `cpc_TestKeyboard`, `cpc_TestKey`, `cpc_SetMode`,
  `cpc_MakeM1RotationLUTs`.
- **Dependencias internas** (no se declaran en la cabecera):
  `cpc_Bit2Mask`, `cpc_TblLookup`, `cpc_bitleft2mask`, `cpc_bitright2mask`.
- **Sprites**: la matriz completa de tamaños y formatos de pixel que el
  generador de assets (`mkts_om.exe mode=sprites`) puede pedir.

### 1.1. Las dos variantes

- `cpcrslib_mt.lib`: la variante normal.
- `cpcrslib_fg_mt.lib`: la variante **fg** ("foreground"), compilada con
  `PASARPORDETRAS = 1`, que permite que los sprites **pasen por detrás** de
  ciertos tiles del fondo.

La diferencia se compila, no se activa en runtime: la variante fg necesita
además que el juego exporte la tabla `behindtilemasks` (256 bytes, 1 por
tile: 1 = "primer plano", el sprite queda detrás; 0 = normal). La usan
**Papaya** y **Sir Ababol**.

La configuración de compilación está en `lib/cpcrslib-mt/CPCconfig.def`
(que `make.bat` sobrescribe con `CPCconfig_normal.def` o
`CPCconfig_fg.def`):

```
DEFC PASARPORDETRAS = 1     ; sólo en la variante fg
DEFC CHURRERA_CLIPPING = 1  ; las marcas se recortan al viewport
DEFC M1LUTR1 = $D700
DEFC M1LUTR2 = $DE00
DEFC M1LUTR3 = $E700
```

### 1.2. Instalación y enlazado

`lib/updatelib.bat` la instala en el z88dk:

```
copy cpcrslib-mt\cpcrslib_mt.lib    c:\z88dk10-orig\lib\clibs\
copy cpcrslib-mt\cpcrslib_fg_mt.lib c:\z88dk10-orig\lib\clibs\
copy cpcrslib-mt\cpcrslib_mt.h      c:\z88dk10-orig\include\
```

La línea de compilación del target CPC (`src/dev/comp.bat:81`) es:

```
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o %game%.bin system\tilemap_conf.asm churromain.c -DCPC -DMODE_%mode%
```

Dos cosas a destacar:

1. **El `tilemap_conf.asm` va en la línea de compilación**, antes que el
   `.c`. Es el módulo que hace de contrato con la librería (ver §2).
2. La variante fg se enlaza cambiando `-lcpcrslib_mt` por
   `-lcpcrslib_fg_mt`.

La cabecera la incluye `src/dev/cpc.h:6`:

```c
#include <cpcrslib_mt.h>
```

---

## 2. El contrato entre el motor y la librería

La librería **no** es autónoma: usa símbolos que aporta el juego. Eso está
documentado en el propio `cpcrslib_mt.h` (líneas 17-22).

### 2.1. Lo que exporta el motor

Vive en `src/dev/system/tilemap_conf.asm` (`XLIB TileMapConf`), que se
compila con el juego:

- `_nametable` = `$100`: la nametable (el "display list" del CPC). 32x24
  bytes, uno por celda, con el número de tile.
- `pantalla_juego` = `$100`: la misma cosa (alias).
- `tiles`: los gráficos de los tiles (los produce `mkts_om.exe`).
- `tiles_tocados` = `$CE00`: el bitfield de celdas sucias (96 bytes).
- `posiciones_pantalla`, `posiciones_super_buffer`: tablas con la dirección
  del inicio de cada línea de la pantalla visible y del superbuffer.
- `posicion_inicial_area_visible` = `$C000`: dónde está la parte visible de
  la pantalla.
- `posicion_inicial_superbuffer` = `$9000`: dónde está el superbuffer.
- `ancho_pantalla_bytes` = 64, `alto_pantalla_bytes` = 192,
  `ancho_pantalla_bytes_visible` = 64.
- `behindtilemasks` (sólo la variante fg): 256 bytes, 1 por tile.
- `mascara1` / `mascara2` = `$00` / `$00`: máscaras de los "garbled bits" del
  modo 0 (ver §6.6; la biblioteca actual no las usa).

Y en `src/dev/cpc.h:9-26`, el motor exporta además:

```c
#asm
	// Imports

		XREF _nametable
		XREF tabla_teclas
		LIB cpc_KeysData
		LIB cpc_UpdTileTable
		LIB cpc_InvalidateRect
		LIB cpc_TestKeyboard

	// Exports

		XDEF viewport_x
		XDEF viewport_y

		defc viewport_x = VIEWPORT_X
		defc viewport_y = VIEWPORT_Y
#endasm
```

`viewport_x` / `viewport_y` los usa la librería para recortar las marcas de
celdas al área de juego (opción `CHURRERA_CLIPPING`).

### 2.2. Lo que exporta la librería

La cabecera `cpcrslib_mt.h` declara **sólo** la API que existe de verdad en
las dos variantes compiladas. Lo que el motor usa:

- En C: `cpc_Border`, `cpc_SetMode`, `cpc_MakeM1RotationLUTs`,
  `cpc_ShowTileMap`, `cpc_TestKey`, y las 35 rutinas de sprite.
- En ensamblador (importadas con `LIB`): `cpc_KeysData`,
  `cpc_UpdTileTable`, `cpc_InvalidateRect`, `cpc_TestKeyboard`.

Los símbolos internos (`cpc_Bit2Mask`, `cpc_TblLookup`, etc.) no se tocan.

---

## 3. Mapa de memoria (CPC)

El motor lo documenta en `src/dev/cpc.h:49-66` y las direcciones están en
`src/dev/definitions.h:46-56`:

```
0100        tilemap        (_nametable / pantalla_juego)
0400        ejecutable     (zorg=1024)
8800        descomprimir canción
9000        superbuffer    (posicion_inicial_superbuffer)
C000        pantalla       (posicion_inicial_area_visible, a trozos)
C600        room buffers
CE00        dirty cells    (tiles_tocados)
D600        arrays
D700        M1 LUT R1      (M1LUTR1)
DE00        M1 LUT R2      (M1LUTR2)
DF80        buffers WYZ
E600        estructuras de sprite  (BASE_SPRITES)
E700        M1 LUT R3      (M1LUTR3)
FE00        LUT            (BASE_LUT: LUT de píxeles transparentes)
```

Las direcciones **no** son negociables: la librería las tiene fijadas en
`CPCconfig.def` (las LUTs M1) y en los `defc` de `tilemap_conf.asm` (el
resto). Si las cambias, dejas de ser compatible con la librería.

---

## 4. El modelo: nametable, superbuffer y celdas sucias

A diferencia de splib2 (que tiene una display list y la redibuja), el CPC
trabaja con **tres** buffers:

1. **La nametable** (`$100`): 32x24 bytes, uno por celda, con el número de
   tile de cada celda. Es el "estado lógico" de la pantalla. El motor
   escribe aquí (nunca en la memoria de pantalla).
2. **El superbuffer** (`$9000`): el buffer de píxeles donde se compone todo
   (fondo + sprites), en el formato real de píxeles del modo activo.
3. **El área visible** (`$C000`): lo que se ve en el CRTC.

Y un **bitfield de celdas sucias** (`tiles_tocados`, 96 bytes en `$CE00`): 1
bit por celda de la nametable, que dice "esta celda hay que recomponer".

El ciclo de un frame es:

1. El motor modifica la nametable (pintando tiles) y marca las celdas
   afectadas con `cpc_UpdTileTable`.
2. `cpc_UpdateNow(1)`:
   a. por cada sprite, se llama a su `invfunc`, que marca en el bitfield las
      celdas que ocupa el sprite **en su posición anterior y en la nueva**;
   b. `cpc_UpdScr`: por cada celda marcada, se copia el gráfico del tile
      desde la nametable al superbuffer (esto **borra** los sprites que
      estuvieran ahí);
   c. por cada sprite, se llama a su `updfunc`, que lo dibuja en el
      superbuffer;
   d. se espera el número de faps del frame;
   e. `cpc_ShowTouchedTiles`: se copian del superbuffer a la pantalla
      visible **sólo** las celdas marcadas;
   f. `cpc_ResetTouchedTiles`: se limpia el bitfield.

Ese paso (e) es lo que hace que el CPC no parpadee: sólo se tocan las
celdas que han cambiado.

### 4.1. Escribir en la nametable

El motor tiene una rutina interna, `__tile_address` (`cpc.h:598-626`), que
devuelve en `DE` la dirección del byte de la nametable que corresponde a
(`_x`, `_y`):

```c
	// HL = _y * 32 + _x + _nametable
```

La usan:

- `draw_coloured_tile` (`cpc.h:642-997`): escribe los 4 tiles del
  metatile 2x2 (los números `64 + _t*4` .. `67 + _t*4`) y marca las 4
  celdas con `cpc_UpdTileTable`.
- `set_map_tile` (`cpc.h:999-1056`): cambia un tile del mapa y llama a
  `draw_coloured_tile`.
- `draw_2_digits` (`cpc.h:1068-1113`): escribe 2 tiles y marca 2 celdas.
- `draw_text` (`cpc.h:1115-1191`): escribe el texto (los caracteres ASCII
  menos 32) y marca las celdas con `cpc_InvalidateRect`.

---

## 5. La API usada, función por función

### 5.1. `cpc_SetMode(mode)`

- **Qué hace**: cambia el modo de vídeo del CPC escribiendo el Gate Array.
- **Parámetro**: `L` (fastcall) = 0 (2 colores) o 1 (4 colores).
- **Uso**: `cpc.h:438-442`

```c
	#ifdef MODE_1
		cpc_SetMode (1);
	#else
		cpc_SetMode (0);
	#endif
```

- Ojo: `cpc_SetMode` **no** configura el CRTC. El motor lo hace a mano
  después (`cpc.h:447-481`), con la "geometría retocada" de 32x24
  caracteres (CRTC reg 1 = 32 columnas, reg 2 = 42, reg 6 = 24 filas; y
  reg 5 = 4 si hay `AUTO_SPLIT`).

### 5.2. `cpc_Border(colour)`

- **Qué hace**: pone el borde del CPC del color indicado.
- **Parámetro**: `L` (fastcall) = el valor del borde.
- **Uso**: `cpc.h:287-294` (definición del motor) y `cpc.h:412`
  (`cpc_Border (0x54)` en `system_init`). El macro `BORDER(b)` del motor
  (`cpc.h:133`) llama a esta función.

### 5.3. `cpc_ShowTileMap(param)`

- **Qué hace**: dos cosas, según el parámetro:
  - `L = 0`: **recompone** toda la pantalla: pasa los 768 tiles de la
    nametable al superbuffer y luego vuelca el superbuffer a la pantalla
    visible.
  - `L = 1`: **sólo vuelca** el superbuffer a la pantalla visible (sin
    recomponer desde la nametable).
- **Uso**:
  - `mainloop.h:39` y `pantallas.h:96`: `cpc_ShowTileMap (1)` tras
    desempaquetar una pantalla fija (el bitmap ya está en el superbuffer).
  - `cpc_ShowTileMap (0)` no lo llama el motor de forma directa, pero es lo
    que usarías para forzar un repintado completo.

### 5.4. `cpc_UpdScr()`

- **Qué hace**: recorre el bitfield de celdas sucias y, para cada celda
  marcada, copia el gráfico del tile (desde `tiles`, indexado por el byte de
  la nametable) al superbuffer.
- **Uso**: `cpc.h:1551`, dentro de `cpc_UpdateNow`.

### 5.5. `cpc_ShowTouchedTiles()` / `cpc_ResetTouchedTiles()`

- `cpc_ShowTouchedTiles`: copia del superbuffer al área visible las celdas
  marcadas en el bitfield.
- `cpc_ResetTouchedTiles`: pone el bitfield a cero (96 bytes).
- **Uso**: `cpc.h:1648-1649`, al final de `cpc_UpdateNow`.

Con la variante fg (`PASARPORDETRAS`), `cpc_ShowTouchedTiles` consulta
`behindtilemasks` para decidir si una celda es "de primer plano" (y por
tanto el sprite debe quedar detrás) o normal.

### 5.6. `cpc_UpdTileTable(x, y)`

- **Qué hace**: marca una celda como sucia.
- **Parámetros**: `E` = x (columna), `D` = y (fila).
- **Uso**: `cpc.h:988-995` (los 4 tiles del metatile) y `cpc.h:1109-1111`
  (los 2 dígitos).

- Ojo: la casilla marcada es `(x, y)` en **coordenadas de pantalla**
  (0..31, 0..23), no en coordenadas de viewport.

### 5.7. `cpc_InvalidateRect(row, col, row2, col2)`

- **Qué hace**: marca un rectángulo de celdas como sucias.
- **Parámetros**: `B` = fila superior, `C` = columna izquierda,
  `D` = fila inferior, `E` = columna derecha (ambos inclusivos).
- **Uso**:
  - `draw_text` (`cpc.h:1175-1187`): marca las celdas del texto.
  - `invalidate_viewport` (`cpc.h:1058-1066`): marca todo el área de juego
    (se usa al cambiar de pantalla).

### 5.8. `cpc_TestKeyboard(line)`

- **Qué hace**: lee una línea del teclado (matriz del PSG) y devuelve el
  estado de las teclas de esa línea.
- **Parámetro**: `A` = número de línea.
- **Salida**: el byte de la línea, **invertido** (0 = tecla pulsada).
- **Uso**: `cpc.h:1199-1216`, dentro de `pad_read`:

```c
	#asm 
			ld  hl, cpc_KeysData + 12
			xor a
			ld  (_pad0), a
			ld  b, 1 				// Bit to switch
		pad_read_loop:
			push bc
			ld  d, (hl) 			// LSB: Column
			inc hl
			ld  a, (hl) 			// MSB: Line
			inc hl
			call cpc_TestKeyboard
			and d  					// Pressed?
			pop bc
			jr  z, pad_read_not_pressed
			...
```

El motor recorre la tabla de teclas (12 entradas de 2 bytes: máscara +
línea), y va montando `pad0` con los bits `sp_LEFT`, `sp_RIGHT`, `sp_UP`,
`sp_DOWN`, `sp_FIRE`, etc. (`cpc.h:210-217`).

### 5.9. `cpc_TestKey(index)`

- **Qué hace**: comprueba si una tecla de la tabla está pulsada.
- **Parámetro**: `L` (fastcall) = índice de la tecla (0 = la primera).
- **Salida**: 1 si está pulsada, 0 si no.
- **Uso**: `mainloop.h:381` (`cpc_TestKey (KEY_AUX1)`, la tecla M del
  scripting) y `mainloop.h:484` (teclas de depuración).

Los índices son los del `cpc_KeysData`: `KEY_LEFT` = 0, `KEY_RIGHT` = 1,
`KEY_UP` = 2, `KEY_DOWN` = 3, `KEY_BUTTON_A` = 4, `KEY_BUTTON_B` = 5,
`KEY_AUX3` = 6, `KEY_AUX4` = 7, `KEY_AUX1` = 8, `KEY_AUX2` = 9,
`KEY_ENTER` = 10, `KEY_ESC` = 11 (`cpc.h:197-208`).

### 5.10. `cpc_KeysData` (la tabla de teclas)

- Es una tabla de 12 palabras de 16 bits (más 10 bytes de "keymap" y 2 de
  reservas al principio). Cada palabra es `(línea << 8) | máscara`.
- **Está en `cpc_KeysData + 12`** (el motor lo tiene hardcodeado en
  `cpc.h:1199` y `cpc.h:1775`).
- El motor la **reescribe** en `select_controls` (`cpc.h:1752-1779`),
  copiando 24 bytes desde `def_keys` (teclado) o `def_keys_joy` (joystick):

```c
		.copy_keys_to_extern
			ld  de, cpc_KeysData + 12
			ld  bc, 24
			ldir
```

  Así el jugador elige si usa teclado o joystick, sin tocar la librería.

### 5.11. `cpc_MakeM1RotationLUTs()`

- **Qué hace**: rellena las tres tablas de rotación de bytes de modo 1
  (`M1LUTR1` = `$D700`, `M1LUTR2` = `$DE00`, `M1LUTR3` = `$E700`), que
  sirven para dibujar sprites al pixel desplazados un byte.
- **Uso**: `cpc.h:424-426`, en `system_init`, y sólo si `MODE_1` y no
  `NO_MASKS`:

```c
	#if defined MODE_1 && !defined NO_MASKS
		cpc_MakeM1RotationLUTs ();
	#endif
```

---

## 6. Los sprites en el CPC

### 6.1. La estructura `SPR`

El motor **no** usa los sprites de la CPCRSLIB original: define su propia
estructura de 16 bytes (`cpc.h:221-233`):

```c
typedef struct sprite {
	unsigned int sp0;			// 0   puntero al gráfico del sprite
	unsigned int sp1; 			// 2   puntero al gráfico de máscara
	unsigned int coord0;		// 4   (no lo usa el motor)
	signed char cox, coy;		// 6 7 offset de la "cara" del sprite
	unsigned char cx, cy; 		// 8 9 posición actual
	unsigned char ox, oy;		// 10 11 posición anterior
	void *invfunc;				// 12 rutina que marca las celdas
	void *updfunc;				// 14 rutina que dibuja el sprite
} SPR;
```

- `cx`, `cy`: la posición **actual** del sprite. Las unidades dependen de la
  rutina que se use (ver §6.3).
- `ox`, `oy`: la posición **anterior**. La rutina de invalidación marca
  las celdas de las dos posiciones, para que el sprite no deje rastro.
- `cox`, `coy`: el desplazamiento de la "cara" del sprite dentro del
  spriteset (los arrays `sm_cox` / `sm_coy` que genera `mkts_om.exe`).
- `invfunc` / `updfunc`: los punteros a las rutinas de la librería.

El array de sprites es `sp_sw[SW_SPRITES_ALL]` (`cpc.h:232`), en
`BASE_SPRITES` (`$E600`), y los índices son (`cpc.h:97-104`):

- `SP_PLAYER` (0): el jugador,
- `SP_ENEMS_BASE` (1): los enemigos,
- `SP_BULLETS_BASE` (1 + `MAX_ENEMS`): las balas,
- `SP_SWORD_BASE`, `SP_COCOS_BASE`, `SP_CUSTOM_BASE`: los siguientes huecos.

`SW_SPRITES_ALL` = 1 (jugador) + `MAX_ENEMS` + `MAX_BULLETS` + (espada) +
`MAX_CUSTOM_SW_SPRITES`. Si añades sprites propios, crece el array (y con
él, la memoria en `BASE_SPRITES`).

### 6.2. Quién pone qué

En `system_init` (`cpc.h:491-595`):

- **El jugador**: `cox`/`coy` de `sm_cox[0]`/`sm_coy[0]`, `invfunc` de
  `sm_invfunc[0]`, `updfunc` de `sm_updfunc[0]` (todo del
  `spriteset_mappings.h` que genera `mkts_om.exe`), y el gráfico apunta a
  `spr_empty` (se cambia cada frame).
- **Los enemigos**: rutinas fijas, `cpc_PutSpTileMap8x16Px` (invalidar) y
  `cpc_PutTrSp8x16TileMapPx` (dibujar) (`cpc.h:527-531`).
- **Las balas**: `cpc_PutSPTileMap2Bx8` (invalidar) y `cpc_PutTrSp2Bx8TileMap`
  o `cpc_PutTrSp2Bx8TileMapG` (dibujar), según `NO_MASKS`
  (`cpc.h:553-561`).
- Al final, **todos** los sprites se colocan fuera de la pantalla
  (`cpc.h:579-595`) poniéndoles `ox`/`oy` fuera del viewport.

Cada frame, `render_all_sprites` (`cpc.h:1368-1469`):

- enemies: `render_this_enemy` (`cpc.h:1328-1366`) pone `cx`, `cy` y el
  puntero al gráfico (`sp0` = `en_an_next_frame[enit]`);
- jugador: `cx`/`cy` desde `gpx`/`gpy`, y `sp0` = `player.next_frame` (o
  `spr_empty` si está parpadeando);
- balas: `cx`/`cy` desde `bullets_x[]`/`bullets_y[]` y `sp0` =
  `spr_bullet`.

### 6.3. Las unidades de las coordenadas

Aquí está la mayor trampa del backend CPC. **Cada familia de rutinas espera
unidades distintas** en `cx`, y el motor lo compensa con divisiones
distintas:

- Las rutinas `*2Bx8*` (balas) trabajan con `cx` **en bytes**: su comentario
  dice "X en bytes! (0-63)", y el motor divide la posición entre 4
  (`cpc.h:1457`).
- Las rutinas `*Px*` (modo 0) trabajan con `cx` en unidades donde 4 = un
  tile ("X está en pixels. Al estar alineado a rejilla si & 3 == 0"): el
  motor divide la posición entre 2 en `MODE_0` (`cpc.h:1340-1342` y
  `1419-1421`) y no divide nada en `MODE_1`.
- Las rutinas `*M1*` (modo 1) trabajan en píxeles de hardware.

Traducción práctica: **si cambias de rutina de sprite, tienes que revisar la
división de la coordenada X**. Cada módulo de la librería lleva en su
cabecera un comentario con la convención que espera; míralo antes de
enlazar uno nuevo.

### 6.4. Los datos del sprite

Los gráficos vienen de `sprites.bin`, que produce
`mkts_om.exe ... mode=sprites` (`comp.bat:37`). El `spriteset_mappings.h`
generado define punteros a cada "cara":

```c
#define SPRITE_00 (_sprites + 0x0000)
#define SPRITE_01 (_sprites + 0x0040)
...
```

El paso entre caras es el tamaño del sprite en bytes (por ejemplo 0x40 = 64
bytes para un 16x16 en modo 1).

El contenido de un sprite es un bitmap de N bytes por fila (N = el "metasize"
del sprite) y 16 filas para los sprites de 16 de alto. La rutina de dibujo
hace, byte a byte:

```
destino = (fondo & LUT[byte_del_sprite]) | byte_del_sprite
```

donde `LUT` es la **LUT de píxeles transparentes** que vive en `$FE00`
(`BASE_LUT`). La genera el propio motor:

- `src/dev/system/cpc_TrPixLutM1.asm` (modo 1) y `cpc_TrPixLutM0.asm`
  (modo 0),
- `comp.bat:76` la ensambla con `pasmo` y la comprime con `zx0`,
- `cpc.h:417-421` la descomprime in situ al arrancar:

```c
	#asm
			ld  hl, _trpixlutc
			ld  de, BASE_LUT
			call depack
	#endasm
```

### 6.5. Las dos familias: con LUT y "G" (OR directo)

Hay dos formas de mezclar el sprite con el fondo:

- Las rutinas **sin "G"** en el nombre (`cpc_PutTrSp2Bx8TileMap`,
  `cpc_PutTrSp8x16TileMapPx`, ...) usan la LUT de `$FE00`:
  `fondo = (fondo & LUT[byte_del_sprite]) | byte_del_sprite`. La LUT dice,
  para cada byte posible del sprite, qué bits del fondo se conservan (LUT[0]
  = `$FF`, se conserva todo; LUT[255] = `$00`, no se conserva nada), así que
  el sprite puede ser opaco o transparente según sus propios datos.
- Las rutinas **"G"** (`cpc_PutTrSp2Bx8TileMapG`, ...) hacen un simple OR:
  `fondo = fondo | byte_del_sprite`. No necesitan LUT, pero no pueden
  "borrar" nada del fondo: los bits a 1 del sprite se mezclan con los del
  fondo.

El motor elige la de las balas así (`cpc.h:558-562`):

```c
			#ifdef NO_MASKS
					ld  hl, cpc_PutTrSp2Bx8TileMap 	// sm_updfunc [0]
			#else
					ld  hl, cpc_PutTrSp2Bx8TileMapG 	// sm_updfunc [0]
			#endif
```

O sea: con `NO_MASKS` definido usa la de la LUT; sin él (por defecto), la
"G" (OR directo).

### 6.6. Los "garbled bits" del modo 0

`tilemap_conf.asm:27-54` exporta dos máscaras, `mascara1` y `mascara2`
(ambas `$00` = "no garbled"), con la idea de marcar los tiles cuyo byte tiene
los dos tipos de píxel intercalados (ABABABAB), que es lo que pasa en modo 0
y complica el enmascarado. **La versión actual de la biblioteca no las usa**
(ningún módulo las referencia): son una reserva para el caso "garbled" que
se puede activar si hace falta.

---

## 7. La matriz de rutinas de sprite

La librería incluye 35 rutinas de sprite, elegidas por **tamaño y formato de
píxel**. Los nombres codifican:

- `cpc_PutSpTileMap...`: la rutina de **invalidación** (marca las celdas
  que ocupa el sprite).
- `cpc_PutTrSp...TileMap...`: la rutina de **dibujo**.

Tamaños: 4x8, 8x8, 8x16, 8x24, 16x16, 16x24, 12x24, 2Bx8, 4Bx16, 4Bx24.
Formatos: `Px` (al pixel en modo 0), `PxM1` (al pixel en modo 1), `GPx`
(al pixel con máscara), `G` (OR directo, sin LUT de transparencia), `LUT`
(con las LUTs de rotación), `CA`.

Las que usa el motor de serie:

- Jugador: las que diga `spriteset_mappings.h` (generado por `mkts_om.exe`).
  En el esqueleto, `cpc_PutSpTileMap16x16PxM1` y
  `cpc_PutTrSp16x16TileMapPxM1LUT`.
- Enemigos: `cpc_PutSpTileMap8x16Px` + `cpc_PutTrSp8x16TileMapPx`.
- Balas: `cpc_PutSPTileMap2Bx8` + `cpc_PutTrSp2Bx8TileMap` (o `...G`).

> Si tu juego usa un tamaño o un formato de píxel que no esté en la
> matriz, hay que recuperar el módulo correspondiente del historial de git
> (ver la nota final de `lib/cpcrslib-mt/README.md`).

---

## 8. Recetas: cómo escribir tu propio código

### 8.1. Añadir un sprite propio

1. Añade el sprite al PNG (`gfx/cpc/sprites.png`) y regenera `sprites.bin`
   con `mkts_om.exe` (`comp.bat:37`).
2. Si es una "cara" nueva del spriteset, `mkts_om.exe` actualizará
   `spriteset_mappings.h` con el puntero (`SPRITE_xx`) y las rutinas
   (`sm_invfunc` / `sm_updfunc`).
3. Crea el sprite en `system_init`: copia el patrón de los enemigos
   (`cpc.h:520-537`) y pon `sp0` al puntero de tu gráfico.
4. En `render_all_sprites`, pon `cx`, `cy` y `sp0` cada frame.

Recuerda: cada sprite necesita sus propias rutinas de invalidación y
dibujo, y **sus unidades de coordenadas** (§6.3).

### 8.2. Escribir en pantalla

- **Cambiar un tile**: escribe el número de tile en la nametable (con
  `__tile_address`) y marca la celda con `cpc_UpdTileTable (x, y)`.
- **Una zona**: escribe los tiles y marca el rectángulo con
  `cpc_InvalidateRect`.
- **Texto**: `draw_text` ya lo hace (escribe en la nametable e invalida).

Nunca escribas en `$C000` (la pantalla visible) esperando que se queda: el
próximo `cpc_UpdScr` + `cpc_ShowTouchedTiles` lo pisa. Escribe en la
nametable.

### 8.3. Teclas

- `cpc_TestKey (KEY_AUX1)` para una tecla concreta.
- `pad0` / `pad1` / `pad_this_frame` para el juego (los rellena `pad_read`
  a partir de `cpc_KeysData`).

### 8.4. Forzar un repintado completo

`cpc_ShowTileMap (0)` recompone la pantalla entera desde la nametable. Y si
lo que quieres es repintar la zona de juego, `invalidate_viewport()`
(`cpc.h:1058`) marca todo el viewport.

---

## 9. Trampas

- **La librería no es autónoma**: si cambias las direcciones de
  `tilemap_conf.asm`, o las de `BASE_LUT`/`BASE_SPRITES`, dejas de ser
  compatible con la librería compilada.
- **El superbuffer es el protagonista**: cualquier cosa que se dibuje
  "a mano" en `$C000` se pierde; hay que ir por la nametable.
- **La ORDEN de las operaciones en `cpc_UpdateNow` es importante**: primero
  se invalidan (marcan celdas), luego se recarga el fondo (`cpc_UpdScr`),
  luego se dibujan los sprites, y al final se vuelca a la pantalla. Si
  llamas a `cpc_UpdScr` después de dibujar los sprites, los borras.
- **Unidades de coordenadas**: cada familia de rutinas espera unidades
  distintas (§6.3).
- **Modo 0 vs modo 1**: no es sólo el número de colores. En modo 0 el
  motor divide la X entre 2, la LUT de transparencia es otra, y las rutinas
  de sprite son otras (`Px` en vez de `PxM1`).
- **La tabla de teclas se reescribe**: `select_controls` copia 24 bytes
  sobre `cpc_KeysData + 12`. Si tocas `cpc_KeysData` en otro sitio, cuidado.
- **La variante fg** (`cpcrslib_fg_mt.lib`) necesita la tabla
  `behindtilemasks` exportada por el juego; si falta, el enlazado falla.
- **`PASARPORDETRAS` es de compilación**: no se puede activar en runtime.
- **Los sprites no tienen "capas"**: el orden de dibujo es el del array
  `sp_sw`, y `cpc_UpdateNow` los recorre **del último al primero**, así que
  el índice más bajo (el jugador) se dibuja el último y queda delante.
- **La ISR es del motor, no de la librería**: el contador de frames
  (`isr_c2`) y el reproductor de música WYZ están en la ISR del motor
  (`cpc.h:303-408`); la librería no toca el sistema de interrupciones.

---

## 10. Índice rápido

Núcleo:

- `cpc_SetMode` — cambia el modo de vídeo. `cpc.h:439`, `441`.
- `cpc_Border` — color del borde. `cpc.h:412`.
- `cpc_ShowTileMap` — recompone y/o vuelca la pantalla. `mainloop.h:39`;
  `pantallas.h:96`.
- `cpc_UpdScr` — recarga en el superbuffer las celdas sucias.
  `cpc.h:1551`.
- `cpc_ShowTouchedTiles` — vuelca a la pantalla las celdas sucias.
  `cpc.h:1648`.
- `cpc_ResetTouchedTiles` — limpia el bitfield de celdas sucias.
  `cpc.h:1649`.
- `cpc_UpdTileTable` — marca una celda como sucia. `cpc.h:988`, `993`;
  `1109`.
- `cpc_InvalidateRect` — marca un rectángulo como sucio. `cpc.h:1064`,
  `1187`, `1324`.
- `cpc_TestKeyboard` — lee una línea del teclado. `cpc.h:1209`.
- `cpc_TestKey` — comprueba una tecla de la tabla. `mainloop.h:381`, `484`.
- `cpc_KeysData` — la tabla de teclas (en `cpc_KeysData + 12`).
  `cpc.h:1199`, `1775`.
- `cpc_MakeM1RotationLUTs` — tablas de rotación de modo 1. `cpc.h:425`.

Sprites:

- `cpc_PutSpTileMap8x16Px` — invalidar enemigos. `cpc.h:527`.
- `cpc_PutTrSp8x16TileMapPx` — dibujar enemigos. `cpc.h:531`.
- `cpc_PutSPTileMap2Bx8` — invalidar balas. `cpc.h:553`.
- `cpc_PutTrSp2Bx8TileMap` / `...TileMapG` — dibujar balas. `cpc.h:559`,
  `561`.
- `sm_invfunc` / `sm_updfunc` — rutinas del jugador, elegidas por
  `mkts_om.exe`. `cpc.h:502`, `506`.

### Ficheros relacionados

- `lib/cpcrslib-mt/` — la librería (fuente en ensamblador + cabecera).
- `lib/cpcrslib-mt/README.md` — notas de la poda de la librería.
- `lib/cpcrslib-mt/cosas.md` — notas de implementación (píxeles de modo 0,
  LUTs de rotación).
- `lib/cpcrslib-mt/CPCconfig.def` — configuración de compilación.
- `src/dev/cpc.h` — el backend CPC: aquí está todo el uso de la librería.
- `src/dev/system/tilemap_conf.asm` — el contrato con la librería.
- `src/dev/system/cpc_TrPixLutM0.asm`, `cpc_TrPixLutM1.asm` — la LUT de
  píxeles transparentes.
- `src/dev/cpc/spriteset_mappings.h` — punteros a las caras del spriteset y
  sus rutinas (generado).
- `src/utils/mkts_om.bas` — el generador de `sprites.bin` y de
  `spriteset_mappings.h`.
- `misc/cpc-cpcrslib-demo/` — ejemplo standalone que usa cpcrslib-mt sin el
  motor.
