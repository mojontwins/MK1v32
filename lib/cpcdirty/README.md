# cpcdirty

Render de celdas sucias para Amstrad CPC, sin superbuffer, como librería de
z88dk 1.10 (`cpcdirty.lib` + `cpcdirty.h`). Es el backend `CPC_DIRTY` de MK1
v3.2, pero no depende del motor: cualquier programa puede usarla. Tampoco
necesita `cpcrslib-mt`: trae las rutinas de teclado y de modo que hacían falta
(`cpc_KeysData`, `cpc_TestKeyboard`, `cpc_TestKey`, `cpc_SetMode`, con los
mismos nombres y la misma tabla de teclas).

## Los tres modos

| Modo | `cpcdirty.h` | MK1 | Colores | Sprites |
|---|---|---|---|---|
| Modo 1 | `CD_MODE_M1` | `-DMODE_1` | 4 pens, compartidos | 16x16 con máscara (pen 0 transparente), a pixel |
| Modo 0 planar | `CD_MODE_P0` | `-DMODE_0` | paleta partida: pen = fondo × 4 + sprite | ORed sin máscara, geometría por slot (8-32 líneas, 8 o 16 píxeles), a pixel |
| Modo 0 nibble | `CD_MODE_N0` | `-DMODE_0` + `MODE_0_NIBBLE` | 16 pens, compartidos | 8x16 (4 bytes x 16 líneas) con máscara (pen `CD_TPEN` transparente), a pixel |

El enlazador solo mete los módulos del modo que usas: cada modo tiene su
`cd_init_*`, `cd_sprites_*`, `cd_pal_*` y `cd_put_char_*`, y el volcado
(`cd_flush`), que es común, llama al compositor de filas del modo a través de
un salto en RAM que pone `cd_init_*` (`CD_OR_JP`). Así ninguna parte común
nombra a un modo concreto.

En el modo nibble el pen transparente lo es solo en los sprites: los tiles lo
pueden usar como un color más (la máscara sale de una tabla, `CD_MLUT`).

## Cómo funciona

Celdas de 2 bytes x 8 líneas (un patrón), viewport de 30 x 20 celdas, pantalla
en C000 de 64 bytes por línea (la de MK1, CRTC R1 = 32) o de 80 (la normal).

* **Marcado.** Los tiles y los sprites marcan las celdas que tocan. Una celda
  que no estaba sucia se sella con la generación actual (`CD_DIRTY`) y su
  columna se apunta en el cubo de su fila (`CD_BUCKET`). Cada sprite marca
  dónde se dibujó la última vez y dónde está ahora, y se apunta en las filas
  que cruza (`CD_ROWSN`/`CD_ROWSPR`, 8 como mucho por fila).
* **Volcado** (`cd_flush`), fila a fila de arriba abajo, solo las sucias:
  (a) el patrón de cada celda sucia a `CD_ROWBUF` (el patrón 0 se rellena con
  `CD_BLANK_PEN`), (b) cada sprite que cruza la fila (OR en modo 0 planar,
  AND + OR en los otros dos), (c) las celdas a pantalla, cuando el haz ya ha
  pintado esa fila en este frame (`CD_ROWINT`). Nunca se ve un fondo sin su
  sprite ni una fila a medio cambiar.
* **Cachés de sprite.** Cada sprite tiene una caché en orden de columnas que
  se rehace solo cuando cambia de frame o de fase (desplazamiento de medio
  byte, o de 0-3 píxeles en modo 1).

## Uso

Antes de `#include <cpcdirty.h>` (una sola vez: el include emite las
constantes con las que se enlaza la librería) hay que definir:

* `CD_MODE_M1`, `CD_MODE_P0` o `CD_MODE_N0`.
* `CD_NSPR`: sprites de la lista (1-8 en los modos con máscara, 1-16 en P0).
* `CD_SPR_LIST`: dirección de la lista: `CD_NSPR` registros de 16 bytes, +0
  frame (word), +8 x, +9 y (como `sp_sw []` de MK1). x en píxeles del modo,
  y en líneas, los dos respecto a la pantalla.
* `CD_VPX`, `CD_VPY`: esquina del viewport, en caracteres de 2 bytes y filas.
* Opcionales: `CD_SCRW` (bytes por línea de pantalla: 64, por defecto, u 80),
  `CD_SCRH` (líneas de pantalla para `cd_screen_copy`: 192, o 200 con 80),
  `CD_BLANK_PEN` (M1, N0: el pen del patrón 0, por defecto 0),
  `CD_TPEN` (N0: pen transparente de los sprites, por defecto 0),
  `CD_CACHE_END` (P0 con tabla de geometría).

La ISR del juego tiene que incrementar `cd_intc` en cada interrupción:

```
    LIB cd_intc
    ...
    ld  hl, cd_intc
    inc (hl)
```

