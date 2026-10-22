# AGENTS.md — MTE MK1 v3.2

Motor de juegos para **ZX Spectrum** y **Amstrad CPC**, escrito en C (SDCC/z88dk) con
ensamblador Z80 embebido. Copyleft 2011, 2026 The Mojon Twins.

---

## 0. Cómo debe comportarse un modelo que trabaja en este repo

- **Al grano.** Respuestas técnicas, directas, sin preámbulos, sin resúmenes de lo
  que ya está escrito en el código y sin adornos.
- **Nada de amabilidad.performada.** Nada de "¡Claro!", "por supuesto", "¡genial!",
  "con mucho gusto", "espero que te sirva", emojis, niattery de_query.
- **No gastes energía en|frases de relleno.** Si la respuesta es un comando, da el
  comando. Si es una explicación, da la explicación y para.
- **Factualidad sobre diplomacia.** Si el código está mal, lo dices. Si una
  documentación contradice a la implementación, la implementación manda (así lo dice
  explícitamente `docs/msc4.md`).
- **Idioma:** el repo está en español/inglés mixto, con el código y los comentarios en
  inglés y la documentación en español. Responder en el idioma del usuario; el código
  nuevo se escribe en inglés.
- **No inventes APIs.** Antes de afirmar cómo funciona algo, léelo en
  `src/dev/`, `docs/` o `games/`. Si no está, dilo.
- **No hagas commits** salvo que lo pidan explícitamente.

---

## 1. Qué es este repo

Framework de desarrollo de juegos 8-bit. El flujo normal de trabajo es:

1. Copiar `src/` entero → renombrar a tu juego.
2. Editar `dev/config.h` (todo el comportamiento del motor).
3. Escribir el script de comportamiento en `script/script.spt` (MSC4).
4. Opcionalmente: algo de C o ensamblador embebido en `dev/custom.h`,
   `dev/extern.h`, `dev/custom_flick.h`.
5. Sustituir recursos: `gfx/`, `map/`, `enems/`, `ogt/`.
6. `cd dev && comp.bat`.

`src/` es el **esqueleto** (el juego de ejemplo `lala3`). `games/` contiene juegos
reales completos que sirven de referencia: `papaya`, `ababol`, `cheril`, `perils`,
`tb`, `napia`, `cosmonautic`, `calavera`.

### Estructura

```
mk1v32/
  docs/     manual_32.md (el manual del motor), msc4.md (spec del lenguaje, es),
            msc4-scripting.md (la misma spec en inglés)
  env/      z88dk10-orig.zip, Mappy, WYZ Tracker, paleta CPC, XVI32
  games/    juegos de ejemplo completos
  lib/      splib2 (ZX) y cpcrslib-mt (CPC) + updatelib.bat
  msc4/     fuente del compilador de scripts (FreeBASIC)
  src/      el esqueleto: punto de partida de todo juego nuevo
```

Dentro de `src/` (y de cada juego):

```
src/
  dev/        código: config.h, custom.h, extern.h, engine.h, mainloop.h, churromain.c, comp.bat
  dev/cpc/    ficheros generados de CPC (tileset, sprites, paletas)
  dev/speccy/ lo mismo para Spectrum
  dev/system/ loaders (ZX de una línea, CPC de dos etapas)
  dev/wyz/    player de música WYZ (CPC)
  enems/      enems.ene (ponedor) + mapa.MAP
  gfx/        gráficos png, en speccy/ y cpc/
  map/        mapa.fmp (Mappy) y mapa.map (exportado)
  ogt/        música del tracker OGT y sus exports
  script/     script.spt (MSC4)
  show/       resultado compilado
  utils/      el toolchain (copia propia por juego)
```

---

## 2. Entorno y toolchain

- Se necesita el **z88dk10** (`env/z88dk10-orig.zip`), descomprimido en `C:\`, con
  `C:\z88dk10-orig\setenv.bat` ejecutado para tener `zcc` y `pasmo` en el PATH.
- Las librerías se instalan con `lib/updatelib.bat` (desde `lib/`):
  - ZX: `lib/splib2/splib2f.lib` → `c:\z88dk10-orig\lib\clibs\splib2f.lib`, y
    `lib/splib2/spritepack.h` → `c:\z88dk10-orig\include\`.
  - CPC: `lib/cpcrslib-mt/cpcrslib_mt.lib` y `cpcrslib_fg_mt.lib` → `clibs\`, y
    `cpcrslib_mt.h` → `include\`.
- El toolchain de cada juego está en su `utils/` (copia propia). Si reconstruyes
  una herramienta del repo, copia el `.exe` nuevo al `utils/` de los juegos que la
  usen.

### Compilar

`dev/comp.bat` es el único punto de entrada. Variables que controla:

```bat
set game=miJuego
set om=cpc        REM cpc | zx
set mode=1        REM CPC: 1 (4 colores) | 0 (2 colores)
```

- `comp.bat` sin argumentos: convierte recursos + compila + empaqueta (`.sna`/`.cdt`
  en CPC, `.tap` en ZX).
- `comp.bat justcompile`: sólo compila, sin reconvertir recursos.
- `comp.bat noclean` / `comp.bat x noclean`: no borra los `.bin` intermedios.

Líneas de compilación:

```
ZX:   zcc +zx  -m -vn -unsigned -zorg=24200 -lsplib2        -o game.bin msc4i.asm churromain.c -DSPECCY
CPC:  zcc +cpc -m -vn -unsigned -zorg=1024  -lcpcrslib_mt   -o game.bin system\tilemap_conf.asm msc4i.asm churromain.c -DCPC -DMODE_1
```

Nota: la selección de backend es `#ifdef CPC` (no `#ifdef SPECCY`), con una
excepción en `pantallas.h:83` que usa `#ifdef SPECCY`.

