# Sir Ababol 2 para Amstrad CPC (MK1 v3.2)

Conversión de Sir Ababol 2 CPC (The Mojon Twins, 2014: cpcrslib antigua, tres partes y
un programa de escenas) a MK1 v3.2 con el render sin superbuffer (`lib/cpcdirty`, modo 0
de 16 colores) en **dos cargas**: niveles 1-3 y 4-6.

**Estado: fase 3.** El juego entero en dos cargas, con cinta (`dev/sa21.cdt`,
`dev/sa22.cdt`) y disco (`dev/sa2.dsk`), y la música como la del original. Falta probarlo
todo y afinar lo que haga falta.

## Estructura

```
tools/sa2_extract.py   extracción de una vez de los recursos del original (Python)
tools/unexo.py         descompresor de exomizer 2 (el formato de cpc_UnExo)
gfx/                   pal.png, font.png, tsA/tsB/tsC.png (los tiles de cada parte),
                       sprites.png, titleA/B/C.png, cut_*.png (escenas, modo 1),
                       tiles1.spt / tiles2.spt (los pools de patrones de cada carga)
map/levelN.map         mapas de Mappy, 15 x 200 (torres de 1 x 20 pantallas)
enems/levelN.ene       enemigos (ponedor), 3 por pantalla
ogt/*.mus              canciones (formato del reproductor de MK1)
dev/                   el juego: config.h, sa2_level.h, sa2_load.h, custom.h (los
                       ganchos), sa2_play.h (el juego), custom_screens.h (las pantallas
                       y escenas), cpc_keys.h, custom_flick.h...
```

`sa2_extract.py` se ejecuta una vez sobre la carpeta del original (la que tiene
`Sir CPC 2 A`, `B`, `C` y `Cutscenes`) y la de los niveles de la versión de ZX
Spectrum (`orig/zx`: `levelN.MAP` y `sa2-lN.ene`, N = 0-4), y escribe todo lo de
arriba más `dev/wyz/instrumentos.h` y `dev/wyz/efectos.h`:

```
python3 tools/sa2_extract.py <carpeta del original> . orig/zx
```

Coge lo que entró en los binarios publicados (los `.exo`, los patrones de
`TileMapConf-sa2.asm`, los sprites de `spriteset.h`), no los PNG de las carpetas
`gfx`, que no siempre coinciden.

## Compilar

Hace falta el z88dk de MK1 con `cpcdirty` instalado (`lib/updatelib.bat` o
`lib/cpcdirty/make.bat install`) y las herramientas de MK1 en `utils`. `mapcnv` y
`ene2h` tienen que ser las de `utils/*.bas` (con `bin` y `screens=`): sus `.exe` no van
en el zip, hay que compilarlos con FreeBASIC (`fbc mapcnv.bas`, `fbc ene2h.bas`).

```
cd dev
comp_cpc.bat 1          (o comp_cpc.sh 1: Linux)  carga 1 -> sa21.sna
comp_cpc.bat 2                                    carga 2 -> sa22.sna
comp_cpc.bat 1 justcompile                        sin convertir recursos
```

`SA2FLAGS=-DSA2_DEBUG_LEVEL=n` empieza en el nivel n (0-2) de la carga.

## Decisiones

* **Niveles** (reconstruidos de las dos versiones; ver abajo):

  | Nivel | Mapa | Enemigos | Tiles |
  |---|---|---|---|
  | 1 | CPC (ZX 0) | ZX 0 | parte A |
  | 2 | CPC (ZX 1) | ZX 1 | parte A |
  | 3 | ZX 2, convertido | ZX 2 | parte A |
  | 4 | CPC (ZX 3) | ZX 3 | parte B |
  | 5 | CPC (ZX 4) | ZX 4 | parte B |
  | 6 | CPC (no está en ZX) | CPC | parte C |

* **Cargas:** carga 1 = niveles 1-3, carga 2 = niveles 4-6. Cada carga tiene un pool de
  256 patrones como mucho: la fuente (0-63, ASCII 32-95) y los tiles de sus niveles (155
  y 243 patrones).
* **Gráficos:** modo 0 de 16 colores (`MODE_0_NIBBLE`), la paleta del original, sprites
  con máscara: el pen 12 (el gris) es el transparente en los sprites; los tiles lo pueden
  usar.
* **Pantalla:** la de MK1 (64 x 192). El original usa la normal de 80 x 200 y dibuja
  sobre una rejilla que empieza 4 líneas más abajo: sus pantallas se suben 4 líneas y se
  recortan a los bytes 8-71 y las líneas 0-191. El marco cabe (se pierden las volutas de
  fuera de los lados de la caja); el área de juego queda en el carácter 1, fila 1, y el
  marcador en la fila 22.