En cada frame:

```
    call cd_sprites_n0   ; lee la lista, rehace cachés, marca
    ; ... esperar a la 3ª interrupción del frame (justo después de que el
    ; haz deje la fila 0 del viewport)
    call cd_flush
```

### Rutinas

| Rutina | Entrada | Qué hace |
|---|---|---|
| `cd_init_m1`, `cd_init_n0` | HL = frame "oculto" | Tablas, mapa vacío. Los patrones tienen que estar ya en `CD_PAT` |
| `cd_init_p0` | HL = frame "oculto", DE = tabla de geometría (h, w por slot) o 0 | Igual, con cachés a medida |
| `cd_sprites_m1/p0/n0` | | La pasada de sprites del frame |
| `cd_flush` | | El volcado |
| `cd_invalidate` | | Todo el viewport sucio |
| `cd_put_tile` | B columna, C fila (celdas del viewport), DE = 4 patrones | Un tile de 2x2 celdas |
| `cd_set_cell` | B columna, C fila, A patrón | Una celda |
| `cd_mark_cell`, `cd_mark_rect` | A columna, C fila / IX+4..7 = c0 c1 r0 r1 | Marcar sin cambiar el mapa |
| `cd_put_char_raw` | B x, C y (caracteres de pantalla), A patrón | Un patrón directo a pantalla (M1, N0) |
| `cd_put_char_p0` | B x, C y, A patrón | Igual, con su color 3 en el pen 3 de sprite (P0) |
| `cd_screen_copy` | HL = imagen lineal de `CD_SCRW` x `CD_SCRH` | A pantalla (p. ej. un superbuffer de `mkts_om` descomprimido en 9000). Una pantalla de 80 bytes en formato `.scr` (`mkts_om mode=scr`) se descomprime directamente en C000 |
| `cd_pal_m1`, `cd_pal_n0` | HL = tintas hardware (4 / 16) | Paleta; el borde, la del pen 0 |
| `cd_pal_p0` | HL = tintas de fondo, DE = de sprite (16 cada una) | Paleta partida |

Desde C están declaradas en `cpcdirty.h` las que se pueden llamar así
(`__FASTCALL__` con HL o sin parámetros); el resto, desde `#asm` con `LIB`.

## Memoria

Los 12K de 9000-BFFF:

```
9000 CD_PAT      256 patrones x 16 bytes
A000 CD_ROWBUF   la fila que se compone (30 celdas, cada 32 bytes)
A400 CD_BUCKET   20 x 32: columnas sucias de cada fila
A700 CD_ROWD     cuántas por fila
A800 CD_ROWINT   interrupciones (desde la 3ª) tras las que el haz ha pintado la fila
A900 CD_ROWSN    sprites que cruzan cada fila
AA00 CD_ROWSPR   20 x 8: cuáles
AB00 CD_ROWTAB   20 x 4: fila del mapa, dirección de pantalla
AB80             variables
AC00 CD_TMAP     30 x 20 patrones (el mapa del viewport)
AE60             más variables (claves de las cachés, frame oculto...)
```

y luego, según el modo:

| | Cachés | Registros (`CD_SPRT`) | `CD_MLUT` | `CD_DIRTY` | Libre desde (`CD_FREE`) |
|---|---|---|---|---|---|
| M1, N0 | B000, 8 x 160 | B500 | B600 | B700 | B958 |
| P0 | B000, 8 x 96 | B300 | — | B400 | B658 |
| P0 con `CD_CACHE_END` | B000 - `CD_CACHE_END` | `CD_CACHE_END` | — | +100 | +358 |

`cd_intc` está fuera de esa zona, en el código de la librería: las pantallas
se descomprimen en 9000-BFFF mientras la ISR sigue contando. Después de
descomprimir cualquier cosa ahí hay que volver a llamar a `cd_init_*`.

## Compilar e instalar

`make.bat` / `make.sh` hacen `cpcdirty.lib` con el `z80asm` del z88dk de
MK1 (`z80asm -x @cpcdirty.lst`); con `install` copian también la librería y la
cabecera al z88dk (en Windows, a `c:\z88dk10-orig`, como `../updatelib.bat`,
que también la instala). Se enlaza con `-lcpcdirty`, en lugar de
`-lcpcrslib_mt`.

## Pruebas

`test/test.sh` compila `test/test_cd.c` en los tres modos, lo ejecuta en
`cpcheadless` y, tras cada uno de sus 16 pasos (8 sprites en sitios
pseudoaleatorios, ocultos, en los bordes, a byte par e impar; celdas y tiles
cambiados), compara la pantalla con un modelo hecho píxel a píxel
(`test/check.py`): tiene que coincidir byte a byte. Se prueba con pantalla de 64
bytes (viewport en 1, 2) y de 80 (en 5, 1). `test.bat` solo compila.