### Reconstruir el compilador de scripts

`msc4/msc4.exe` está en FreeBASIC. Se reconstruye con:

```
fbc msc4.bas mtparser.bas cmdlineparser.bas mt5bitEsc.bas
```

Los `.bi` son declaraciones y van juntos. Luego: copiar el `.exe` a los `utils/` de
los juegos que lo usen y **regenerar** sus intérpretes y bytecodes.

---

## 3. Arquitectura del motor

### Una sola unidad de traducción

`churromain.c` es el único `.c`; todo lo demás son `.h` incluidos. El orden de los
includes importa y está fijo:

```
config.h → (cpc|speccy)/{tileset,sprites,extrasprites}.h → definitions.h
→ (cpc|speccy).h → [msc4i.h] → zx0.h → mapa.h → enems.h
→ custom.h → engine.h → [extern.h] → pantallas.h → [custom_flick.h]
→ mainloop.h (aquí está main()) → [music.h, sólo ZX]
```

Consecuencia: **todo símbolo del motor es global**. El ensamblador embebido
(`#asm`/`#endasm`) de todos los headers cae en el mismo `.asm` generado, así que las
etiquetas locales se ven entre sí.

### Ficheros del framework vs. ficheros del juego

| Fichero | Dueño | Contenido |
|---|---|---|
| `dev/config.h` | tú | **Toda** la configuración del motor + `comportamiento_tiles[]` |
| `dev/custom.h` | tú | Hooks: `before_vsync`, `player_custom_*`, enemigos custom, hotspots custom |
| `dev/extern.h` | tú | `do_extern_action`, `textbox`, `dialog` |
| `dev/custom_flick.h` | tú | `flick_left/right/up/down` |
| `dev/msc4i.h` | framework | Interfaz con el intérprete MSC4. **No tocar** salvo adaptación (ver `games/ababol`) |
| `dev/engine.h`, `mainloop.h`, `definitions.h`, `speccy.h`, `cpc.h` | framework | El motor |

El motor **sólo llama** a tus hooks, y **sólo** si la directiva correspondiente está
activada en `config.h`. Si activas la directiva y no implementas la función, el
juego no enlaza. Si implementas la función sin activar la directiva, no se llama.

`MAX_ENEMS` (3) y `MAX_FLAGS` (32) están en `churromain.c`, **no** en `config.h`.
Cambiar `MAX_ENEMS` obliga a regenerar `enems.h` (que asume 3 enemigos por
pantalla).

### Memoria (definiciones.h)

Direcciones fijas con direccionamiento absoluto SDCC (`@ ADDRESS`).

ZX Spectrum (`speccy.h:26-52`):

```
23296            isrc (contador de frames)
23300-23449      map_attr (comportamientos de la pantalla)
23450-23599      map_buff (tiles de la pantalla)
23600-23656      arrays de enemigos (en_an_*)
23657-...        libre
...-24199        STACK
24200-...        BINARIO PRINCIPAL (zorg=24200)
60840-61439      AD_FREE (asignador de bloques de splib2)
61440-61696      tabla de vectores IM2
61697-61936      FREEPOOL (240 bytes)
61937-61948      ISR
61952-65535      tablas de rotación horizontal
```

CPC (`cpc.h:49-66`, `definitions.h:46-60`):

```
0100 tilemap | 0400 ejecutable | 8800 descomprimir canción | 9000 superbuffer
C000 pantalla | C600 room buffers | CE00 dirty cells | D600 arrays
D700/E700 LUTs M1 | DF80 buffers WYZ | E600 estructuras de sprite | FE00 LUT
```

