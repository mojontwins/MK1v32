# Yun ZX y CPC, en dos cargas

Las ocho fases de Yun (NES v5) en dos cargas de cuatro, con el motor MK1 v3.2, en ZX
Spectrum 48K y Amstrad CPC (modo 0, backend `CPC_DIRTY`). Antes eran tres cargas
(`yunload`, `yunload2`, `yunload3`); este proyecto las junta.

| Carga | Fases (nº interno) | Viene de | Mapa de la zona de nivel | Cerrojos |
|---|---|---|---|---|
| 1 | 1-4 (0-3) | yunload (0-2), yunload2 (3) | 4x6 = 24 pantallas | 2 |
| 2 | 5-8 (4-7) | yunload2 (4-5), yunload3 (6-7) | 9x3 = 27 pantallas | 4 |

La fase 5 (4, la primera de la carga 2) es cíclica (`YUN_CYCLIC_LEVEL` en `yun_level.h`,
`custom_flick.h`): cada fila del mapa de 12x2 da la vuelta, a la izquierda de la 0 está
la 11 y a la izquierda de la 12 la 23 (y al revés). En el resto de fases, izquierda y
derecha son siempre n_pant - 1 y n_pant + 1, como en MK1. `ENABLE_CUSTOM_FLICK` solo
se activa en la carga 2.

La fase 8 (7) es la de las salas oscuras (`YUN_DARK_LEVEL`, solo en la carga 2).

## Salas oscuras en ZX

- El juego enlaza con `dev/lib/splib2a.lib`: la splib2 de siempre, con un interruptor
  (`SPAttrWrite`) que hace que el actualizador no escriba atributos. Ver `dev/lib/LEEME.txt`.
- En una sala oscura el interruptor está apagado y los atributos los pone `custom.h`:
  todo el área de juego en negro sobre negro, salvo las 3x3 celdas que puede ocupar Yun,
  en 71 (blanco brillante).
- Rayo: la tinta de toda el área sube de 0 a 7, un paso por frame, se queda en 7
  `DARK_FLASH` frames y vuelve a 0 (`yun_level.h`). No se repinta ningún tile.
- Al coger la bombilla, el interruptor vuelve a encenderse y se invalida el área de
  juego: Sprite Pack pinta los colores de verdad en la siguiente actualización.
- En CPC sigue igual: solo cambian las tintas de fondo.

## Niveles en Aportador

Cada fase tiene su proyecto de Aportador: `map/levelN.apor`. Lleva el mapa, los
enemigos, los hotspots, los comportamientos y los tilesets. Exporta (CTRL + E) a los
`levelN.map` y `../enems/levelN.ene` de siempre, así que la compilación no cambia.

- Los tilesets salen de `gfx/{speccy,cpc}/apor/levelN.png`: `tsN.png` con `tsx.png` debajo, "ZX" y "CPC" (este a doble ancho). Los hotspots se pintan
  con el tile 16 + tipo. Si cambian los gráficos de los que salen,
  `python3 tools/mk_apor.py --tilesets` los vuelve a hacer; luego, en Aportador,
  **Archivo > Reimportar el tileset actual**.
- Los comportamientos están para verlos (B) y para el comprobador de bordes (CTRL + B).
  Aportador también los exporta a `map/levelN_behs.txt`, pero el juego todavía los
  coge de `dev/yun_load.h` (`lvl_behs`): si se cambia alguno, hay que cambiarlo también allí.
- `tools/mk_apor.py` hizo los proyectos a partir de los `.map` y `.ene`, y comprobó
  que Aportador exporta exactamente lo mismo: el `.map` idéntico, y en el `.ene` solo
  cambian la cabecera (las rutas) y los hotspots vacíos, que Aportador escribe en 0, 0.
  Ahora los originales son los `.apor`: el script no vuelve a escribir uno que ya existe
  (salvo con `--force`).

## Recursos

Todo va numerado por fase, de 0 a 7:

- `map/levelN.map`, `enems/levelN.ene`
- `gfx/speccy/tsN.png`, `spritesN.png`; `gfx/cpc/tsN.png`, `spritesN.png`, `pal_cnvtilesN.png`
- Por carga: `gfx/{speccy,cpc}/tiles1.spt` y `tiles2.spt` (charset y tile maps de
  sus cuatro fases), `ending1.png` y `ending2.png`, y en CPC `pal1.png` y `pal2.png`
  (tintas de título y final).
- Comunes: `title.png` (ZX 256x192, CPC 128x192), `sprites.png` (32 celdas fijas),
  `tsx.png`, `font.png`, `pal_cnvspr.png`, `pal_dark.png` y `pal_light.png`.

