# Punchy en MK1 v3.2 (ZX y CPC modo 0): prototipo

El tester `MK1_NES/testers/02_tester_punchy` funcionando sobre MK1 v3.2 en ZX
Spectrum 48K y Amstrad CPC (modo 0, el backend CPC_DIRTY de Yun). Gráficos
provisionales convertidos automáticamente del tester. Sigue el documento de
diseño "Punchy en MK1 v3.2".

- `dev/punchy.tap` (ZX) y `dev/punchy.sna` (CPC) ya compilados.
- Controles: O P izquierda/derecha, Q salta (y sube escaleras), A baja
  escaleras, ESPACIO pega (puñetazo en el suelo, patada en el aire; sobre un
  warper, lo usa).
- Se gana con 6 enemigos derrotados (los del tester).
- Va a 3 frames por ciclo (`MIN_FAPS_PER_FRAME 3`, 16,7 Hz). Velocidades y
  contadores salen del NES escalados por 3,6 (60 / 16,7). Con 2 (25 Hz)
  también funciona: `config.h` y `custom.h` (`PN_NUM`, `PN_T16`...) tienen
  las dos tablas.

## Qué hay

| Del tester | Aquí |
|---|---|
| Jugadora 16x32 | Sprite alto nativo (5 filas en ZX, slot de 32 líneas en CPC), 16 px por encima de la caja de MK1 |
| Puñetazo, patada (frames de 24 px) | Cuerpo de 16x32 + pieza de 8x16 a su lado |
| Volteo | Por software: un buffer por actor, rellenado solo al cambiar de frame u orientación |
| Cajas altas | `PLAYER_VSTRETCH_BG/FG`, `ENEMS_VSTRETCH_FG` (8 px) |
| Punchies, shooties, lineales | Tipos 1, 2 y 3 (`pn_enems.h`), 3 golpes, retroceso, aturdidos un momento, reaparecen al volver a la pantalla |
| Cocos | 4 sprites de 8x8 |
| Warpers | Tipo 32 + pantalla destino; destino en x2, y2 del ponedor (`ene2h dsall`) |
| Escaleras, propellers, rompibles, morir y reaparecer | `pn_player.h` |
| Shines | Fuera, como se decidió |

Los números de los frames y los desplazamientos de las piezas están en
`dev/pn_frames.h`.

## Código en ensamblador

`custom.h` y lo que incluye (`pn_gfx.h`, `pn_enems.h`, `pn_player.h`) están
escritos en ensamblador dentro de las funciones: sccz80 solo pone el
envoltorio. El C original de cada función va en su comentario. Se comprobó
que se comporta igual que la versión en C: las mismas entradas por frame de
juego dan pantallas idénticas, frame a frame, en las dos máquinas (15
partidas por máquina, unas 1000-1400 frames de juego cada una, por las cuatro
pantallas, con golpes, muertes, rompibles, propellers, cocos y warpers).

Convenciones: sin parámetros (todo por globales), resultados en L (HL);
`peta_el_beeper` recibe L; `draw_coloured_tile` se llama en
`_draw_coloured_tile_do` con `__x`, `__y`, `__t` puestos. Las constantes que
usa el ensamblador son números sin paréntesis (z80asm lee `ld a, (x)` como
una lectura de memoria).

## Compilar

    dev/comp.sh zx|cpc [justcompile]        (Linux)
    dev\comp.bat zx|cpc [justcompile]       (Windows)

Usan las utilidades de MK1 (`mkts_om`, `mapcnv`, `ene2h`, `png2scr`): en Windows, copia `src/utils` del repositorio de MK1 v3.2 a `utils/` (no van en el zip). Los
sprites de 16x32, 8x16 y 8x8 los saca `mkts_om mode=sprites metasize=...`, sin
herramientas nuevas. `tools/pn_assets.py` es el generador de un solo uso que
hizo el mapa, los enemigos y los PNG provisionales a partir del tester. No forma
parte de la compilación.

Para probar empezando en otro sitio:
`PNFLAGS="-DPN_TEST_SCR=1 -DPN_TEST_X=8 -DPN_TEST_Y=8" ./comp.sh cpc justcompile`.

## Conversión

- **Mapa:** las pantallas del NES son de 16x12 y las de MK1 de 15x10. Se quita
  la columna 15 y dos filas: las 0-1, o la 0 y la 2 en la pantalla 3 para no
  perder su plataforma de arriba. El tile 13, sin usar, es el segundo frame
  del propeller, y el 19 el de "rompiéndose".
- **ZX:** monocromo, negro y blanco, con un tramado al 50% para los tonos
  medios (por luminancia). El cielo es negro. Los sprites van en negativo
  (lo oscuro sale blanco) para que se vea su contorno sobre el fondo negro.