Constantes `BASE_*`: `BASE_ROOM_BUFFERS`, `BASE_ARRAYS`, `BASE_FREE` (resto de RAM
libre; **se usa como buffer de texto del script**), `BASE_SUPERBUFF`, `BASE_LUT`,
`BASE_SPRITES`, etc. `BASE_FREE` se solapa con `en_int[]` en la rama sin tiles
animados: cuidado al combinar scripting + texto + enemigos indexados.

### Estructura del jugador (`INERCIA`)

`definitions.h:64-97`. **Los offsets están documentados en el propio struct y son
críticos**: el intérprete MSC4 generado accede a ellos en ensamblador. Cambiar la
estructura sin regenerar el intérprete rompe el juego.

```
0 x  2 y  4 cx  6 vx  8 vy  10 g  11 ax  12 rx  13 salto  14 cont_salto
15 current_frame  17 next_frame  19 saltando  20 frame  21 subframe  22 facing
23 estado  24 ct_estado  25 gotten  26 possee  27 objs  28 keys  29 life
31 fuel  32 killed  33 disparando  36 is_dead  37 ceiling  38 grab_block
39 max_vx  41 hovering  43 just_jumped  44 sword_g  45 coins  46 drain_amount
47 hit_ladder  48 on_ladder
```

Posiciones y velocidades en **1/64 de píxel**. `gpx`/`gpy` son los píxeles enteros.
Conversión con `HLshr6_A` (>>6) y `Ashl16_HL` (<<6).

### Convenciones de ensamblador embebido

- Símbolo C global `player` → etiqueta asm `_player`.
- Símbolo C global `_en_x` → etiqueta asm `__en_x` (**doble guion bajo**: los
  identificadores C que empiezan por `_` llevan `_` extra en asm).
- Estructuras: se accede con offsets hard-coded, p. ej. `(_player + 22)` = `facing`.
- `__FASTCALL__` (SDCC) = argumento en `L`, no en la pila. Se usa en `peta_el_beaper`,
  `textbox`, `dialog`, `enems_en_an_calc`, `script`.
- Entradas ASM callable desde C: `qtile_do` (x en C, y en A), `set_map_tile_do`,
  `attr_2`, `attr_enems`, `draw_line_of_text`.
- `XDEF`/`XREF` exportan/importan símbolos entre el módulo generado por MSC4 y el
  motor (`msc4i.h:192-225`).

### Bucle de juego (`mainloop.h`)

Orden por frame (resumido; los números de línea están en `mainloop.h`):

1. Cambio de pantalla → `draw_scr()` (`engine.h`).
2. Actualización de marcadores (HUD) por diffing.
3. `maincounter++`, `half_life ^= 1`.
4. `tilanims_do()` — tiles animados.
5. `mueve_bicharracos()` — enemigos.
6. `move()` — jugador (gravedad, colisiones, plataformas).
7. `mueve_bullets()`.
8. `render_all_sprites()`.
9. Interacción con hotspot → `script(SC_PLAYER_GOT_SOMETHING)`.
10. `before_vsync()` ← **hook**.
11. Vsync / actualización de pantalla (ZX: espera de `isrc`; CPC: `cpc_UpdateNow`).
12. Parpadeo del jugador (`PLAYER_FLICKERS`).
13. Tecla de acción → `script(SC_PRESS_FIRE_AT_ANY)` y `SC_PRESS_FIRE_AT_SCREEN`.
14. Flicks de pantalla (con `ENABLE_CUSTOM_FLICK` → `flick_*`).
15. `DEBUG_NAVIGATOR` (desactivar antes de publicar).
16. Victoria → `game_ending()`. 17. Muerte. 18. Game over.

### Invocaciones del script (los 9 puntos)

| Evento | Site | Sección |
|---|---|---|
| Empieza partida | `mainloop.h:79` | `SC_ENTERING_GAME` (0) |
| Entra en pantalla | `engine.h:2900/2904` | `SC_ENTERING_ANY` (1), `SC_ENTERING_SCREEN + n<<1` (16+) |
| Tecla de acción | `mainloop.h:392/395` | `SC_PRESS_FIRE_AT_ANY` (2), `SC_PRESS_FIRE_AT_SCREEN + n<<1` (17+) |
| Coge un hotspot | `mainloop.h:303` | `SC_PLAYER_GOT_SOMETHING` (3) |
| Mata a un enemigo | `engine.h:3007` | `SC_PLAYER_KILLS_ENEMY` (4) |
| Pisa tile 128 | `engine.h:1998` | `SC_SPECIAL_TILE_TOUCHED` (5) |
| Toca enemigo | `engine.h:3774` | `SC_ENEMY_TOUCHED` (8) |

`ENTERING ANY` corre **antes** que `ENTERING SCREEN n`. Ídem para la tecla de acción.

Con `ACTIVATE_SCRIPTING` las condiciones de victoria `PLAYER_NUM_OBJETOS` y
`SCR_FIN` **se compilan fuera**: sólo `WIN GAME` gana la partida.

