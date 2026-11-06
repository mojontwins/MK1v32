# Yun ZX y CPC, primera carga (fases 1-3)

Conversión de las tres primeras fases de Yun (NES v5) a ZX Spectrum 48K con MK1 v3.2 a
15x10, en una sola carga. Probada en `zxheadless`, no en hardware real.

- **Fase 1:** mapa 4 de NES (3x4 pantallas), 20 enemigos que matar.
- **Fase 2:** mapa 0 (4x6), 40 enemigos, 2 cerrojos, fantys y monococos.
- **Fase 3:** mapa 1 (4x6), 48 enemigos, 1 cerrojo, fantys.

## Recursos

Todo se convierte con las herramientas de `utils` (las mismas que el resto de juegos de
MK1); no hay ningún script de Python en la compilación en Windows. Los gráficos de
ahora son placeholders sacados de los de NES con `tools/nes2png.py`, que se ejecutó una
sola vez y no forma parte de la compilación.

| Fichero | Qué es | Lo convierte |
|---|---|---|
| `gfx/speccy/font.png` | 18 caracteres: blanco, 0-9, A E G M O R V | `mkts_om` (`tiles.spt`) |
| `gfx/speccy/ts0-2.png` | 16 tiles de cada fase | `mkts_om` (`tiles.spt`) |
| `gfx/speccy/tsx.png` | tiles 16-18 (corazón, estrella, llave), comunes | `mkts_om` (`tiles.spt`) |
| `gfx/speccy/sprites.png` | 22 celdas fijas (abajo) | `sprcnv2 ... 22 nomask` |
| `gfx/speccy/sprites0-2.png` | 12 celdas de enemigos de cada fase | `sprcnv2 ... 12 nomask bin` |
| `gfx/speccy/title,marco,ending,loading.png` | pantallas | `png2scr` |
| `map/level0-2.map` | mapas, 4x6 pantallas (las que sobran, macizas) | `mapcnv ... packed bin maxlocks=2` |
| `enems/level0-2.ene` | enemigos y hotspots (ponedor, 3 bytes) | `ene2h ... bin=` |

En CPC, lo mismo en `gfx/cpc`, en modo 0 a su resolución (un tile de 16x16 son 8x16
píxeles en el PNG), más las paletas:

- `pal_cnvtiles0-2.png`: las 4 tintas de fondo de cada fase en los pens 0, 4, 8 y 12.
  Con ellas se convierten los tiles, y de ellas salen las tintas de la fase
  (`mkts_om mode=pals`).
- `pal_cnvspr.png`: las 3 tintas de sprite en los pens 1-3, para sprites y hotspots.
- `pal.png`: los 16 pens del título, el marco y el final (`my_inks`). Los pens 1-3 tienen
  que ser los de los sprites, porque el marco sigue en pantalla durante la partida.

**Celdas fijas** (`sprites.png`): 0-7 Yun, 8-9 burbuja, 10-11 plataforma, 12-13 fanty,
14-15 fanty atrapado, 16-19 monococo (arbusto, asomando, fuera, atrapado), 20 coco,
21 explosión (en CPC, `spr_boom`).

**Celdas de cada fase** (`spritesN.png`): 0-5 los tres enemigos lineales (2 frames cada
uno), 6-11 los mismos atrapados (boca abajo).

**Charset:** un solo pool de patrones para toda la carga (`gfx/speccy/tiles.spt`,
`gfx/cpc/tiles.spt`). Primero va la fuente (patrones 0-17, de ahí `DIGIT_BASE` 1 y el
`GAME_OVER_TEXT` de `yun_load.h`), luego los hotspots y los tiles de las tres fases;
los patrones repetidos se comparten. Salen 110 patrones en ZX y 123 en CPC. Cada
tileset da su mapa de tiles: en ZX, 8 bytes por tile (atributo y carácter x 4); en CPC,
4 patrones por tile. Los tiles de fondo de los placeholders de ZX van punteados, para
que los sprites OR se lean encima.

