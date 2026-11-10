# Nin Nin! para Amstrad CPC (MK1 v3.2, modo 1)

Conversión de Nin Nin (NES, `mojontwins/ninnin64`) a CPC 464 con MK1 v3.2 en modo 1
(256x192, 4 colores por fase), con el backend `CPC_DIRTY` (sin superbuffer) y sprites
alineados a píxel con el color 0 transparente. Las pantallas pasan de 16x12 a 15x10 con
la misma reducción que Yun. Probado en `cpcheadless`, no en hardware real.

Quedan 10 de las 12 fases: las dos de lava que sube (LAVA PIT y HELL PIT) se han
quitado enteras. Van en dos cargas de 5, un solo proyecto:

| Carga | Fases | Binario (0400-) | Libre hasta 8800 |
|---|---|---|---|
| 1 | THE BOSQUE, THE DESERT, GOOD MARSH, DE NADAL 1, CUEVA CAVE | 28099 bytes, hasta 71C3 | 5693 bytes |
| 2 | NOSHE WOOD, DE NADAL 2, MARSHY GOO, ICY DUSTER, EL CASTLER | 29183 bytes, hasta 75FF | 4609 bytes |

Cada fase cuesta entre 0,8 y 1,2 KB comprimida (mapa + enemigos + 16 celdas de sprites).
Las medidas son con la música de ejemplo de MK1 (505 bytes comprimida) y el buffer de
canciones de 2 KB en 8800: si la canción más grande es más pequeña, el buffer puede
empezar más arriba (9000 menos su tamaño) y lo que sobre es más sitio para el binario.

## Niveles en Aportador

Cada fase tiene su proyecto de Aportador: `map/levelN.apor`. Lleva el mapa, los
enemigos, los hotspots, los comportamientos y el tileset. Exporta (CTRL + E) a los
`levelN.map` y `../enems/levelN.ene` de siempre, así que la compilación no cambia.

- Los tilesets salen de `gfx/cpc/apor/levelN.png`: los 16 tiles de la fase (`nn_tsmaps`, de `work.png`) en su paleta (`palN.png`), y debajo los tiles 16-31 globales (16 recarga, 17 objeto, 18 llave). Los hotspots se pintan
  con el tile 16 + tipo. Si cambian los gráficos de los que salen,
  `python3 tools/mk_apor.py --tilesets` los vuelve a hacer; luego, en Aportador,
  **Archivo > Reimportar el tileset actual**.
- Los comportamientos están para verlos (B) y para el comprobador de bordes (CTRL + B).
  Aportador también los exporta a `map/levelN_behs.txt`, pero el juego todavía los
  coge de `dev/nn_levels.h` (`comportamiento_tiles`, por tile global): si se cambia alguno, hay que cambiarlo también allí.
- `tools/mk_apor.py` hizo los proyectos a partir de los `.map` y `.ene`, y comprobó
  que Aportador exporta exactamente lo mismo: el `.map` idéntico, y en el `.ene` solo
  cambian la cabecera (las rutas) y los hotspots vacíos, que Aportador escribe en 0, 0.
  Ahora los originales son los `.apor`: el script no vuelve a escribir uno que ya existe
  (salvo con `--force`).

## Compilar

    cd dev
    comp_cpc.bat 1          (o 2: la carga; Linux: ./comp_cpc.sh 1)
    comp_cpc.bat 1 justcompile

Sale `ninnin1.sna` (`ninnin2.sna`). `justcompile` usa los recursos de la
última conversión completa, que es de la carga que fuese. Para empezar en otra fase de
la carga: `set NNFLAGS=-DNN_DEBUG_LEVEL=2` (Linux: `NNFLAGS=-DNN_DEBUG_LEVEL=2
./comp_cpc.sh 1 justcompile`). `-DNN_BENCH` deja tres cocos y el puñetazo siempre en
pantalla, para medir.

Hace falta el MK1 con `mk1v32-all.patch` (o `mk1v32-ninnin.patch` encima de los
parches de Yun) y dos herramientas recompiladas desde los `.bas` de `utils`:
`utils\build_tools.bat` (o `build_tools.sh`) hace `rle44map_sp.exe` (opciones
`keepempty` y `maxlocks`) y `ene2h.exe` (`bin`, `screens`). El resto son las de siempre.

## Recursos

Los genera una vez `tools/nn_assets.py` (no forma parte de la compilación) desde el
repositorio de NES, las conversiones a 15x10 de `yun2mk1.py` y los gráficos dibujados
para CPC (`gfx/cpc/src_*.png`). La compilación solo usa las herramientas de `utils`.