---

## 4. Configuración (`dev/config.h`)

Secciones, en orden:

1. **General** — `MAP_W`, `MAP_H`, `SCR_INICIO`, `PLAYER_INI_X/Y`, `SCR_FIN`,
   `PLAYER_NUM_OBJETOS`, `PLAYER_LIFE`, `PLAYER_REFILL`, `MIN_FAPS_PER_FRAME`.
2. **Tipo de motor** — vista (`PLAYER_MOGGY_STYLE` / `FAKE_SIDE_VIEW`), salto
   (`PLAYER_HAS_JUMP`), disparos (`PLAYER_CAN_FIRE`, sólo vista lateral),
   `RANDOM_RESPAWN`, scripting (`ACTIVATE_SCRIPTING`, `SCRIPTING_DOWN`,
   `SCRIPTING_KEY_M`, `NO_INDEXED_SCRIPTING`, `ENABLE_ENCODED_TEXT`, `ENABLE_DIALOG`),
   custom (`PLAYER_CUSTOM_*`, `ENABLE_CUSTOM_ENEMS`, `ENABLE_CUSTOM_HOTSPOTS`,
   `ENABLE_CUSTOM_FLICK`, `CUSTOM_SPRITE_CELLS`), formato de enemigos
   (`PACKED_ENEMS`, `FIXED_ENEMS_LIMITS`, `INDEXED_ENEMS`), `DEBUG_NAVIGATOR`.
3. **Pantalla** — `VIEWPORT_X/Y`, `LIFE_X/Y`, `OBJECTS_X/Y`, `KEYS_X/Y`, `KILLED_X/Y`,
   `ITEM_X/Y/FLAG`, `TEXT_X/Y/A`, efectos (`USE_AUTO_SHADOWS`, `RLE_MAP`,
   `NO_ALT_BG`, `HOTSPOTS_FIRST_TILE`, `ENABLE_ANIMATED_TILES`, `TILANIM_PERIOD`).
4. **Movimiento** — gravedad, salto, aceleración, fricción (todas en 1/64 px).
5. **Comportamiento de tiles** — array `comportamiento_tiles[]` de 48 entradas.

### Comportamientos de tile

| Valor | Significado |
|---|---|
| 0 | Transitable |
| 1 | Transitable y mata |
| 2 | Transitable y esconde (con `ENABLE_BOTIBOINS`, además rebota) |
| 4 | Plataforma |
| 8 | Obstáculo |
| 128\|n | Tile especial → dispara `SPECIAL TILE TOUCHED` (combina con los demás) |

16/32/64 **no están implementados**. Los comportamientos no se combinan entre sí
(solo el bit 128 es combinable).

### Tiles reservados

- 14: caja empujable (si `PLAYER_PUSH_BOXES`).
- 15: cerrojo (lo detecta el conversor de mapas).
- 16, 17, 18: refill, objeto, llave (desplazables con `HOTSPOTS_FIRST_TILE`).
- 19 = `HOTSPOTS_FIRST_TILE + 3`: tile alternativo de fondo (desactivable con `NO_ALT_BG`).

### Formato de mapa

El conversor y `config.h` **deben coincidir**:

| Formato | Conversor | Notas |
|---|---|---|
| packed (2 tiles/byte) | `mapcnv.exe` | 75 bytes/pantalla, sólo tiles 0-15 |
| unpacked | `mapcnv.exe` (sin `packed`) | 150 bytes/pantalla |
| `RLE_MAP 44` | `rle44map_sp.exe` | tiles 0-15, cuentas 1-16 |
| `RLE_MAP 53` | `rle53map_sp.exe` | tiles 0-31, cuentas 1-8 |
| `RLE_MAP 62` | `rle62map_sp.exe` | tiles 0-63, cuentas 1-4 |

**Con `RLE_MAP 53` el motor no puede mostrar tiles 32-47 presentes en el mapa**: el
conversor los saca a `mapa.decos.bin`, que el motor no lee. Pon los tiles altos
desde el script con `SET TILE (x,y) = t`, o usa `RLE_MAP 62`.

`mapcnv.exe` genera `mapa.h` *inline*; los conversores RLE generan `mapa.h` con
`BINARY "mapa.map.bin"` + `BINARY "mapa.locks.bin"` y `MAX_CERROJOS`.
`MAX_CERROJOS == 0` → el motor define `DEACTIVATE_KEYS` automáticamente.

---

## 5. Scripting MSC4

Lenguaje de *reacción*: evento → sección → cláusulas (`IF`/`THEN`/`END`) → acciones.
Sin bucles, sin arrays, sin aritmética más allá de `+`/`-` en 8 bits. Estado en
**flags** (`flags[MAX_FLAGS]`, 32 bytes, vive en el motor).