- **CPC:** modo 0 con la paleta partida del backend. El fondo tiene 4 tintas
  (cian, negro, granate y rosa, elegidas para cubrir mejor los tiles) y los
  sprites 3, compartidas por todos (azul oscuro, rosa y blanco). Por eso los
  enemigos no conservan su color del NES (rojo, verde, azul). El arte va a la
  mitad de ancho.

## Cambios en MK1 (`mk1v32-bigsprites.patch`)

Sobre el árbol actual (con los parches de Nin Nin). Todo va detrás de
`#ifdef`, así que los juegos que no los definen compilan como antes (Nin Nin y
Yun probados con el backend nuevo: salen iguales).

- `cpc_dirty.h`, `cpc_dirty_flush.h`:
  - registros de sprite de 16 bytes;
  - con `CD_GEOMETRY`, alto (8/16/32) y ancho (8/16 px) por slot desde `cd_geom []`, con cachés alineadas y `CD_CACHE_END`;
  - recorte por arriba;
  - como mucho 8 sprites por fila de celdas (antes se pisaba la lista de la fila siguiente).
- `cpc.h`:
  - `BIG_SPRITES` dibuja al jugador y a los enemigos 16 líneas más arriba;
  - corregido el bucle de `saca_a_todo_el_mundo_de_aqui`, que avanzaba 17 bytes por sprite en vez de 16.
- `speccy.h`:
  - `BIG_SPRITES`: sprites de 5 filas y `fix_sprites` de 10;
  - `PLAYER_SPR_DX`: la x del cuerpo en los frames de puñetazo y patada.
- `engine.h`:
  - `PLAYER_VSTRETCH_BG` en el choque hacia arriba y en el horizontal, que mira 3 filas (`cm_mid_point`);
  - `PLAYER_VSTRETCH_FG` y `ENEMS_VSTRETCH_FG` en `collide_enem`;
  - el hook `PLAYER_CUSTOM_FRAME_PTR`.

Tres cosas que hay que saber:

- **Volteo en CPC.** La tabla y los buffers del volteo están en B900-BC5F. La
  pantalla de título y el marco se descomprimen sobre 9000-BFFF, así que
  `custom_screen_enter` los rehace.
- **Volteo en ZX.** La tabla está en E800 (entre el binario y los bloques de
  splib2). El binario tiene que acabar antes de E800, y `comp.sh` lo comprueba.
  No puede ir bajo 24200: ahí está la pila, que pisaba las últimas entradas de
  la tabla (las de los bytes de máscara 0xFF). De ahí salía la caja negra al
  mirar a la izquierda.
- **sccz80 y las constantes negativas.** `-a * b / c` con constantes se
  calcula como división sin signo. Por eso la escalera mandaba al jugador al
  suelo. Las constantes van siempre en positivo, con el signo aparte.
- **PLAYER_HAS_LADDERS de MK1 v3.2 no compila.** Llama a `_attr2`, que no
  existe. Las escaleras de Punchy van en el motor vertical propio.

## Medidas

| | ZX | CPC |
|---|---|---|
| Binario | 29019 bytes (24200-53219) | 20962 bytes (0400-55E2) |
| Binario con el código de Punchy en C | 32226 bytes | 23952 bytes |
| Pantalla 0 (1 enemigo) | 16,7 fps | 16,7 fps |
| Pantalla 1 (3 enemigos), quieta | 16,6 fps | 14,3 fps (13,5 en C) |
| Pantalla 1 (3 enemigos), sin pegar | ~15 fps (emulador sin contención: real algo menos) | ~14 fps |

En la pantalla más cargada no llega a los 16,7 Hz, y el juego va algo más lento. En CPC el tiempo
se lo lleva sobre todo reconstruir las cachés: el constructor generalizado
(ancho y alto variables) cuesta ~21% del tiempo, y la espera al haz otro ~21%.
Es lo primero que hay que optimizar: el constructor desenrollado por tamaño.

## Probado (zxheadless y cpcheadless)

- Título, partida, HUD, las 4 pantallas.
- Andar, saltar, subir a plataformas, subir y bajar escaleras.
- Volteo de la jugadora y de los enemigos en las dos máquinas.
- Puñetazo y patada con su pieza.
- Golpear y matar a un punchy (cuenta en KILLS).
- Romper un bloque, flotar con un propeller.
- Usar un warper (pantalla 2 → 0).
- Disparos de shooty (cocos), recorte de sprites por arriba.

**No probado:** caer en el agua y reaparecer, que te maten del todo (game
over), ganar con 6 bajas, ni jugar el tester entero.