* **Velocidad:** el original va a unos 26 fps (medido en el emulador: 25-28, con bajadas
  a 20), así que sus valores de físicas se usan tal cual a 25 fps fijos.
* **Música:** las canciones del original con la tabla de bucles que pide el reproductor
  de MK1 (8 bytes a 0 tras la cabecera: cada canal vuelve a su principio). Los efectos
  pasan de 2 a 3 bytes por paso (el ruido, a 0). Los instrumentos y la batería, iguales.
* **Escenas:** son de modo 1 con 4 tintas propias. Cada lámina es el mismo marco con una
  ventana de 128 x 72 píxeles: se guarda el marco una vez y las ventanas aparte
  (`cut_frame.png`, `cut_winN.png`). La lámina 0 no la usa el original. En una sola
  partida no hace falta el programa de escenas del original.

## El juego (`dev/sa2_play.h`)

El `engine.h`, `printer.h` y `levelmanager.h` del original, en ensamblador sobre MK1, con
estos ganchos (`config.h`):

* `PLAYER_CUSTOM_MOVE`: el muñeco entero es `player_custom_move ()`, el `move ()` del
  original 1:1 (caja de colisión x + 4..11, y + 8..15; sólo se posa con los pies en la
  mitad de arriba del tile; el salto y la espada empiezan sólo con una pulsación nueva; hielo y
  frigoababol con aceleración y rozamiento 4; los tiles que matan rebotan y quitan una
  vida). Sin el "redondeo" de la x al saltar y al pararse.
* `CUSTOM_RENDERER`: el fondo de montañas (tiles 19-21) en las filas 5-9 y los brillos
  (tile 18, uno de cada 16) sobre el tile 0, con el generador del original sembrado con
  la altura y el nivel: salen los mismos brillos que en el original.
* `ENABLE_CUSTOM_ENEMS`: los enemigos (tipos 1-4, que pasan a 17-20 para que MK1 no los
  mueva, y el jefe, tipo 6). La espada los aturde 32 frames y los da la vuelta (a
  velocidad 1); tocarlos congela al muñeco y quita una vida. El jefe persigue al muñeco
  (±8, 256 como mucho), aguanta 3 espadazos (su vida va en `x1` y su casa en `x2`, `y2`) y
  al morir se acaba el nivel (`PLAYER_NUM_OBJETOS` 1).
* `ENABLE_CUSTOM_SPRITES` (1 hueco): la espada, en tres direcciones. Más rápida que en
  el original: 6 frames en vez de 9 (rompe bloques en los frames 2-3 y da a los
  enemigos en los 2-4; el original, 3-5 y 3-6).
* `ENABLE_CUSTOM_PRE_MOVE`: el marcador (monedas con 3 cifras, llaves) y los bloques que
  se rompen (2 a la vez, 6 frames; dejan moneda o vida a veces).
* `ENABLE_CUSTOM_SCREEN_ENTER`, `ENABLE_CUSTOM_GAME_START`, `ENABLE_CUSTOM_FLICK` (no se
  sube de pantalla congelado; no hay pantallas a los lados).
* `CPC_CUSTOM_KEYS`: las teclas del original (A, D, W, N espada, M salto); con joystick,
  fuego 1 espada y fuego 2 salto.

Lo que se coge, se rompe o se abre queda escrito en el mapa (4 bits por tile) hasta el
final del nivel, como en el original. La vida y las llaves pasan de un nivel al
siguiente; las monedas también (25 dan una vida).

Diferencias con el original, a propósito: la espada es más rápida; saltar y dar espadazos piden soltar el botón (el original repetía si se mantenía pulsado); congelado, cada
pulsación de la espada cuenta para descongelarse (mantenerla pulsada no); los pinchos se miran en los puntos que tocan
(el original usaba los del choque horizontal del frame anterior), la espada no rompe
fuera de la pantalla y los bloques que se estaban rompiendo no siguen en la pantalla
siguiente.

## Las pantallas (`dev/custom_screens.h`)

Con `CUSTOM_SCREENS` el título, el fin de la partida y el final son del juego (el
`main ()`, `menu.h`, `cpcpass.h` y `cutscene.h` del original):

* **Título:** el de la parte; en la carga 1, la primera vez, antes la introducción.
* **"LEVEL n / LET'S GO!!"** con su sintonía, al empezar cada nivel (desde
  `custom_level_setup`), sobre el área de juego vacía (fondo y brillos).
* **Fin de la partida:** el recuadro " GAME OVER! " con su música, y al título.
* **Contraseña:** la de la parte A del original (izquierda, arriba, salto, espada,
  derecha, espada, salto, izquierda, arriba). La carga 1 la enseña con las flechas al
  acabar el nivel 3; la carga 2 la pide una vez, tras el primer título (si está mal,
  suena y se vuelve a pedir).