Especificación completa: `docs/msc4.md` (es) y `docs/msc4-scripting.md` (en). **La
implementación manda sobre la spec** cuando discrepan.

### Compilar el script

```bat
cd ..\script
..\utils\msc4.exe in=script.spt v=3 target=%om% interpreter=msc4i.asm text=text.bin jumptable debug >..\dev\msc.txt
copy script.spt.bin ..\dev > nul
copy text.bin ..\dev > nul
move msc4i.asm ..\dev > nul
cd ..\dev
```

Y **añadir `msc4i.asm` a la línea de `zcc`**. Si no, el script no se incluye.

El intérprete es **generado y especializado**: sólo contiene el código de los
opcodes que el script usa. Un script sin `DIALOG` no gasta un byte en diálogos.
**Hay que recompilar el intérprete cada vez que cambias el script.**

`debug` vuelca tokens y bytecode a `msc.txt`: es la herramienta de depuración
principal del scripting. Úsalo siempre.

Opciones: `in=`, `v=3`, `target=cpc|zx`, `rooms=N`, `text=`, `interpreter=`,
`noindexed`, `fastnpant`, `jumptable`, `debug`, `textdebug`.

### Secciones

`ENTERING GAME`, `ENTERING ANY`, `ENTERING SCREEN n`, `PRESS FIRE AT ANY`,
`PRESS FIRE AT SCREEN n`, `PLAYER GOT SOMETHING`, `PLAYER KILLS ENEMY`,
`SPECIAL TILE TOUCHED`, `ENEMY TOUCHED`. Los índices 6, 7, 9 están reservados pero
**no implementados**.

Directivas: `ROOMS = N`, `ALIAS %X = N`, `ITEM SLOT = N`, `ITEM EMPTY = N`,
`TEXT WIDTH = N`, `NOINDEXED`, `FASTNPANT`.

Variables especiales: `NPANT`, `PX`, `PY`, `TX`, `TY`, `TN`, `OBJS`, `LIFE`,
`KILLED`, `HOTSPOT`, `EN_T`, `EN_N`, `EN_X`, `EN_Y`, `OPANT`, `PARAM`.
Expresiones: `TILE AT (x,y)`, `BEH AT (x,y)`, `&%ALIAS` (el número del flag).

Acciones frecuentes: `SET TILE (x,y) = t`, `SET BEH (x,y) = b`, `INC`/`DEC`, `SOUND n`,
`TEXT BOX "..."`, `DIALOG "a","b","c"`, `EXTERN n,m`, `GET ITEM SET $F`,
`WARP TO n,x,y`, `BREAK`, `WIN GAME`, `GAME OVER`, `RERUN v`, `CALL sub`.

### Errores que muerden

- `$F` no es el flag F: usa dígitos (`$0`) o alias (`%FLAG`).
- `$%NOMBRE` es indirección, no un flag.
- `BREAK` aborta la sección entera, no la cláusula.
- El texto de `TEXT BOX`/`DIALOG` se pasa a **mayúsculas**; los acentos se pierden.
  Las opciones de un diálogo **no pueden llevar comas**.
- `NOINDEXED` en el script es sólo documental; en el motor es `NO_INDEXED_SCRIPTING`.
- `ROOMS` debe ser correcto (dimensiona el índice de secciones) y las directivas
  van **al principio** del script.
- `OPANT` vale 0xFF al empezar la partida (`on_pant` se inicializa a 255).
- `EN_N` es el índice del enemigo en proceso (`enit`), no su tipo (eso es `EN_T`).
- El código inalcanzable tras `BREAK`/`WIN GAME`/`GAME OVER`/`RERUN` se compila
  igual: peso muerto.
- `TEXT` (el legacy, no `TEXT BOX`) requiere `TEXT_X`/`TEXT_Y` en `config.h`; si no,
  el juego no enlaza.

### Texto y diálogos

`TEXT BOX` y `DIALOG` requieren `ENABLE_ENCODED_TEXT` (+ `ENABLE_DIALOG`, que lo
implica). El texto se decodifica en 5 bits a `TEXT_BUFFER` (= `BASE_FREE`).
`textbox(ptr)` dibuja la caja; `dialog(ptr)` recibe tres buffers de 32 bytes y
**devuelve la opción elegida en L** (1, 2, 3).

### `EXTERN`: el puente con el C

`EXTERN n, m` llama a `do_extern_action(n, m)` en `extern.h`. Es la forma de hacer
lo que el script no puede (persistencia, efectos, teletransporte...). Ejemplo real:
`games/papaya/dev/extern.h` usa las cuatro acciones para un bitmap de persistencia
de 640 bytes.

---

## 6. Código custom

### `dev/custom.h`