`ending2.png` es el de la antigua tercera carga: todavía dice "END OF THE THIRD LOAD".

## ZX: 25 Hz y Yun de 16x24

- **25 Hz** (`MIN_FAPS_PER_FRAME 2`; CPC sigue a 16,7 Hz). El juego va al 75% del tiempo
  del NES: cada frame de ZX son 1,8 frames de NES (CPC: 3,6). Por eso, respecto a CPC:
  velocidades a la mitad, aceleraciones a la cuarta parte, tiempos al doble. Física en
  `config.h`, tablas en `yun_luts.h` (un bloque para cada máquina).
- **Salto:** mismo alcance que antes (52,6 px de alto, 107,8 px de largo a toda
  velocidad; medido en el juego: 53 px), pero más lento. `PLAYER_G` y
  `PLAYER_MAX_VY_SALTANDO` están ajustados simulando el salto del motor; el empuje dura
  `PLAYER_JUMP_FRAMES` (16; antes fijo a 8 en `engine.h`). Para ir más rápido o más lento
  con el mismo alcance: velocidades por k, aceleraciones por k², tiempos entre k.
- **Enemigos lineales y plataformas** (tipos 1-4): en ZX, `custom_level_setup` divide
  sus velocidades entre 2 al cargar la fase (1 se queda en 1). Los `.ene` no cambian.
- **Yun 16x24** (`PLAYER_TALL`, solo ZX): `gfx/speccy/player.png`, 8 frames de 16x24
  en el orden de las celdas 0-7 de antes. Ahora mismo son los frames de 16x16 con 8
  filas vacías encima: **hay que dibujarlos**. Como en el NES, se pinta 8 px por encima
  de su caja de 16x16, así que colisiones, montar en burbujas, etc. no cambian.
  Sprite OR de 4 filas, `fix_sprites` de 8, `NUMBLOCKS` 83 (`SPR_VAR_BASE` 59331). Las
  celdas 0-7 de `sprites.png` ya no se convierten (`sprcnv2 ... from=8`).
- **Herramientas:** `mapcnv`, `ene2h` y `sprcnv2` tienen que ser las de `utils/*.bas`
  (con `bin`; `sprcnv2` además con `tall`, `from=` y `label=`). Sus `.exe` no van en el
  zip: hay que compilarlos con FreeBASIC (`fbc mapcnv.bas`, etc.). Los scripts paran con
  un error si encuentran las versiones viejas.
- **sprcnv2** (`utils/sprcnv2.bas`) tiene tres opciones nuevas: `tall` (celdas de
  16x24), `from=k` (empieza en la celda k sin renumerar) y `label=nombre`. **Hay que
  recompilar `sprcnv2.exe` con FreeBASIC**: el `.exe` de `utils` no las tiene. Sin
  opciones, la salida es idéntica a la de antes.
- **Rendimiento** (T-states por frame en el emulador; 25 Hz = 139.776):

  | | Frames que caben en 25 Hz |
  |---|---|
  | Plataformas, sin burbujas | 91-97% |
  | Soltando burbujas | 54-85% |

  Los frames que no caben bajan a 16,7 Hz. Los cambios de pantalla y las muertes
  siempre tardan más.
- **Salas oscuras:** la zona iluminada alrededor de Yun ahora es de 3x4. El rayo se queda
  `DARK_FLASH` = 6 frames en tinta 7, y empieza con probabilidad 1/64 por frame.

## CPC: Yun de 8x24

- `gfx/cpc/player.png`: 8 frames de 8x24 (píxeles de modo 0), en el orden de las celdas
  0-7 de `sprites.png`. Se convierte con `mkts_om mode=sprites metasize=2,3 size=8,1`
  (4 bytes x 24 líneas por frame). Las celdas 0-7 de `sprites.png` se siguen
  convirtiendo, pero no se usan: 512 bytes que se pueden recuperar vaciándolas y
  moviendo el resto de celdas.
- El backend es el de Punchy (`cpc_dirty.h`, `cpc_dirty_flush.h`), con `CD_GEOMETRY`:
  cada sprite tiene su alto (`cd_geom []` en `custom.h`: Yun 24 líneas, el resto 16).
  Una caché no puede cruzar una página de 256 bytes; `cd_init` empieza página nueva cuando
  no cabe, así que valen altos que no sean potencia de 2.
- Memoria: cachés B000-B3BF (`CD_CACHE_END` B400), `CD_SPRT` B400, `CD_DIRTY` B500-B757,
  y las celdas de sprites de cada fase en B780-BA7F (`SPR_VAR_BASE`).