* **Escenas** (modo 1, sus 4 tintas: negro, azul cielo, cian pastel y blanco brillante):
  el marco, y por cada lámina su ventana y dos textos que se escriben letra a letra
  (la espada los pone de golpe; cualquier tecla pasa antes) y se borran línea a línea.
  La introducción usa las láminas 1-2 y los textos 1-4; el final, las 3-4 y los 5-8.
  Usan 9000-BFFF de buffer y vuelven al modo 0; la siguiente pantalla rehace la
  librería.
* **"UN PERRO":** en el nivel 6, pantalla 3, en el sitio de siempre.

La paleta de las escenas sale de las tintas de su `mysystem.h` (números hardware: el
`palette.txt` de sus gráficos no es lo que se ve).

## Cinta y disco

`comp_cpc` hace, además de la instantánea:

* **`sa2N.cdt`** (una cinta por carga, `RUN""`; en 664/6128 antes `|TAPE`):
  `system/sa2pre.asm`, un fichero del firmware que se carga en 0400, carga con
  `cpct_miniload` `system/sa2loader.asm` en BA00, que pone la pantalla en negro, carga el
  juego comprimido en zx0 por debajo de BA00, lo descomprime en 0400 y lo ejecuta. Unos
  2 minutos por carga. No hay pantalla de carga.
* **`sa2.dsk`** (cuando ya están hechas las dos cargas): `SA21.BIN` y `SA22.BIN`, con
  cabecera de AMSDOS, carga y ejecución en 0400: `RUN"SA21` y `RUN"SA22`.

Las herramientas son de MK1 (`utils/cpctape.bas`, `utils/cpcdsk.bas`: en Windows hay que
compilarlas una vez con `fbc cpctape.bas cmdlineparser.bas`, igual `cpcdsk`) y los
cargadores se ensamblan con el `z80asm` de z88dk. Los bloques `raw` y los cargadores están
probados en el emulador (carga desde la cinta hasta el título, las dos cargas); el bloque
del firmware y el disco, sólo comparando su formato con el del original.

## Música

Las canciones son las del original, que eran para el reproductor viejo de cpcwyzlib. El
de MK1 las lee de otra forma, y se convierten al extraerlas (`old_mus` en
`tools/sa2_extract.py`): tabla de bucles, un byte de volumen tras cada cambio de
instrumento, el canal de percusión mudo (el original no la toca nunca) y la tabla de
notas del original en `dev/wyz/wyz_player.h`. La tabla de instrumentos va por posición
(el original no tiene el 6). Comprobado con los registros del AY: el título y la sintonía
de "LEVEL n" salen iguales frame a frame que en el original.

## Memoria

| | Carga 1 | Carga 2 |
|---|---|---|
| Binario | 29110 bytes (0400-75B6) | 30056 bytes (0400-7968) |
| Buffer de canciones (la mayor) | 86D7 (2345) | 88EA (1814) |
| Libre | 4385 bytes | 3970 bytes |

Más los 1700 bytes que deja libres la librería por encima de sus buffers (B958-BFFF).

## La reconstrucción de los niveles

Las fuentes del CPC se perdieron y la versión de CPC se hizo deprisa. Sus niveles son
los de ZX Spectrum (5 niveles, tiles de 48) pasados a los 16 tiles de cada parte, más
uno que no está en ZX. Comparando las dos versiones (mapas pantalla a pantalla y
enemigos):

* El nivel 2 del CPC (ZX 1) perdió los enemigos de sus pantallas 0-9, el jefe entre
  ellos: por eso no se podía ganar. Se usan los de ZX.
* El nivel 2 de ZX no está en el CPC: su hueco (el nivel 5 del CPC) es otra vez el
  nivel 4 de ZX, con los tiles de la parte C, unos retoques y los mismos enemigos. Aquí
  el nivel 3 es el 2 de ZX, pasado a los tiles de la parte A (los niveles 0-2 de ZX usan
  el mismo tileset) con la correspondencia de tiles que sale de los niveles 0 y 1 de ZX
  frente a los 1 y 2 de CPC (coinciden en un 98,5% y un 99,3%; el resto son retoques a
  mano del mapa del CPC).
* El nivel 6 del CPC no está en ZX ni en la demo de 48K: se deja como está. El nivel de
  la demo tampoco es ninguno de los otros.
* En el nivel 6, pantalla 19, un enemigo empieza a medio camino (170, 106): aquí empieza
  en una esquina.
* Los niveles 5 y 6 tienen un segundo jefe en la pantalla 1 (con matar uno se gana; en
  ZX también).