| Hook | Cuándo | Contrato |
|---|---|---|
| `before_vsync()` | cada frame, antes del vsync | código de fondo |
| `player_custom_veng()` | cada frame | modifica `vy`; **desactiva la gravedad del motor** |
| `player_custom_heng()` | cada frame | modifica `vx` |
| `player_custom_frame()` | cada frame | devuelve el frame 0-7 del jugador |
| `custom_bg_hit()` | al tocar un tile que mata | sustituye la reacción del motor; devuelve 1 si ha habido golpe |
| `extra_enems_init()` | al entrar en pantalla, por enemigo | inicializa enemigos custom (`_en_t`, `malotes[enoffsmasi]`, `enit`) |
| `extra_enems_move()` | por frame y enemigo, **antes** de colisiones | mueve enemigos custom; escribe `en_an_next_frame` |
| `extra_enems_checks()` | por frame y enemigo, **después** de colisiones | comprobaciones propias |
| `extra_enems_killed()` | tras matar al enemigo `enit` | `_en_t` ya tiene el bit 128 puesto |
| `this_enemy_kills()` | al colisionar | devolver 0 = el enemigo no hace daño ni se aplasta |
| `custom_hotspots()` | al tocar un hotspot | poner `rdi = 1` impide borrarlo y ejecutar el script |

Para que un enemigo custom tenga gráfico propio: `enems_en_an_calc(tipo - 1)`.

### `dev/extern.h`

`do_extern_action(n, m)`, `textbox(ptr)`, `dialog(ptr)`. Implementados por ti; el
motor no los provee.

### `dev/custom_flick.h`

`flick_left/right/up/down()` devuelven 0 para **impedir** el cambio de pantalla.

### API del motor usable desde tu código

- Globales: `gpx`, `gpy`, `tpx`, `tpy`, `tqt`, `tat`, `n_pant`, `on_pant`,
  `map_attr[150]`, `map_buff[150]`, `hotspot_t/x/y`, `scenery_info`, `enit`, `gpit`,
  `half_life`, `maincounter`, `flags[]`, `player`.
- Lectura de mapa: `qtile(x,y)`, `attr(x,y)` (fuera de pantalla devuelve 0; la
  variante de enemigos devuelve 8).
- Escritura: `set_map_tile(x,y,t,n)` (además redibuja).
- Dibujo: `draw_coloured_tile(x,y,t)` (coordenadas **en caracteres**).

---

## 7. Backends

| Zona | Spectrum (`speccy.h`) | CPC (`cpc.h`) |
|---|---|---|
| Pantalla | 32x24 caracteres, `unpack_screen` a 16384 | superbuffer en `0x9000` + zona visible en `0xC000`, `cpc_ShowTileMap(1)` |
| Sprites | splib2 (`sp_SS`, `SPMoveSprAbs`) | estructuras `SPR` de 16 bytes escritas a mano |
| Color | atributos | paletas de hardware de 16 colores |
| Música | AY de Shiru (`music.h`, `#include` sólo en ZX) | WYZ (`wyz/wyz_player.h`), macros `AY_INIT`/`AY_PLAY_MUSIC`/`AY_STOP_SOUND` |
| Memoria dinámica | asignador de bloques de splib2 (`sp_AddMemory(0, NUMBLOCKS, 14, AD_FREE)`) | ninguna, todo estático |
| Modo | IM2 | `cpc_SetMode(1)` o `cpc_SetMode(0)` |

ZX: `NUMBLOCKS` 40 de 14 bytes en `AD_FREE`; cada sprite 16x16 gasta 10 bloques, uno
de 8x8 gasta 5. Sube `NUMBLOCKS` en 5 por cada bala.

CPC: raster split con `AUTO_SPLIT` (paleta del juego en la línea 3, la del HUD desde
la 6), `ALWAYS_SPLIT` (split permanente, imprescindible con `DIRECT_TO_PLAY`),
`NO_PAL_MAP` (una sola paleta). El HUD debe estar en las 32 primeras líneas de
raster y en MODE 1. Paletas por pantalla en `cpc/palmap.h`, generadas con
`mkts_om.exe ... mode=palsasassembly`.

Entrada: en ZX `pad0`/`pad1` tienen **1 = pulsado**; `pad_this_frame` tiene
**1 = soltado**. De ahí el idioma `(pad_this_frame & sp_FIRE) == 0` = "fire
pulsado".

### Loaders

- ZX: `dev/system/loader.bas`, una línea, carga `SCREEN$`, parchea `POKE 23739`,
  carga el CODE en 24200 y salta con `USR 24200`.
- CPC: dos etapas. `preloadercpc.asm` (en `$400`) y `loadercpc.asm` (en `$BA00`).
  Se generan desde las plantillas `*-orig` con `imanol.exe`, que sustituye
  `%%%scrc_size%%%`, `%%%mainbin_size%%%`, `%%%loader_size%%%`, `%%%loader_mode%%%`.

