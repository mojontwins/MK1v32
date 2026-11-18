# Nanako in Classic Japanese Monster Castle (MK1 v3.2)

Port a MK1 v3.2 del juego de 2006 (ZX, CEZ GS) / 2009 (CPC, Mojon Twins), para ZX Spectrum 48K/128K (una sola cinta) y Amstrad CPC 464. La referencia es la versión CPC (`orig/dev/ncjmccpc.c`).

## Compilar

- ZX: `dev/comp.bat` (`comp.sh` en Linux) → `nanako.tap` (48K y 128K).
- CPC: `dev/comp_cpc.bat` (`comp_cpc.sh`) → `nanako_cpc.sna`, `.cdt` (`RUN""`), `.dsk` (`RUN"NANAKO`).
- Generadores de una vez (Python, ya ejecutados; solo si cambian los originales):
  - `tools/nanako_conv.py`: niveles, contraseñas, enemigos y gráficos provisionales.
  - `tools/nanako_music.py`: canciones y efectos AY (CPC y RAM1 del 128K).
  - `tools/nanako_beeper.py`: la música del título del ZX original (`dev/music.h`).

Necesita `mk1v32.patch` aplicado en el repo (desde la raíz: `git apply exp\nanako\mk1v32.patch`): `PLAYER_JUMP_FIRE_ONLY`, `SPRITES_CLIP_*` y los ficheros del 48K/128K que faltan en `src` (`sfx_player.h`, `system/zxloader.asm`, `system/loader128.bas`, `utils/sfxcnv.bas`).

## Cómo está hecho

Casi todo es vanilla; lo propio está en `dev/custom.h`, `dev/custom_flick.h` y `dev/pantallas.h`.

- **Mapa**: renderer propio (`ENABLE_CUSTOM_MAP_RENDERER`) con los datos originales (`dev/levels.h`, 21 bytes por nivel). El área de juego es de 12x8 tiles en las columnas 1-12 y filas 0-7. Las columnas 0, 13 y 14 y las filas 8 y 9 son sólidas (tile 12, no se pintan: las torres y el suelo son del marco). Arriba está abierto. Se pinta de abajo arriba, como la cortinilla del original.
- **Niveles**: pantallas 1-25 (`MAP_W 1`, `MAP_H 26`; el flick vanilla hacia arriba pide `n_pant >= MAP_W`). Saltar por arriba es la salida: `flick_up` pasa de nivel (caja del nivel con la contraseña y el jingle) o, tras el último, al final.
- **Movimiento**: vanilla con poca inercia (`PLAYER_MAX_VX` 2 px/frame, se alcanza y se para en 2 frames). Salto de unos 20 px como máximo: sube un tile pero nunca dos. `PLAYER_CUSTOM_HENG` aplica la mitad de aceleración y de fricción en el aire. Salto solo con FIRE (`PLAYER_JUMP_FIRE_ONLY`).
- **Cajas** (tile 5, `PLAYER_MOVE_OVERRIDE`): ABAJO coge la caja bajo los pies (la de debajo del centro, si no la otra columna que pisa); ARRIBA la deja donde está Nanako y la sube un tile. Van a tiles enteros (el original permitía medio tile en horizontal). Solo con `gpy` alineado a 16: encima del karakasa no se puede dejar caja.
- **Karakasa**: dos enemigos de 16x16. La cabeza (slot 0) es una plataforma móvil vanilla (tipo 4): Nanako se sube y la lleva. Su `my` rebota entre +1 y -1 con `y1 = y2` para que el código vanilla la compruebe siempre aunque esté quieta. El cuerpo (slot 1, tipo 5) copia la posición y mata. El paseo aleatorio y la colocación son los del original. Nota: los intentos al azar del original nunca aciertan en una plataforma (miran el mismo tile dos veces), así que casi siempre sale en el suelo; se ha dejado igual.
- **Puf** (slot 2, tipo 6, no mata): al coger o dejar caja y al morir.
- **Muerte**: puf, se pierde una vida y se repite el nivel (al recargar la pantalla vuelven las cajas). R reintenta (cuesta una vida); H (ZX) / ESC (CPC) abandona. 5 vidas: game over al morir con 0, como el original.
- **Recorte de sprites (ZX)**: el marco se descomprime directamente en pantalla, así que splib2 no tiene como fondo las casillas que rodean el área de juego, y los sprites de 16x16 las borraban al pasar (la fila de debajo y la columna de la derecha). Con `SPRITES_CLIP_*` (opción nueva del motor, en `speccy.h`) se recortan al área de juego. En CPC no pasa: el marco se descomprime en el superbuffer, que es el fondo de los sprites.
- **Marcador**: 5 iconos de vida (caracteres `[` vacío y `]` lleno de la fuente).
- **Título**: 1/2/3 controles, P contraseña (O/P cursor, Q/A letra, ESPACIO acepta; la del CPC original).