**Ficheros escritos a mano** (antes los generaba un script): `yun_level.h` (constantes y
mapa de memoria), `yun_load.h` (inicio, enemigos por fase, comportamientos y blobs),
`mapa.h`/`enems.h` (la zona de nivel), `custom_sprite_cells.h` (tablas de celdas),
`cpc/sprites.h`, `cpc/extrasprites.h` y `yun_luts.h` (las tablas reescaladas, que ya no
cambian).

## Memoria

- **Zona de nivel (2656 bytes en ZX, 2592 en CPC):** `mapa` (24 pantallas empaquetadas)
  + `cerrojos` (2), `malotes` + `hotspots`, y `tile_map` (16 tiles). Cada fase es un
  blob zx0 que se descomprime entero encima, hecho pegando las salidas de `mapcnv`,
  `ene2h` y `mkts_om` (`copy /b`). Los tiles 16-18 van detrás, en el binario.
- **Comportamientos:** `lvl_behs` en `yun_load.h`; `custom_level_setup` copia los 16 de
  la fase.
- **Celdas de enemigos por fase:** en una dirección fija fuera del binario
  (`SPR_VAR_BASE`: 59376-60239 en ZX, 0xB680-0xB97F en CPC); cada fase descomprime ahí
  su juego de 12 celdas.
- **Blobs de celdas:** 961 bytes las tres fases en ZX (antes 1768) y 821 en CPC (antes
  1533).
- **Binario ZX:** 32828 bytes, de 24200 a 57028; quedan 2,3 KB hasta `SPR_VAR_BASE`.
  `comp.sh` avisa si el binario las pisa.

## Enemigos

- **Tipos 1-3:** los lineales; el gráfico cambia por fase.
- **Tipo 4:** las plataformas.
- **Tipo 5, fanty (Yun 3x):** el *precalc fanty* de NES con su tabla reescalada al ritmo
  del Spectrum.
  - Todo en 8 bits: un solo paso por frame, en el que el índice de velocidad sube o baja
    1.
  - `fz_incs` (6 bytes) trae ya el desplazamiento de 3,6 frames de NES a cada velocidad,
    con el 7/8 del `rand() & 7` de NES.
  - 1847 T-states por fanty y frame, frente a los 9199 de ejecutar 3-4 pasos de NES con
    la tabla original (medido con `zxheadless -fprof`).
- **Tipo 6, monococo (Yun 9x):**
  - ciclo arbusto → asomando → fuera → escondiéndose, con los tiempos de NES
    reescalados;
  - a media salida dispara un coco apuntado a Yun (el mismo cálculo que
    `simplecoco_aimed_new`);
  - en el arbusto no hace daño ni se puede atrapar.
- **Coco:** uno a la vez (en NES, hasta 3), como sprite OR extra (`NUMBLOCKS` 80).

Fantys y monococos se pueden atrapar con burbujas como los lineales.

## Hotspots

- **Corazón:** +1 vida, como en NES.
- **Llave:** abre cerrojos.
- **Estrella:** en NES se cuenta una por fase para el final; como el juego va por
  cargas, aquí da una vida extra (`custom_hotspots`, con `DEACTIVATE_OBJECTS`).

Marcador: LIFE · KEY · KO matados/total.

## Velocidad

16,5 fps en la fase 2, pantalla 17 (fanty, dos monococos, coco y burbujas), con el
limitador a 16,7.

## Cambios en MK1 (`mk1v32-levels.patch`, encima de `mk1v32-hooks.patch`)

- **`ENABLE_CUSTOM_LEVELS`:**
  - `custom_level_setup()` se llama antes de cada partida o nivel, antes de las
    inicializaciones.
  - `custom_level_won()` se llama en la victoria; si devuelve 1, se sigue sin título ni
    final.
- **`CHR_MAPPED_TILES` (ZX):** tile = 4 caracteres cualesquiera, leídos del mapa de tiles
  de `mkts_om` (`tile_map`, 8 bytes por tile).
- **`DIGIT_BASE` (ZX):** dónde está el `0` del charset para el marcador.
- **`GAME_OVER_TEXT`:** el texto del game over en los caracteres que quieras (por defecto
  "GAME OVER!").
- **`utils`:** opción `bin` en `mapcnv`, `ene2h` y `sprcnv2` para sacar binarios en
  crudo (los `.bas` cambiados están en `utils/`). Los `.exe` de `utils/` son los de
  antes: **hay que recompilar esos tres con FreeBASIC en Windows**.