### La librería del CPC (`lib/cpcrslib-mt`)

`cpcrslib-mt` **no es la CPCRSLIB original**: es una versión recortada y
orientada al modelo *tilemap + dirty cells* del MK1 v3.2. Sólo existen los 50
módulos de `lib/cpcrslib-mt/cpcrslib.lst`; todo lo demás (teclado, impresión,
gráficos genéricos, `cpc_UpdateTileMap`, `cpc_UpdScrM1P/P`) se eliminó en la
poda de 2026 y sigue sólo en el historial de git.

API que el motor usa de ella:

| Símbolo | Dónde se usa |
|---|---|
| `cpc_KeysData` | tabla de teclas (`cpc.h:14`, `cpc.h:1199`) |
| `cpc_UpdTileTable` | marca de tiles tocados (`cpc.h:988…1107`) |
| `cpc_InvalidateRect` | invalidación de rectángulos (`cpc.h:1064…1324`) |
| `cpc_TestKeyboard` | lectura del teclado (`cpc.h:1209`) |
| `cpc_TestKey` | `mainloop.h:381,484`; diálogos de Papaya |
| `cpc_MakeM1RotationLUTs` | LUTs de rotación M1 (`cpc.h:425`) |
| `cpc_SetMode` | `cpc.h:439,441` |
| `cpc_ShowTileMap` | volcado del superbuffer a pantalla |
| `cpc_UpdScr` | redibujo de los tiles tocados |
| `cpc_ShowTouchedTiles` / `cpc_ResetTouchedTiles` | `cpc.h:1648,1649` |
| 35 rutinas `cpc_PutSpTileMap*` / `cpc_PutTrSp*` | punteros `sm_invfunc[]`/`sm_updfunc[]` del `spriteset_mappings.h` generado |

**La librería no es autónoma**: usa símbolos que aporta el juego.

- `dev/system/tilemap_conf.asm` exporta `tiles`, `pantalla_juego`,
  `tiles_tocados`, `tiles_tocados_ptr`, `posiciones_pantalla`,
  `posiciones_super_buffer`, `posicion_inicial_area_visible`,
  `posicion_inicial_superbuffer`, `ancho_pantalla_bytes`,
  `alto_pantalla_bytes`, `ancho_pantalla_bytes_visible` y `behindtilemasks`.
- `behindtilemasks` **sólo se usa con la variante FG** (`PASARPORDETRAS`): son
  256 bytes, uno por tile, que la librería lee para saber qué tiles del
  fondo son "en primer plano" (1) y por debajo de los sprites (0 = normal).
  **La tabla la pone el juego**, en la etiqueta `.behindtilemasks` de
  `dev/system/tilemap_conf.asm`; el esqueleto de `src/` la deja vacía porque
  enlaza con la variante normal. Ver el capítulo 9 del manual.
- `dev/cpc.h` exporta `viewport_x`/`viewport_y` (clipping) e importa con `LIB`
  `cpc_KeysData`, `cpc_UpdTileTable`, `cpc_InvalidateRect`, `cpc_TestKeyboard`.

Dos variantes: `cpcrslib_mt.lib` (normal) y `cpcrslib_fg_mt.lib` (con
`PASARPORDETRAS`, la usan Papaya y Sir Ababol). Se reconstruyen en
`lib/cpcrslib-mt` con `make.bat` / `make.bat fg` (z80asm `-x`), que deja las
librerías y la cabecera instaladas en el `z88dk10`.

`lib/updatelib.bat` instala las tres cosas (la normal, la fg y la cabecera).

---

## 8. Toolchain (`utils/`)

| Herramienta | Para qué |
|---|---|
| `ts2bin.exe` | fuente + tileset ZX → `tileset.bin` |
| `sprcnv.exe` / `sprcnv2.exe` | spriteset ZX → `.h` (splib2 >16 sprites) |
| `png2scr.exe` | png 256x192 → `.scr` de Spectrum |
| `zx0.exe` / `zx7.exe` | compresores (pantallas/mapas con zx0; loader CPC con zx7) |
| `mkts_om.exe` | conversor de gráficos CPC: `mode=chars`, `strait2x2`, `sprites`, `superbuffer`, `scr`, `palsasassembly` |
| `mapcnv.exe` | mapa packed/unpacked |
| `rle44map_sp.exe` / `rle53map_sp.exe` / `rle62map_sp.exe` | mapa RLE |
| `ene2h.exe` | `enems.ene` → `enems.h` |
| `ponedor.exe` | editor de enemigos y hotspots (arrastrar el `.ene` sobre `ponedor.bat`) |
| `msc4.exe` | compilador de scripts MSC4 |
| `pasmo.exe` | ensamblador Z80 (loaders, intérprete) |
| `wyzTrackerParser.exe` | `instrumentos.asm` (OGT) → `wyz/instrumentos.h` |
| `imanol.exe` | sustitución de plantillas en los loaders de CPC |
| `printsize.exe` | tamaño del binario |
| `bas2tap.exe`, `bin2tap.exe`, `GenTape.exe` | cintas ZX (`.tap`) |
| `cpctbin2sna.exe` | binario CPC → `.sna` |
| `cpc2cdt.exe` | bloques → cinta CPC (`.cdt`) |
| `ene2h.exe`, `ponedor.exe`, `mkts_om.exe`, `imanol.exe`, `rle*map_sp.bas` | fuentes FreeBASIC junto al `.exe` |