| Fichero | Qué es | Lo convierte |
|---|---|---|
| `gfx/cpc/pal.png` | los 4 pens con que está dibujado todo: teal (0, transparente), negro, verde, blanco | (paleta de conversión) |
| `gfx/cpc/pal0-9.png` | las 4 tintas de cada fase, de los mockups | `mkts_om mode=pals` |
| `gfx/cpc/font.png` | 64 caracteres (la fuente de NES), patrones 0-63 | `mkts_om mode=chars` |
| `gfx/cpc/work.png` | los 48 tiles, patrones 64-255 | `mkts_om mode=strait2x2` |
| `gfx/cpc/sprites.png` | 12 celdas fijas, dibujadas: Nin Nin (8), explosión, coco, puño mirando a la derecha (10) y a la izquierda (11) | `mkts_om mode=sprites` |
| `gfx/cpc/sprites0-9.png` | 16 celdas de cada fase: 8 de enemigos, 6 de nadar, gota | `mkts_om mode=sprites` |
| `gfx/cpc/title.png`, `ending.png` | pantallas, dibujadas: el título lleva el marco (`DIRECT_TO_PLAY`: no hay `marco.png`); un solo final para las dos cargas (los textos irán en el código) | `mkts_om mode=superbuffer` |
| `map/level0-9.map`, `levels.txt` | mapas 15x10 con los tiles de cada fase (0-15) y su tamaño | `rle44map_sp ... keepempty maxlocks=4` |
| `enems/level0-9.ene` | enemigos y hotspots (ponedor, 3 bytes) | `ene2h ... bin= screens=24` |
| `dev/nn_levels.h` | tablas de las 10 fases: mapeo de tiles, comportamientos, inicio... | (generado) |

**Tileset.** Un solo tileset de 48 tiles; cada fase coge 16 con su tabla de
`nn_levels.h` (`MAPPED_TILESETS`). El tileset de CPC tiene el corazón, el onigiri y la
llave en 16-18 (los hotspots de MK1); en NES estaban en 45, 43 y 44, así que esos tiles
están intercambiados en las tablas.

**Paletas.** Un color por pen en todas las fases: pen 0 es el "transparente" de los
sprites (teal en los PNG), pen 1 el negro (`BLACK_PEN 1`), 2 y 3 los otros. Asignación
(a falta de confirmar): BOSQUE bosque; DESERT desierto; GOOD MARSH y MARSHY GOO pantano;
DE NADAL agua (las pantallas de superficie, las de BOSQUE); CUEVA y CASTLER cueva;
NOSHE WOOD e ICY DUSTER hielo.

**Sprites.** Nin Nin (andar, saltar) y el esqueleto son los dibujados. El esqueleto
sustituye a la abeja de NES, que es el fanty del bosque. Todo lo demás (enemigos, nadar,
explosión, gota) está convertido de `fiddle/ss.png`: fondo a pen 0, negro a pen 1, y el
resto por brillo a 2 y 3. Cada fase lleva solo sus 8 celdas de enemigos.

**Mapas.** Pantallas reducidas que el informe marca como malas (una salida que deja de
alcanzarse): GOOD MARSH 4 está arreglada a mano (`FIXES` en `nn_assets.py`); quedan por
revisar NOSHE WOOD 5 y 6 y MARSHY GOO 2, 4 y 10 (carga 2).

## Lo que hay de Nin Nin

Todo está en `dev/custom.h` y los `nn_*.h` que incluye:

- **Saltar y nadar** (`PLAYER_CUSTOM_VENG`): el salto de NES (patada fuerte, gravedad
  ligera mientras se mantiene arriba unos frames, recorte al soltar) y el nado (abajo se
  hunde, si no flota). Constantes de NES pasadas a 25 Hz en `config.h`. DE NADAL cambia
  de motor y de paleta por pantalla, con el empujón al pasar de superficie a agua.
- **Horizontal** (`PLAYER_CUSTOM_HENG`): normal, hielo y arenas movedizas.
- **Puñetazo** (FIRE): rompe bloques (las cajas de estrella dejan moneda 6 de cada 8) y
  mata enemigos. Los enemigos también mueren al tocar a Nin Nin, como en NES.
- **Monedas** (50, vida extra), cintas, arenas, pinchos con reaparición en el último
  sitio seguro, llaves y cerrojos, onigiri que acaba la fase.
- **Persistencia**: bloques rotos y monedas cogidas no vuelven (un bit por tile).
- **Puzles**: rayos que abren las vallas, caras que hay que pulsar en orden de cadenas
  para quitar los pinchos, trampas de muelle. EL CASTLER tiene además lo suyo
  (`nn_castle.h`) con las coordenadas de NES pasadas a 15x10.
- **Enemigos** (`nn_enems.h`): lineales y plataformas del motor; fanty (6), perseguidor
  (7), gota (9); y los que disparan según el nibble alto: a la vista horizontal o
  vertical, cada cierto tiempo en una dirección, o girando. Velocidades de NES x2.