## Sonido

- **ZX 48K**: efectos de beeper en el tiempo libre (`sfx/sfx.txt`, `SFX_IDLE_PLAYER`). Título: la música del ZX original, con su propio reproductor de dos canales sacado del snapshot (`tools/nanako_beeper.py`).
- **ZX 128K**: las tres canciones del original (título, juego, jingle) y sus 4 efectos en RAM1 (`ogt/ram1`), como en Uwol.
- **CPC**: las mismas, comprimidas con zx0. Canales B y C intercambiados: el original tocaba la batería y los efectos por C y MK1 la toca por B (`WYZ_FX_CHANNEL 1`).
- Los números de efecto son los de `sfx/sfx.txt` (10, lo que admite el reproductor del CPC): 2 muerte, 3 salto, 7 coger, 8 dejar, más game over, final y comienzo de nivel (solo beeper).

| Canción | Original | MK1 |
|---|---|---|
| title | 790 | 874 |
| ingame | 1463 | 1806 |
| jingle | 55 | 74 |

## Gráficos (provisionales)

Mapeados como recursos estándar de MK1 para rehacerlos:

| Recurso | Contenido |
|---|---|
| `work.png` tiles 0-1 | fondo, desconchón |
| 2-4 | objetos fijos (el original elige uno al azar al pintar) |
| 5 | caja |
| 6-11 | plataformas: estilos 0, 1, 2, 3a, 3b, 4 (`nivel % 5`; el 3 alterna 3a/3b) |
| 12 | negro (marco sólido y cortinilla) |
| 13-25 | caja del nivel (6x4 tiles, `dev/levelbox.h`) |
| `sprites.png` 0-7 | Nanako (MK1: derecha 0-3, izquierda 4-7) |
| 8-11 / 12-15 | karakasa: cabeza (sus 8 filas de arriba) / cuerpo (las 16 de abajo); izquierda 0, 1, derecha 0, 1 |
| 16-18 | puf |
| `font.png` `[` `]` | vida vacía / llena |

- **CPC**: los gráficos originales del CPC, en el formato de MK1. Tinta 0 transparente en los sprites (en el original era la 9), `BLACK_PEN 9`. El marco del original recortado a 128 píxeles de modo 0: área de juego en (4, 3) caracteres, `VIEWPORT_Y 3`.
- **ZX**: todo lo del ZX original salvo el puf, que es el del CPC. Sale del snapshot (`orig/ncjmc-zx.z80`) y de sus pantallas (`orig/zx/*.scr`):
  - el marco y la fuente;
  - el fondo, los tres objetos fijos (calavera, jarrón, cajón azul) y la caja;
  - los cinco estilos de plataforma. El del nivel 4 mide tres caracteres, así que en ZX usa tres tiles (9, 10 y 26); el CPC alterna dos;
  - Nanako y el karakasa.

  En el ZX el área de juego está en (4, 4): `VIEWPORT_Y 4`. Los tiles de un solo color llevan tinta amarilla (`ts2bin ... 006`), así que los sprites salen amarillos como Nanako en el original.
- **`gfx/zx_original`** (`tools/nanako_zxgfx.py`): los gráficos del ZX original para rehacerlos.
  - El original es BASIC compilado y todo se imprime con caracteres: el juego de caracteres del juego (códigos 32-80 en EB34), los UDG A-U (cajas, karakasa, icono de vida, plataforma del nivel 5), la fuente, y los tiles y sprites montados con los colores que se ven en el juego. Cada uno en tamaño real y a 4x con los códigos.
  - Las plataformas de los niveles 2-5 se han visto entrando con las contraseñas del ZX (`orig/zx/map-pavero-2010.png`).
  - Sin identificar: los caracteres 55-66 y los UDG A-H (¿el puf o la muerte?).
- El CPC usa sus propios gráficos (arriba); en ZX no se usa nada de CEZ Silver.
- No se han convertido la pantalla de carga ni la del final: son de solo texto.

## Probado

En los emuladores headless:

- se pintan los 25 niveles;
- coger y dejar caja, subirse al karakasa y que te lleve, la muerte y el game over, pasar de nivel;
- la contraseña;
- la música y los efectos del 128K y del CPC.

## Pendiente

- Jugar los 25 niveles con el movimiento nuevo: puede que alguno sea más fácil; ninguno debería quedar imposible.
- Los gráficos definitivos.