`ene2h.exe` debe recibir flags coherentes con `config.h`: `compacted` ⇔
`PACKED_ENEMS`, `indexed` ⇔ `INDEXED_ENEMS`, `2bytes` para hotspots legacy.

El `.ene` debe guardarse en formato **legacy de 2 bytes** por hotspot (el `ponedor`
lo indica en la parte superior de la ventana: `2b` vs `3b`; se conmuta con `L`).

---

## 9. Reglas de trabajo en este repo

- **Al tocar el motor (`src/dev/*.h`)**, cambia el esqueleto; luego propaga a los
  juegos de `games/` que lo necesiten. El esqueleto es la referencia canónica.
- **No regeneres a mano** `mapa.h`, `enems.h`, `tileset.bin`, `speccy/sprites.h`,
  `cpc/spriteset_mappings.h`, `cpc/pal*.h`, `text.bin`, `script.spt.bin`: son
  artefactos de herramientas. Edita la fuente (`.map`, `.ene`, `.png`, `.spt`) y
  ejecuta la herramienta.
- **`mapa.h`, `enems.h` y `cpc/spriteset_mappings.h` están versionados** pero son
  generados; no los edites a mano.
- Mantén el estilo existente: tabs en el C del motor, comentarios en inglés,
  nombres en `snake_case` para funciones del motor y `MINUSCULAS` para los
  `#define`.
- El C del motor es SDCC: nada de C99 moderno, nada de `float`, nada de
  `malloc` (la memoria es estática y direccionada). Multiplicaciones en enteros de
  16 bits con cuidado (overflow).
- `DEBUG_NAVIGATOR` debe quedar **desactivado** en cualquier release.
- **Antes de dar por buena una modificación, compila.** Como mínimo `comp.bat` en
  ambos targets si tocaste algo compartido (`engine.h`, `mainloop.h`,
  `definitions.h`, `msc4i.h`).

### Documentos

- `docs/manual_32.md` — el manual del motor (10 capítulos + apéndices). Fuente de
  verdad para configuración, recursos, scripting, custom, CPC y toolchain.
- `docs/msc4.md` — especificación del lenguaje MSC4 (es). Actualizada a la
  implementación real; el capítulo 16 lista los bugs ya corregidos y los que
  siguen.
- `docs/msc4-scripting.md` — la misma spec en inglés.
- `src/diary.md` — ideas en curso (disparo en 4 direcciones, escaleras). **No está
  implementado**; no lo des por hecho.
- `msc4/LANGSPECS.md` — notas de diseño **abandonadas**. No es la spec.
- Apéndice C del manual: lista de lo que la v3.2 **no** tiene (cajas empujables
  completas, motor de espada, jetpac, tiles rompibles/resbalosos, escaleras,
  persistencia, monedas, motor de niveles). No busques esas cosas.

---

## 10. Errores frecuentes

1. **Activar una directiva sin implementar el hook** → el juego no enlaza.
2. **Implementar el hook sin activar la directiva** → el motor no lo llama.
3. **Olvidar `msc4i.asm` en la línea de `zcc`** → el script no se incluye.
4. **No recompilar el intérprete** tras cambiar el script → comportamiento viejo.
5. **Desajustar el conversor de mapa y `config.h`** (`RLE_MAP` vs conversor) → mapa
   corrupto.
6. **Tiles 32-47 en el mapa con `RLE_MAP 53`** → no se ven.
7. **`MAX_CERROJOS == 0`** desactiva las llaves automáticamente.
8. **Con `ACTIVATE_SCRIPTING`**, `SCR_FIN` y `PLAYER_NUM_OBJETOS` dejan de validar la
   victoria: usa `WIN GAME`.
9. **Cambiar la estructura `INERCIA`** sin regenerar el intérprete MSC4 →offsets
   corruptos.
10. **Usar `TEXT` (legacy) sin definir `TEXT_X`/`TEXT_Y`** → el juego no compila.
11. **`ENEMS_CELL_OFFSET`** vive en `speccy.h`/`cpc.h`, no en `config.h`.
12. **Los directorios `utils/` de cada juego** son copias: actualiza el del repo y
    propaga, o al revés, pero mantén la coherencia.