- Se pinta 8 líneas por encima de su caja de 16x16 (`coy = -8`), como en ZX y en el NES.
- En el `player.png` original había dos colores fuera de la paleta de sprites: el marrón
  muy oscuro (42,24,0) pasó a negro (transparente) y el salmón (241,131,107) a naranja.

## DIRECT_TO_PLAY

Activado en `config.h`: el título es también el marco del juego, así que ya no hay
`marco.png`. Al pasar de fase, en ZX se vuelve a descomprimir el título como marco
(`cortina ()` borra toda la pantalla); en CPC solo se borra el área de juego.

## Música (CPC)

Las canciones se descomprimen en `WYZ_SONG_BUFFER` (`dev/definitions.h`), que tiene que
dejar sitio hasta 9000 para la `.mus` más grande. Ahora es `0x8B47` = 9000 - 1209
(`lala-ingame.mus`), en vez de `0x8800`: 839 bytes más para el binario.

Si cambias la música, pon `WYZ_SONG_BUFFER = 0x9000 - (bytes de la .mus más grande)`.
`comp_cpc` lo comprueba: avisa si el binario llega al buffer o si la `.mus` más grande no
cabe, y dice el valor que hace falta.

## custom.h en ensamblador

Todo `custom.h` está escrito en ensamblador, con el C original en los comentarios.
`yun_luts.h`, `yun_load.h` y `yun_level.h` son solo datos y se quedan como están.

- Los arrays de burbujas (`bub_ct`, `bub_x`...) y los de cada enemigo (`en_cap`,
  `cap_x`, `cap_y`, `en_ct`, `en_st`) van seguidos en memoria (`defs`), para llegar a
  cada campo con IX/IY + desplazamiento (`YB_*` y `YE_*`).
- `dark_set`, `coco_new`, `coco_straight` y el resto de funciones internas son rutinas
  en ensamblador que reciben lo que necesitan en registros. Las que llama el motor
  siguen siendo funciones C con el cuerpo en `#asm`.
- `custom_flick.h` no se incluye: `ENABLE_CUSTOM_FLICK` no está activado.

Comprobado frame a frame (zxheadless, cpcheadless) contra la versión en C, con entradas
aleatorias, en las ocho fases y en las dos máquinas. Los fotogramas salen iguales. La
única diferencia está en CPC, en los fotogramas en que empieza o acaba un rayo de la
fase 8: el cambio de tintas cae en otro punto del barrido porque el código es más
rápido, y el HUD no parpadea en gris. En ZX, para que la versión en C cupiera en la
prueba, se vaciaron los datos de las fases que no se probaban.

## Tamaño

| | Antes (C) | Ahora | Final | Límite | Libre |
|---|---|---|---|---|---|
| ZX carga 1 | 36404 | 32670 | 56870 | 59331 (`SPR_VAR_BASE`) | 2461 |
| ZX carga 2 | 37357 | 33462 | 57662 | 59331 | 1669 |
| CPC carga 1 | 32574 | 29744 | 0x7830 | 0x8B47 (`WYZ_SONG_BUFFER`) | 4887 |
| CPC carga 2 | 33587 | 30698 | 0x7BEA | 0x8B47 | 3933 |

"Antes" ya es con las dos cargas y `DIRECT_TO_PLAY`: en C, ninguna de las dos cargas
de ZX cabía.

## Compilar

```
dev\comp.bat 1            (ZX, carga 1: convertir + compilar + yun1.tap)
dev\comp.bat 2 justcompile
dev\comp_cpc.bat 2        (CPC, carga 2: yun2.sna)
```

Los `.sh` hacen lo mismo en Linux. Con `YUNFLAGS=-DYUN_DEBUG_LEVEL=n` se empieza en la
fase n (0-3) de la carga.

- Arriba de cada script, en el bloque de cada carga: sus fases (número, ancho y alto del
  mapa, tile de cerrojo), `SCREENS` y `LOCKS`. Tienen que coincidir con `MAP_W`, `MAP_H`
  y `MAX_LOCKS` de `yun_level.h`.
- Los datos de cada fase están en `yun_load.h`: pantalla y posición de inicio, KO, ancho
  del mapa, comportamientos, paletas.

`tools/nes2load.py` y `nes2load.txt` son de cuando se sacaron las fases 7-8 de NES (allí
eran 0 y 1); no forman parte de la compilación.

## Probado (zxheadless, cpcheadless)

- Las dos cargas en ZX y CPC: título → juego → las cuatro fases → final (forzando los
  KO).
- El marco de ZX se repinta al pasar de fase.
- Burbujas, coco, sala oscura y rayos en la fase 8 (en ZX, con el fundido de tinta).
- **No probado:** coger una bombilla en ZX (vuelta a los colores normales).
- **No probado:** jugar las fases enteras, los cerrojos y las cintas.