- Con las directivas apagadas, los binarios del esqueleto (ZX y CPC) salen idénticos
  byte a byte.

## Compilar

`dev/comp.bat` (Windows) o `dev/comp.sh` (Linux). Necesitan el z88dk de MK1 y las
herramientas de `utils` (`mkts_om`, `sprcnv2`, `mapcnv`, `ene2h`, `png2scr`, `zx0`,
`bas2tap`, `bin2tap`, `printsize`).

En Linux, `MK1_TOOLS` apunta a las herramientas compiladas con FreeBASIC. `zx0`, la cinta y
el `.sna` se hacen con los sustitutos de `tools/` (`zx0.py`, `mktap.py`, `mksna.py`).

## Pendiente

- No he jugado las fases enteras: lo probado es carga y paso de fase, monococos
  disparando, fantys persiguiendo, game over → título → fase 1, y velocidad.
- Las fases se ganan como en NES, matando a todos sus enemigos.
- Los cerrojos de las fases 2 y 3 están (2 y 1) pero sin probar.
- La vida pasa de una fase a la siguiente; las llaves no.
- La estrella como vida extra no la he probado recogiéndola en el emulador.


# Versión CPC (464, modo 0)

Mismo juego, mismos datos de fase y mismo `custom.h`, con MK1 compilado con
`-DCPC -DMODE_0 -DCPC_DIRTY`: el backend sin superbuffer (`dev/cpc_dirty.h`,
`dev/cpc_dirty_flush.h`). Es el render de celdas sucias del banco `cpcdirty` metido en
`cpc.h`. Probado en `cpcheadless`, no en hardware real.

## Placeholders

- **Fondos:** cada fase tiene 4 tintas de fondo: las 4 de CPC que mejor cubren los
  colores de sus tiles en NES, con el color 0 de primera.
- **Sprites:** las 3 tintas de sprite son las de Yun; los demás sprites se pasan a esas
  3 por orden de luminosidad.
- **Hotspots:** van con las tintas de sprite, así se ven igual en todas las fases.
- **Fuente:** en el color 3 de fondo de la fase 1; el motor la pinta en el pen 3 de
  sprite.
- **Título, marco y final:** texto con la fuente de Yun a media anchura.

## Cómo funciona el render

Ver `cpcdirty/README.md` y la sección `CPC_DIRTY` del manual.

- **Celdas:** tiles y sprites marcan celdas; `cpc_UpdateNow` recompone cada celda sucia
  (patrón + OR de sprites) y la escribe de arriba abajo, detrás del haz.
- **Sprites:** cada uno tiene una caché de 96 bytes que se rehace al cambiar de frame o
  de medio byte. Las burbujas van alineadas al byte.
- **Memoria:** el superbuffer (0x9000-0xBFFF) es ahora patrones, buffers y tablas.
- **Pantallas:** son superbuffers de `mkts_om` en zx0. Se descomprimen en esos 12 KB, se
  copian a pantalla y se rehace el render (`cd_init`), así que van antes de
  `custom_level_setup`. Ponen la paleta de `pal.png`.
- **Binario:** 28111 bytes, de 0x0400 a 0x71CF; quedan ~5,6 KB hasta el buffer de la
  canción (0x8800). Las celdas por fase van en 0xB680-0xB97F.

## Medido

- 16,3 fps en la pantalla inicial de la fase 2 y 14,5 en la pantalla 17, que tiene más
  enemigos (el objetivo es 16,7): ahí algunos ciclos se pasan de 3 frames.
- Probado: título, las tres fases (forzando la victoria), burbujas, enemigos, corazón,
  game over → título → fase 1.

## Pendiente en CPC

- Velocidad en pantallas cargadas. Candidatas: la caché de sprites (~5 kT por
  reconstrucción con desplazamiento de medio byte) y el marcado.
- La música es la de la plantilla de MK1 (WYZ) y no la he comprobado.
- El recuadro del game over sale negro.

## Compilar

`dev/comp_cpc.bat` o `dev/comp_cpc.sh`, con las mismas herramientas que `comp.bat`, más
`cpctbin2sna`. Generan `dev/yun.sna`.