- **Render del mapa** (`nn_renderer.h`): las reglas de NES que añaden el detalle
  (troncos, tierra bajo la hierba, raíces, estalactitas, hojas, arbustos...).
- **Texto de fase**: "LEVEL n" (hasta "LEVEL 10") y el nombre antes de empezar. Con
  `DIRECT_TO_PLAY` el marco es el del título, así que `custom_level_setup` quita los
  sprites y borra la zona de juego (el logo del título o la fase anterior) antes del
  texto.

## Cambios en MK1 (en el parche)

- `CPC_DIRTY` en modo 1: sprites de 16x16 alineados a píxel, el color 0 transparente con
  una máscara AND que sale de una tabla, la caché por columnas de byte (no depende de la
  paridad: moverse 4 píxeles no la reconstruye) y el desplazamiento con `rrca`/`rlca`
  parcheados para cada shift. `cd_sprites` en ensamblador (también en modo 0).
- `CUSTOM_RENDERER`: con `RLE_MAP` el mapa se decodifica entero en `map_buff` y luego
  `custom_renderer ()` puede cambiar cada tile antes de pintarlo.
- `CUSTOM_CLEAR_TILE`: lo que deja un cerrojo abierto (aquí el fondo de la fase).
- `draw_rectangle` con `CPC_DIRTY` pintaba mal (cd_char pisaba sus variables); afecta
  también al "game over" de Yun CPC.
- `rle44map_sp`: `keepempty` (las pantallas vacías se codificaban sin datos y con índice
  0, que el motor leía como si fuese la pantalla) y `maxlocks=n`.

## Código en ensamblador

`custom.h` y todo lo que incluye (`nn_renderer.h`, `nn_play.h`, `nn_enems.h`,
`nn_castle.h`), y `custom_flick.h`, están escritos en ensamblador dentro de las
funciones; el C original de cada una va en su comentario. Ahorró unos 3,8 KB por carga
(medido con el reparto anterior en tres cargas):

| Carga | En C | En ensamblador | Ahorro |
|---|---|---|---|
| 1 | 30804 bytes | 27006 bytes | 3798 bytes (12,3%) |
| 2 | 29879 bytes | 26081 bytes | 3798 bytes (12,7%) |
| 3 | 31242 bytes | 27068 bytes | 4174 bytes (13,4%) |

Se comprobó que se comporta igual que el C: con las mismas teclas por frame de juego
(`cpcheadless -gpc` en `before_vsync`, `-gk`, `-gshot`), las pantallas salen idénticas
frame a frame en las 10 fases, 3 partidas por fase de 500 a 1850 frames de juego.

De paso se corrigió `nn_set_tile`: `draw_coloured_tile` deja sus argumentos en `_x`,
`_y`, así que en C los bucles que pintaban varios tiles (abrir vallas o quitar pinchos
en `nn_clear_tiles`, el muro y la columna de EL CASTLER) se salían o se descolocaban
después del primero. Ahora `nn_set_tile` conserva `_x` y `_y`.

También en el motor: `saca_a_todo_el_mundo_de_aqui` (`cpc.h`) avanzaba 17 bytes por
sprite en vez de 16, así que no vaciaba los sprites y pisaba otros campos (la misma
corrección que en Punchy). Se notaba al pasar de fase sin el marco: los sprites de la
fase anterior seguían en pantalla.

Convenciones: sin parámetros ni locales (todo por globales), resultados en L (HL);
`peta_el_beeper` y `enems_en_an_calc` reciben L; `draw_coloured_tile` se llama en
`_draw_coloured_tile_do` con `__x`, `__y`, `__t` puestos; las demás funciones de C,
con los argumentos apilados de izquierda a derecha. Las constantes que usa el
ensamblador no pueden empezar por paréntesis (z80asm lee `ld a, (x)` como una lectura
de memoria).

## Medidas

En `cpcheadless`, a 25 fps salvo picos: pantallas normales (Nin Nin, 2-3 enemigos, el
puño) 23-25 fps; el peor caso, 8 sprites (Nin Nin, 3 enemigos, 3 cocos y el puño)
moviéndose todos, 17,5 fps. El volcado de celdas sucias es lo que más pesa.
`cpcheadless` tiene ahora `-profrange F1-F2 -profout f` para perfilar solo esos frames,
y `profsum.py` suma por funciones con el `.map`.

## Pendiente

- **Música**: en `ogt/wyz/` están el título (Menu), una del juego (Action) y game over
  convertidas con `ftm2wyz` a `.wyz`. Hay que exportarlas a `.mus` con WYZTracker,
  comprimirlas (`ogt/compress_music.bat`) y sustituir las de ejemplo de MK1 que lleva
  ahora; con solo dos o tres canciones el buffer de 8800 puede ser más pequeño.
- Las pantallas malas de la carga 2, y probar a fondo EL CASTLER.
- Pantalla de carga y cintas `.cdt`.
