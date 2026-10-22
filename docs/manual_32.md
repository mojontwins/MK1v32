# MTE MK1 v3.2

Copyleft 2011, 2026 by The Mojon Twins

## Qué es este documento

Es el manual de la **MTE MK1 v3.2**, el motor de juegos para ZX Spectrum y Amstrad CPC
que vive en este repositorio. Si ya sabes hacer cosas con la churrera (la MTE MK1
original) puedes saltarte los capítulos 1 a 5 (las herramientas son casi las mismas) e
irte directamente al 6, que es donde se configura el motor.

El capítulo 7 habla del scripting, pero **no** sustituye a la especificación del
lenguaje: para la gramática completa del lenguaje ve a [`msc4.md`](msc4.md). Aquí sólo
explicamos cómo se engancha el script con el motor de la v3.2.

Los capítulos 1 a 5 explican cómo se montan los recursos (tileset, mapa, sprites,
pantallas fijas y enemigos). Los capítulos 6 a 9 explican la configuración del motor, el
scripting, el código custom y el port a CPC. El capítulo 10 es la referencia del
*toolchain*, y los apéndices cubren la reconstrucción del compilador de scripts y la
estructura del repositorio.

> Las capturas de pantalla del manual original (Mappy, colores del conversor, marcos de
> juego) no están incluidas en este repositorio. Donde el manual original las usaba,
> aquí tienes una descripción de lo que se ve.

## Antes de empezar

Los güegos de **MTE MK1** se compilan con **splib2** (Spectrum) y con **cpcrslib** (CPC).
El entorno de desarrollo es el `z88dk10` que viene en `env/z88dk10-orig.zip`: descomprímelo
en `C:\` y ejecuta `C:\z88dk10-orig\setenv.bat` para tener el compilador de Z80 (`zcc`),
el ensamblador (`pasmo`) y las librerías en el `PATH`.

Las librerías que usa el motor están en `lib/`:

* `lib/splib2/` — la librería de sprites de Shiru. El `.lib` que se instala es
  `splib2f.lib` (que es el nombre con el que se enlaza; ver el capítulo 1), y el
  `spritepack.h` va al `include` del `z88dk10`.
* `lib/cpcrslib-mt/` — la librería del CPC. Hay dos variantes: `cpcrslib_mt.lib` y
  `cpcrslib_fg_mt.lib` (la "fg" es la que usa **Papaya** y **Sir Ababol**), más la
  cabecera `cpcrslib_mt.h`.

`lib/updatelib.bat` se encarga de copiarlas a la instalación del `z88dk10`:

```cmd
    copy splib2\splib2.lib c:\z88dk10-orig\lib\clibs\splib2f.lib
    copy splib2\spritepack.h c:\z88dk10-orig\include
    copy cpcrslib\cpcrslib.lib c:\z88dk10-orig\lib\clibs\
    copy cpcrslib\cpcrslib_mt.h c:\z88dk10-orig\include\
```

Ojo: los nombres de fichero de ese script están desfasados respecto a lo que hay
actualmente en `lib/` (`splib2f.lib` y `cpcrslib-mt`). Adapta las rutas o copia los
ficheros a mano.

### Cómo se hace un güego nuevo

El esqueleto de un güego nuevo está en **`src/`**. **Copia el directorio entero** (con
todo lo que hay dentro) a donde quieras y renómbralo con el nombre de tu juego. Si
entras en `dev/` y ejecutas `comp.bat` obtendrás el güego de ejemplo (que se llama
`lala3`).

Antes de que te copies nada, echa un vistazo a cómo se monta un güego:

```
src/
  dev/        <- el código del güego: config.h, custom.h, engine.h, ...
  enems/      <- la colocación de enemigos y hotspots (enems.ene + mapa.MAP)
  gfx/        <- los gráficos (png) del güego, en subdirectorios speccy/ y cpc/
  map/        <- el mapa (fmp de Mappy, y el .MAP exportado)
  ogt/        <- la música (OGT tracker + exports)
  script/     <- el script de MSC4 (script.spt), si lo usas
  show/       <- el resultado compilado
  utils/      <- el toolchain
  dev/system/ <- los loaders y cosas del backend
```

El proceso de crear un güego se basa en sustituir los recursos del ejemplo, cambiar la
configuración, y quizá programar un poquito en C (en `custom.h` / `extern.h`) o en el
lenguaje de scripts.

Los ficheros **más personales** de cada juego, y que en `src/` son simples esqueletos
(es decir, funciones vacías esperando a que las rellenes), son:

| Fichero | Qué es |
|---|---|
| `dev/custom.h` | Los hooks del motor (`before_vsync`, `player_custom_*`, enemigos custom, hotspots custom). |
| `dev/extern.h` | Los callbacks del script (`do_extern_action`, `textbox`, `dialog`). |
| `dev/custom_flick.h` | Los hooks de transición de pantalla (`flick_left`, `flick_right`, `flick_up`, `flick_down`). |

Estos tres ficheros están pensados para que los escribas tú, en C o con ensamblador
embebido en C. Los esqueletos de `src/` sólo tienen las funciones vacías con los
`#ifdef` de las directivas de `config.h` ya puestos, para que te limites a rellenar los
cuerpos. Los ficheros de `src/dev/msc4i.h`, en cambio, son del *framework*: no los
tocas (salvo que tu juego necesite adaptarlos, como hace **Sir Ababol**; ver el
capítulo 8).

---

# Capítulo 1 - El tileset

El tileset es la colección de archivos de 16x16 que se emplea para dibujar las pantallas
del güego. Cada tile puede tener un comportamiento diferente que se define en la
configuración del proyecto (obstáculo, plataforma, etc). **MTE MK1 v3.2** maneja *tilesets*
de 48 tiles, de los cuales, y dependiendo del formato del mapa, los 16 primeros o todos
ellos podrán ser empleados en dibujar la pantalla.

Los tilesets se dibujan siguiendo las restricciones del ZX Spectrum en un archivo .png de
256x48: 16 tiles de 16x16 por fila y 3 filas, o sea 48 tiles. En Spectrum lo guardamos en
`gfx/speccy/work.png`; en CPC, en `gfx/cpc/work.png`.

Vayas a utilizar 16 o 48 tiles para el mapa, hay algunos tiles especiales, y contando
los tiles de 0 a 47:

* El tile **14** (el penúltimo de la primera fila) será la *caja empujable* si activas
  `PLAYER_PUSH_BOXES`. Si no, puedes usarlo normalmente.
* El tile **15** (el último de la primera fila) será el *tile cerrojo* (una llave que
  abre una puerta). Lo detecta el conversor de mapas y lo guarda aparte.
* El tile **16** será la *recarga de vida* (los *hotspots* de tipo refill).
* El tile **17** representará los *objetos coleccionables*.
* El tile **18** representará las *llaves*.
* El tile **19** (el cuarto de la segunda fila) representa un tile alternativo por el que
  se sustituirá el tile 0 de forma aleatoria, para hacer bonito. Esto se puede
  desactivar con `NO_ALT_BG`, en cuyo caso puedes usarlo normalmente.

El primer tile de los *hotspots* (16) se puede cambiar con la directiva
`HOTSPOTS_FIRST_TILE`; a partir de ahí, los tres siguientes son refill, objeto y llave.
Ojo: el tile alternativo del fondo del que hablábamos **también** se desplaza con
`HOTSPOTS_FIRST_TILE`: en realidad es `HOTSPOTS_FIRST_TILE + 3`.

## El comportamiento de los tiles

El comportamiento de cada tile se define en el array `comportamiento_tiles`, al final de
`config.h`:

```c
    unsigned char comportamiento_tiles [] = {
        0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 0,
        128, 4, 4, 128, 4, 4, 4, 8, 8, 8, 8, 8, 0, 0, 0, 0,
        128, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0
    };
```

(Éste es el de **Papaya**, por si quieres ver un ejemplo real.)

Los valores que la v3.2 entiende son:

| Valor | Significado |
|---|---|
| 0 | Transitable (nada) |
| 1 | Transitable y **mata** |
| 2 | Transitable y **esconde** (con `ENABLE_BOTIBOINS`, además **rebota**) |
| 4 | **Plataforma** (sólo frena al jugador si cae encima) |
| 8 | **Obstáculo** (bloquea al jugador en todas las direcciones) |
| 128 | **Tile especial**: dispara la sección `SPECIAL TILE TOUCHED` del script |

Los valores 16, 32 y 64 **no están implementados** en la v3.2 (no hay tiles resbalosos
ni rompibles), y los comportamientos no se pueden combinar: un valor 5 (mata + plataforma)
mata, pero un 12 (plataforma + obstáculo) es un obstáculo. Ojo con esto: el motor
comprueba los bits con `&`, así que 12 se comporta como 4 y como 8 a la vez.

El bit 7 (128) se puede combinar con los demás: un comportamiento 132 es "tile especial
que además es plataforma". El motor lo comprueba con `& 128`, así que funciona.

## `ts2bin.exe` y los colores del Spectrum

Los tilesets se convierten con la herramienta `ts2bin.exe`. **MTE MK1** espera que queden
en `dev/tileset.bin`, por lo que, si abres `comp.bat` con tu editor de textos, verás que
la llamada es:

```cmd
    ..\utils\ts2bin.exe ..\gfx\speccy\font.png ..\gfx\speccy\work.png tileset.bin 7 > nul
```

`ts2bin.exe` necesita también un archivo png de 256x16 con la fuente del juego (64
caracteres, ASCII 32-95). El cuarto parámetro (un número) es la tinta por defecto que se
emplea si un cuadro de 8x8 tiene un solo color. Si se pasa "-1", el conversor elegirá
blanco para los colores oscuros o negro para los claros.

La forma de detectar los colores del Spectrum tiene ciertas particularidades. Los colores
serán detectados a partir de valores R, G o B mayores o iguales de 128, con un nivel mayor
o igual que 192 para los colores sin brillo y mayor o igual a 240 para los colores con
brillo.

**MTE MK1 v3.2** no soporta flash, pero puede utilizar el bit 7 del atributo (el de
flash) para hacer algo muy interesante: las celdas con este bit a 1 no mostrarán sprites
sobre ellas, con lo que parecerá que los sprites pasan *por detrás* de estos cuadros. Para
que el conversor levante el bit 7 hay que utilizar valores diferentes para R, G o B:
entre 128 y 159 para colores sin brillo y entre 160 y 191 para los colores con brillo.

Para guiarte puedes usar la plantilla del manual original: los colores de arriba
representan a los colores del Spectrum sin y con BRIGHT; las dos filas de abajo
representan esos mismos colores pero harán que el conversor levante el bit 7.

Por último, si vas a usar esta funcionalidad tendrás que linkar contra `splib2f` en
lugar de `splib2` modificando la linea que llama a `zcc` en `comp.bat`:

```cmd
    zcc +zx -vn churromain.c -o %game%.bin -lsplib2f -zorg=24200  > nul
```

En la práctica, la librería que se instala con `lib/updatelib.bat` ya se llama
`splib2f`, así que este cambio suele ser innecesario.

## El tileset en CPC

En CPC las cosas son distintas: no hay atributos, hay paletas de hardware. El conversor
es `mkts_om.exe`, que genera la fuente, el tileset y los sprites por separado:

```cmd
    ..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=chars greyordered in=..\gfx\cpc\font.png out=font.bin silent > nul
    ..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal_cnvtiles.png mode=strait2x2 greyordered in=..\gfx\cpc\work.png out=work.bin silent > nul
```

Los tiles del CPC se dibujan en "strait2x2" (rectos de 2x2) y las paletas se generan con
`mode=palsasassembly`, que produce ficheros de ensamblador con las secuencias de
`out (c),a` (ver el capítulo 9).

## El tileset para mappy

Mappy tiene su historia y necesita que el primer tile sea completamente negro. En esta
versión de **MTE MK1** tendrás que generar un segundo tileset específico para Mappy con
el primer tile negro si el primer tile no lo es. Puedes grabarlo como `mappy.png` en
`gfx`.

---

# Capítulo 2 - Mapeando

Para hacer el mapa puedes usar cualquier cosa siempre que te curres un conversor. Si
quieres ir por el método estándar, tendrás que hacer los mapas con **Mappy** (en
`env/mappy-mojon.zip`). Es interesante que asocies el tipo de archivo fmp a `mapwin.exe`
para más comodidad de vida.

## Definiendo nuestro mapa

Lo primero es definir el tamaño de nuestro mapa para saber con qué nos enfrentamos. Hay
que tener en cuenta que estamos en 48K y sería conveniente que nuestro mapa no emplease
demasiado, ya que a lo que ocupe el mapa habrá que sumar lo que ocupen los enemigos.

* Si vamos a usar sólo 16 tiles diferentes, un número de tile se codifica sólo con 4
  bits, con lo que en cada byte podemos almacenar dos tiles. De ese modo las pantallas,
  que son de 15 x 10 = 150 tiles ocuparían 75 bytes.

* Si vamos a usar los 48 tiles, ya no podemos meter dos tiles por byte, por lo que cada
  pantalla ocupará 150 bytes, el doble.

Los enemigos, que ya veremos más adelante, ocupan 7 bytes por pantalla si usamos
`PACKED_ENEMS` (o 9 si no), y los *hotspots* 3 bytes por pantalla.

## Creando un proyecto en Mappy

Vamos a `File->New Map`, y rellenaremos el cuadro de diálogo que aparece con los datos
de nuestro mapa. En el ejemplo, el mapa es de 8 pantallas de ancho y 5 de alto. Como cada
pantalla tiene 15x10 tiles, eso significa que mi mapa será de 8 * 15 = 120 tiles de ancho
y 5 * 10 = 50 tiles de alto. Además, nuestros tiles serán de 16 x 16 píxels. Por tanto,
rellenamos estos datos sobre nuestro mapa en el cuadro de diálogo, dejando todo lo demás
como estaba.

*(Captura: el diálogo "New Map" de Mappy con el tamaño en tiles del mapa.)*

Pulsamos OK y el programa nos dirá que deberíamos cargar un tileset. Aceptamos y vamos a
hacerle caso. Vamos a `File->Import`, lo que nos abrirá un cuadro de diálogo de selección
de archivo. Buscamos nuestro tileset preparado para Mappy (el `mappy.png` que preparamos
en el primer paso) y lo cargamos. La ventana de Mappy debería mostrarnos el tileset
cargado junto al mapa.

Por último, vamos a colocar unas guías que nos ayuden a saber dónde empieza y acaba
cada pantalla. Para ello, nos vamos a `Maptools->Dividers`, marcamos la casilla Enable
dividers y rellenamos en `Pixel gap X` y `Pixel gap Y` los valores 240 y 160, que son
las dimensiones en pixels de cada pantalla. Pulsamos OK, y con esto estamos preparados
para trabajar. Para empezar, grabamos el mapa en nuestro directorio `map` con el nombre
`mapa.fmp`. Ahora sólo hay que ponerse a colocar tiles... Y acordarnos de grabar como
`mapa.fmp` de cuando en cuando, por si las moscas.

La forma de colocar cerrojos (para ser abiertos con llaves) es sencilla: sencillamente
pon el tile del cerrojo (el 15) en el mapa, y el conversor los detectará y los almacenará
aparte. Hay que tener cuidado con una cosa: **en esta versión los cerrojos deberían ir a
ras de suelo (sobre una plataforma sobre la que podamos caminar), ya que el motor puede
no detectarlos si el muñeco no está posado y con una coordenada "justa"**.

## Exportando nuestro mapa

Para exportar nuestro mapa, sólo hay que irse a `File->Save As...` y salvarlo como
`mapa.map`, especificando el `.map` de forma explícita, en el subdirectorio `map`.

## Importando nuestro mapa

Hay dos conversores para los mapas en la v3.2, y la elección depende de lo que queramos
que ocupe el binario.

### `rle53map_sp.exe` (RLE, lo que se usa ahora)

```cmd
    ..\utils\rle53map_sp.exe in=..\map\mapa.map out=mapa.bin size=8,5 scrsize=15,10 tlock=15 mk1h=mapa.h mk1locks fixmappy > nul
```

* `in=` / `out=`: rutas del `.map` de Mappy de entrada y nombre base de los ficheros de
  salida.
* `size=8,5`: ancho y alto del mapa en pantallas.
* `scrsize=15,10`: dimensiones de la pantalla en tiles.
* `tlock=15`: el tile que se considera cerrojo.
* `mk1h=mapa.h`: genera el `mapa.h` con los `#define` y la estructura de los cerrojos.
* `mk1locks`: genera además el binario con la lista de cerrojos.
* `fixmappy`: resta 1 a cada byte del mapa (Mappy exporta los tiles numerados desde 1).
  Si tu mapa no lo exportaste con Mappy, quítalo.

Con `out=mapa.bin`, el conversor escribe en `dev/` los ficheros `mapa.map.bin` (el mapa) y
`mapa.locks.bin` (los cerrojos): el nombre base es el de `out=` y la extensión se añade
detrás.

El formato binario es: un índice de `2 * MAP_W * MAP_H` bytes con el offset de los datos
RLE de cada pantalla, y luego los datos RLE en sí. Con `RLE_MAP 53` cada byte es
`(tile & 31) | ((cuenta - 1) << 5)`: 5 bits de tile y 3 de longitud (1 a 8). Las pantallas
vacías tienen offset 0 y las pantallas idénticas comparten offset (el conversor las
detecta y las guarda una sola vez).

Las variantes del RLE son:

| Constante | Bits de tile | Bits de cuenta | Rango de tiles |
|---|---|---|---|
| `RLE_MAP 44` | 4 | 4 | 0-15, cuentas de 1 a 16 |
| `RLE_MAP 53` | 5 | 3 | 0-31, cuentas de 1 a 8 |
| `RLE_MAP 62` | 6 | 2 | 0-63, cuentas de 1 a 4 |

Cada variante tiene su conversor: `rle44map_sp.exe`, `rle53map_sp.exe` y
`rle62map_sp.exe`. Si no defines `RLE_MAP`, el mapa se guarda *packed* (dos tiles de 4
bits por byte, 75 bytes por pantalla); con `UNPACKED_MAP` se guarda sin comprimir (150
bytes por pantalla). El cambio de formato se hace en `config.h` y tiene que coincidir con
el conversor.

### Un detalle importante: los tiles 32-47

El conversor RLE sólo puede codificar un rango limitado de tiles. Con `RLE_MAP 53` (que
es lo que usan **Papaya** y **Sir Ababol**) el rango es 0-31: cualquier tile por encima
de 31 que esté en el mapa se convierte en una "decoración", y las decoraciones se
escriben en un fichero aparte (`mapa.decos.bin`) que **el motor no sabe leer**. O sea:
el motor v3.2 no puede mostrar tiles de 32 a 47 *que estén en el mapa*.

La solución es la que usan los juegos: guardar en el mapa sólo los tiles "de fondo" (0 a
31) y colocar los tiles altos (la caja de **Papaya**, las criptas de **Sir Ababol**...)
desde el script, con `SET TILE (x, y) = t`. Si el conversor te avisa de que ha
encontrado tiles fuera de rango, usa la opción `nodecos` para que los tire en vez de
generar el fichero de decoraciones.

Si necesitas tiles 32-47 *en el mapa*, usa `RLE_MAP 62` (6 bits de tile: rango 0-63) a
cambio de más bytes por cuenta (2 bits: de 1 a 4 repeticiones).

### `mapcnv.exe` (packed, el clásico)

```cmd
    ..\utils\mapcnv.exe ..\map\mapa.map mapa.h 6 5 15 10 15 packed  > nul
```

Los dos primeros parámetros indican las rutas de entrada y salida. Los dos siguientes
indican el **ancho** y el **alto** del mapa en pantallas. Los dos siguientes indican las
dimensiones de cada pantalla, que para **MTE MK1** siempre son 15 y 10. El siguiente
dígito indica el tile que debe ser detectado como cerrojo. El último parámetro `packed`
indica que debe meter dos tiles en cada byte; si tu mapa usa los 48 tiles, deberás
omitirlo.

Este es el conversor que usa el esqueleto de `src/`, y el `mapa.h` que genera es *inline*
(el array de bytes va dentro del propio `.h`), a diferencia del que genera
`rle53map_sp.exe`.

## El `mapa.h`

El conversor RLE genera un `mapa.h` con la información del mapa:

```c
    extern unsigned char mapa [0];
    #asm
        ._mapa
            BINARY "mapa.map.bin"
    #endasm

    #define MAX_CERROJOS 55
    typedef struct { unsigned char np, x, y, st; } CERROJOS;
    extern CERROJOS cerrojos [0];
    #asm
        ._cerrojos
        #if MAX_CERROJOS > 0
            BINARY "mapa.locks.bin"
        #endif
    #endasm
```

O sea, el binario del mapa se llama `mapa.map.bin` y la lista de cerrojos
`mapa.locks.bin`, y los deja en `dev/` (junto al `.h`).

`MAX_CERROJOS` es el número de cerrojos del mapa. Si vale 0, el motor desactiva las
llaves automáticamente (`churromain.c` define `DEACTIVATE_KEYS`).

---

# Capítulo 3 - Sprites

Nuestro juego empleará 8 sprites para el personaje principal y 8 más (4 enemigos con 2
frames cada uno) para los enemigos. Todos los sprites son de 16 x 16. El orden de estos
sprites en nuestro *spriteset* depende de si tenemos un juego de vista cenital o de vista
lateral.

Para las plataformas, será:

|#|Qué tiene
|---|---
|0|Personaje principal, derecha, andando, frame 1
|1|Personaje principal, derecha, andando, frame 2, o parado
|2|Personaje principal, derecha, andando, frame 3
|3|Personaje principal, derecha, en el aire
|4|Personaje principal, izquierda, andando, frame 1
|5|Personaje principal, izquierda, andando, frame 2, o parado
|6|Personaje principal, izquierda, andando, frame 3
|7|Personaje principal, izquierda, en el aire
|8|Enemigo tipo 1, frame 1
|9|Enemigo tipo 1, frame 2
|10|Enemigo tipo 2, frame 1
|11|Enemigo tipo 2, frame 2
|12|Enemigo tipo 3, frame 1
|13|Enemigo tipo 3, frame 2
|14|Enemigo tipo 4 (plataforma móvil), frame 1
|15|Enemigo tipo 4 (plataforma móvil), frame 2

Para los juegos de vista genital será:

|#|Qué tiene
|---|---
|0|Personaje principal, derecha, andando, frame 1
|1|Personaje principal, derecha, andando, frame 2, o parado
|2|Personaje principal, izquierda, andando, frame 1
|3|Personaje principal, izquierda, andando, frame 2, o parado
|4|Personaje principal, arriba, andando, frame 1
|5|Personaje principal, arriba, andando, frame 2, o parado
|6|Personaje principal, abajo, andando, frame 1
|7|Personaje principal, abajo, andando, frame 2, o parado
|8|Enemigo tipo 1, frame 1
|9|Enemigo tipo 1, frame 2
|10|Enemigo tipo 2, frame 1
|11|Enemigo tipo 2, frame 2
|12|Enemigo tipo 3, frame 1
|13|Enemigo tipo 3, frame 2
|14|Enemigo tipo 4, frame 1
|15|Enemigo tipo 4, frame 2

El motor calcula solo el cell base de cada tipo de enemigo:
`((tipo - 1) << 1) + ENEMS_CELL_OFFSET`, con `ENEMS_CELL_OFFSET` = 8 por defecto. Así, el
tipo 1 usa los cells 8 y 9, el tipo 2 los 10 y 11, etc. Ese offset vive en
`speccy.h`/`cpc.h`, no en `config.h`, y sólo tiene sentido si cambias el LUT de cells (ver
`CUSTOM_SPRITE_CELLS`, en el capítulo 6).

Para crear nuestro *spriteset*, partiremos de una imagen vacía de 256 x 32 píxels.
Tendremos una hilera con los sprites del personaje principal y sus máscaras, y otra
justo debajo con los demás sprites, con sus máscaras. Las máscaras irán intercaladas,
es decir, tendremos un sprite seguido de su máscara, seguido del siguiente sprite
seguido de su máscara, y así sucesivamente. Para pintarlas podemos usar cualquier color.
El otro color, que indica "vacío", debe ser NEGRO PURO (RGB = 0, 0, 0) para que el
conversor lo detecte sin problemas.

Grabaremos nuestro *spriteset* en `gfx` con nombre `sprites.png` (en `gfx/speccy/` para
Spectrum y en `gfx/cpc/` para CPC).

## Importanto el *spriteset*

La importación se hace con `sprcnv.exe`:

```cmd
    ..\utils\sprcnv.exe ..\gfx\speccy\sprites.png speccy\sprites.h  > nul
```

Si hemos seguido las instrucciones no necesitarás tocar nada.

Si tu juego tiene más de 16 sprites, usa `sprcnv2.exe`, que admite un parámetro con el
número de sprites:

```cmd
    ..\utils\sprcnv2.exe ..\gfx\speccy\sprites.png speccy\sprites.h 32 > nul
```

(Es lo que hace **Sir Ababol**, que tiene 32 cells.)

## Sprites extra

Hay tres ficheros de sprites adicionales, cada uno con su propio conversor:

* `sprites_extra.png`: los sprites que quieras añadir (2 de momento). Se convierten con
  `mkts_om.exe ... mode=sprites ... out=sprites_extra.bin max=2`.
* `sprites_bullet.png`: el sprite de la bala, de 8x8. `max=1 metasize=1,1`.
* `sprites_sword.png`: el sprite de la espada, de 8x8. `max=4`. Ojo: **la v3.2 no tiene
  motor de espada**; este sprite sólo se usa si escribes tú el código.

---

# Capítulo 4 - Pantallas fijas

En un juego de **MTE MK1** hay cuatro pantallas fijas. Una se almacena tal cual, en
formato Spectrum: la pantalla de carga. Las otras tres se comprimen y forman parte del
binario: se trata de la pantalla de título, el marco y la pantalla con la escena final.
Cuanto más compleja sea una pantalla, más ocupará comprimida, por lo que una forma de
liberar memoria es simplificar las pantallas que van en el binario.

Aunque sea obvio decirlo, las pantallas deben ocupar 256x192 pixels y respetar las
restricciones del ZX Spectrum.

## La pantalla de carga

Si no modificas el paquete, la pantalla de carga se almacena como un archivo binario con
formato Spectrum, esto es, 6912 bytes, por lo que no importa lo compleja o simple que sea.
La pantalla de carga debe almacenarse en `gfx` como `loading.png` (y se convierte en
`loading.bin`).

## La pantalla de título

La pantalla de título se presentará a modo de menú y sobre ella sonará la música de
presentación. Deberían salir título, créditos y un menú con tres opciones 1: Keyboard,
2: Kempston, 3: Sinclair.

La pantalla de título se genera con `draw_text (12, 12, ..., "MK1 V3.2")` en
`pantallas.h` y con `select_controls()` (en `speccy.h` / `cpc.h`), que espera a que
pulgues 1, 2 o 3 para elegir el tipo de joystick.

Esta pantalla debe guardarse en `gfx` como `title.png`.

## El marco

El marco se mostrará alrededor del área de juego mientras jugamos. Se utiliza para
contener los marcadores. En el tendremos que hacer sitio para los diferentes valores
del juego, dependiendo siempre de la configuración del motor. Los espacios donde van los
marcadores numéricos deben estar alineados a carácter y dejar sitio para dos dígitos.

También tiene que haber espacio para la ventana de juego, igualmente alineado a carácter.
Esta ventana ocupa 240x160 píxels (o 15x10 tiles).

*(Captura: el marco de juego de **Papaya**, con los marcadores de vida, objetos y llaves
en las dos primeras filas y el área de juego en las filas 4 a 23.)*

Esta pantalla debe guardarse en `gfx` como `marco.png`.

## Título y marco

Para ahorrar memoria, **MTE MK1** permite juntar título y marco en una sola pantalla. El
título y el menú se dibujarán dentro del area de juego. Si eliges esta configuración,
graba la pantalla combinada de título y marco en `gfx` como `title.png` y deja
`marco.png` en negro, y define `DIRECT_TO_PLAY` en `config.h` (con `DIRECT_TO_PLAY`, la
pantalla de título *es* el marco del juego y no se vuelve a mostrar al empezar una
partida nueva).

## La pantalla del final

La pantalla del final se mostrará cuando el jugador haya conseguido su objetivo final en
el juego. Debe guardarse en `gfx` como `ending.png`.

## Convirtiendo a formato Spectrum

Para ello, desde `comp.bat` se utiliza `png2scr.exe` y `zx0.exe` para comprimir:

```cmd
    ..\utils\png2scr.exe ..\gfx\speccy\title.png ..\gfx\speccy\title.scr  > nul
    ..\utils\zx0.exe ..\gfx\speccy\title.scr titlec.bin > nul
```

El resultado son `titlec.bin`, `marcoc.bin` y `endingc.bin`, que se incrustan en el
binario desde `pantallas.h`.

En CPC se usa `mkts_om.exe ... mode=superbuffer` y luego `zx0.exe` también.

---

# Capítulo 5 - Enemigos y *hotspots*

En este capítulo usaremos `ponedor.exe` para colocar en cada pantalla de nuestro juego
hasta tres enemigos y un *hotspot*, que no es más que una casilla donde poner un objeto
coleccionable o una llave, o donde aparecerán las recargas.

## Los enemigos

En **MTE_MK1 v3.2** los enemigos también se refieren a las plataformas móviles. Los
enemigos tienen asociados una serie de valores, entre ellos un **tipo**, que define el
comportamiento y de forma indirecta el gráfico con el que se pinta. El motor sólo sabe
gestionar por su cuenta estos tipos:

| Tipo | Comportamiento |
|---|---|
| 1, 2, 3 | Enemigo **lineal**: se mueve de un punto a otro rebotando en sus límites. Todos se comportan igual; lo que cambia es el gráfico. |
| 4 | **Plataforma móvil** (sólo en vista lateral): el jugador puede subirse y es arrastrado por ella. |

Los gráficos de los enemigos se eligen automáticamente por su tipo (ver el capítulo 3).

Cualquier otro tipo (5, 6, 15, 32...) es un **enemigo custom**: el motor lo dibuja
vacío (`spr_empty`) y es tu código en `custom.h` el que lo implementa. **Sir Ababol** usa
los tipos 5 a 8 para sus peces, y **Papaya** usa el tipo 15 para el guarda. Los
enemigos de tipo *fanty* (los del motor de *Random Respawn*, ver más abajo) no se colocan
en el mapa: aparecen solos cuando muere un enemigo.

### Colocación y edición: `ponedor.exe`

El editor de enemigos es `ponedor.exe`, que vive en `utils/`. Se puede lanzar arrastrando
el fichero `enems.ene` sobre `enems/ponedor.bat`, o desde la línea de comandos:

```
    ponedor.exe file.ene
```

* Con `file.ene` como único argumento, abre el fichero para editarlo.
* `ponedor.exe new out=file.ene map=file.map tiles=file.png|bmp [adjust=n] size=w,h [scrsize=w,h] [nenems=n] [x2]` crea un fichero nuevo.

Los valores por defecto del `ponedor` son: pantalla de 16x12 tiles y 3 enemigos por
pantalla. Para **MTE MK1** la pantalla es de 15x10 tiles, así que hay que darle
`scrsize=15,10` explícitamente (y `size=w,h` con el tamaño del mapa en pantallas).

El botón `H` de la barra de botones exporta un `enems.h` al estilo "Churrera MK1", que
**no** es el formato que usa este motor. El flujo moderno es exportar el `.ene` con el
`ponedor` y convertirlo con `ene2h.exe` (ver más abajo).

**Ojo con el formato**: el fichero `.ene` debe guardarse en el formato *legacy* de 2 bytes
por hotspot. El `ponedor` lo detecta automáticamente en los ficheros originales, pero si
creas uno nuevo hay que especificarlo. El modo de compatibilidad se comprueba examinando
la parte superior de la ventana: si aparece `2b`, el `.ene` está en formato *legacy*; si
aparece `3b`, en el formato actual. Para cambiar de uno al otro basta con pulsar la tecla
`L`.

Los tipos de enemigo se introducen **en hexadecimal** en el diálogo. El campo `Attr` es
la velocidad en píxeles por frame (potencias de 2: 1, 2, 4).

### De `.ene` a `enems.h`: `ene2h.exe`

```cmd
    ..\utils\ene2h.exe ..\enems\enems.ene enems.h 2bytes compacted indexed > nul
```

* `2bytes`: el formato legacy de 2 bytes por hotspot.
* `compacted`: el layout "packed" de los enemigos (`PACKED_ENEMS`), que guarda `x1,y1` y
  `x2,y2` en dos bytes con nibbles en vez de en cuatro bytes. Ahorra 2 bytes por enemigo.
* `indexed`: genera el array `enoffs_index[]`, con el índice del primer enemigo de cada
  pantalla (`INDEXED_ENEMS`). Con esto, las pantallas sin enemigos no ocupan nada.

Los tres banderas corresponden a las tres directivas de `config.h` que hay que activar a
la vez (`PACKED_ENEMS`, `FIXED_ENEMS_LIMITS` e `INDEXED_ENEMS`).

Otras opciones de `ene2h.exe`: `empty=W,H,N` genera un `.ene` nuevo con todo a cero
(WxH pantallas, N enemigos), `dslight` / `dsall` ordenan los puntos límite de los
enemigos, y `CMD=C,T,O,V` genera un `enems.h` con los comandos de customización.

El `enems.h` que genera lleva en la cabecera un comentario que habla de un motor
antiguo; ignóralo, el resto del fichero es válido.

El `enems.h` generado contiene la estructura `MALOTE`:

```c
    typedef struct {
        unsigned char x, y;
        unsigned char xy1, xy2;        // PACKED_ENEMS
        signed char mx, my;
        signed char t;
        #if ((defined PLAYER_CAN_FIRE || defined ENABLE_SWORD) && ENEMS_LIFE_GAUGE > 1) || defined FORCE_ENEMS_LIFE
        unsigned char life;
        #endif
    } MALOTE;
```

Sin `PACKED_ENEMS`, el `ene2h` genera en su lugar los cuatro bytes `x1, y1, x2, y2`. Y los
*hotspots* se guardan en una estructura aparte:

```c
    typedef struct {
        unsigned char xy, tipo, act;
    } HOTSPOT;
```

(`xy` es el par de coordenadas empaquetado en nibbles; `tipo` es 1, 2 o 3; `act` vale 1
si el hotspot está activo, 0 si ya se recogió y 2 si se convirtió en recarga.)

### El comportamiento de los enemigos lineales

El enemigo se mueve a velocidad `mx`/`my` (píxeles por frame) y **rebota** entre los dos
puntos límite (`x1,y1` y `x2,y2`). Si los dos puntos no están alineados a un eje, el
enemigo traza una diagonal "escalonada", porque los rebotes en X y en Y son independientes.

Si se define `PLAYER_PUSH_BOXES` o `ENABLE_CUSTOM_ENEMS`, los enemigos además colisionan
con el escenario: se paran ante los obstáculos y ante los tiles que matan (según la
máscara `ENEMIES_COLLIDE_MASK`, que vale 9 por defecto: obstáculo + mata).

### Random Respawn Fanties

Con `RANDOM_RESPAWN` se activa un motor en el que, cada vez que un enemigo muere, puede
aparecer un *fanty* (un enemigo volador) en su lugar. La probabilidad es de 1/32 por frame
y por enemigo muerto. El fanty **acelera hacia el jugador** con `FANTY_A` hasta una
velocidad máxima de `FANTY_MAX_V`, y **se aleja** si el jugador está escondido. Se puede
destruir con `FANTIES_LIFE_GAUGE` disparos.

## Los *hotspots*

Cada pantalla tiene **un** *hotspot* como máximo. Hay tres tipos:

| Tipo | Tile | Efecto |
|---|---|---|
| `HOTSPOT_TYPE_OBJECT` (1) | 17 | +1 objeto (y sonido 7) |
| `HOTSPOT_TYPE_KEY` (2) | 18 | +1 llave (y sonido 8) |
| `HOTSPOT_TYPE_REFILL` (3) | 16 | +`PLAYER_REFILL` de vida (y sonido 9) |

El primer tile de los hotspots se define con `HOTSPOTS_FIRST_TILE` (16 por defecto), de
modo que con `HOTSPOTS_FIRST_TILE 32` los hotspots serían 32 (recarga), 33 (objeto) y 34
(llave).

Con `ONLY_ONE_OBJECT`, el jugador sólo puede llevar un objeto a la vez: si ya lleva uno,
el hotspot **no** desaparece y el script no se ejecuta.

Con `DEACTIVATE_REFILLS` no hay recargas. Y ojo: si **no** se define
`DEACTIVATE_REFILLS`, cada vez que el jugador recoge un objeto, ese hueco tiene un 25 %
de probabilidad de mostrar una **recarga** en su lugar (el "refill aleatorio" clásico de
MK1, por el que las recargas van apareciendo donde han quedado los objetos).

## Los marcadores

Los marcadores del HUD se posicionan con coordenadas de **caracteres** (no de píxeles):

```c
    #define VIEWPORT_X   1     // Esquina superior izquierda del área de juego
    #define VIEWPORT_Y   4
    #define LIFE_X      20
    #define LIFE_Y       2
    #define OBJECTS_X    5
    #define OBJECTS_Y    2
    #define KEYS_X      17
    #define KEYS_Y       0
    #define KILLED_X    20
    #define KILLED_Y    21
    #define ITEM_X      28
    #define ITEM_Y       1
    #define ITEM_FLAG    0     // El flag que se muestra como "el objeto que llevas"
```

* El marcador de objetos (`OBJECTS_X`) sólo se dibuja si no defines `DEACTIVATE_OBJECTS`.
  Con `REVERSE_OBJECT_COUNT` muestra los que *quedan* en vez de los que llevas. Con
  `ONLY_ONE_OBJECT` y `OBJECTS_ICON_X` se dibuja un icono en vez de dos dígitos.
* El marcador de llaves (`KEYS_X`) sólo se dibuja si defines `KEYS_X` y no
  `DEACTIVATE_KEYS`.
* El marcador de muertes (`KILLED_X`) sólo se dibuja con `PLAYER_KILLS_ENEMIES`.
* El "objeto que llevas" (`ITEM_X`/`ITEM_Y`) se dibuja a partir del flag `ITEM_FLAG`, y
  **sólo si el scripting está activado**. Es el mecanismo que usa `GET ITEM SET` (ver el
  capítulo 7).

---

# Capítulo 6 - Configurando el güego

El comportamiento básico del juego se define en el archivo `config.h` que hay situado en
`dev`. Luego podremos tunear mediante el scripting o el código inyectable de
`custom.h`, pero hay que empezar por el principio.

## I. Configuración general

En la primera sección configuramos el comportamiento general del juego: las dimensiones
del mapa, la pantalla de inicio, la pantalla de final (si aplica), la posición del muñeco
al inicio, el número de objetos que hay que recoger, etcétera.

```c
    //#define MIN_FAPS_PER_FRAME    2       // Limits the max # of fps to 50/N
    #define MAP_W                   8       //
    #define MAP_H                   5       // Map dimmensions in screens
    #define SCR_INICIO              0       // Initial screen
    #define PLAYER_INI_X            1       //
    #define PLAYER_INI_Y            2       // Initial tile coordinates
    //#define SCR_FIN              99       // Last screen. 99 = deactivated.
    //#define PLAYER_FIN_X         99       //
    //#define PLAYER_FIN_Y         99       // Player tile coordinates to finish game
    #define PLAYER_NUM_OBJETOS     99       // Objects to get to finish game
    #define PLAYER_LIFE              5      // Max and starting life gauge.
    #define PLAYER_REFILL            1      // Life recharge
```

* `MIN_FAPS_PER_FRAME` coloca un limitador de frames. El valor que le asignes será el
  máximo número de frames que dejará pasar por cuadro de juego. El ZX Spectrum funciona
  a 50 fps, por lo que, por ejemplo, un valor de 2 limitaría el juego a 25 fps, y uno de 3
  a 16.6 fps. Obviamente si hay mucha carga en una escena el frame rate del juego
  bajará.

* `MAP_W` y `MAP_H` definen el tamaño del mapa en pantallas. Deben ser los mismos valores
  que usas en `comp.bat` como parámetros del conversor de mapas.

* `SCR_INICIO` es el número de pantalla en la que se empieza a jugar, contando de
  izquierda a derecha y de arriba abajo, empezando por 0. `PLAYER_INI_X` y `PLAYER_INI_Y`
  son las coordenadas de inicio a nivel de tiles.

* `SCR_FIN`, `PLAYER_FIN_X` y `PLAYER_FIN_Y` definen la pantalla y las coordenadas en
  las que el juego se da por terminado. **Cuidado**: si activas `ACTIVATE_SCRIPTING`,
  estas condiciones de victoria **desaparecen** (ver más abajo) y sólo gana quien
  ejecute `WIN GAME` en el script.

* `PLAYER_NUM_OBJETOS` especifica cuántos *hotspots* de tipo 1 tendremos que coleccionar
  para ganar. Con el scripting activado, esta condición también desaparece.

* `PLAYER_LIFE` es la cantidad de vida con la que empezamos a jugar, `PLAYER_REFILL` es
  cuánto se recarga la vida al coger una recarga. **Ojo**: la vida es un entero *con
  signo*, y el juego se acaba cuando `life < 0`, así que en realidad se puede perder
  `PLAYER_LIFE + 1` vidas.

* `MAX_ENEMS` y `MAX_FLAGS` **no** están en `config.h`: se definen en `churromain.c` (3 y
  32 por defecto). `MAX_FLAGS` es el tamaño del array de flags del script, y `MAX_ENEMS`
  el número de enemigos por pantalla.

## II. Tipo de motor

Esta sección se usa para definir el comportamiento del motor. Muchas directivas están
relacionadas, y otras son excluyentes entre sí.

### Directivas miscelaneas

```c
    //#define PLAYER_PUSH_BOXES                 // If defined, tile #14 is pushable
    #define DIRECT_TO_PLAY                      // If defined, title screen is also the game frame.
    //#define DEACTIVATE_KEYS                   // If defined, keys are not present.
    //#define FIRE_TO_USE_KEY                   // If defined, press fire to use key.
    //#define DEACTIVATE_OBJECTS                // If defined, objects are not present.
    //#define ONLY_ONE_OBJECT                   // If defined, only one object can be carried at a time.
    //#define OBJECT_COUNT              1       // Defines which FLAG will be used to store the object count.
    //#define DEACTIVATE_EVIL_TILE              // If defined, no killing tiles (behaviour 1) are detected.
    //#define PLAYER_BOUNCES                    // If defined, collisions make player bounce
    #define PLAYER_FLICKERS                     // If defined, collisions make player flicker instead.
    //#define ENABLE_BOTIBOINS                  // If defined, enable botiboins on beh 2.
```

* `PLAYER_PUSH_BOXES`: el tile 14 se puede empujar. Ojo: para que se pueda empujar, su
  comportamiento en `comportamiento_tiles` debe ser 8 (obstáculo), porque el motor sólo
  intenta empujar cuando el jugador choca con un muro.

* `DIRECT_TO_PLAY`: la activamos cuando compartimos título y marco en la misma pantalla.
  La pantalla de título no se vuelve a mostrar al empezar la partida.

* `DEACTIVATE_KEYS`: si nuestro juego no usa llaves ni cerrojos, activamos esta directiva
  para ahorrar espacio. Se activa automáticamente si `MAX_CERROJOS` es 0.

* `FIRE_TO_USE_KEY`: la llave sólo se usa si además se pulsa FIRE.

* `DEACTIVATE_OBJECTS`: si nuestro juego no usa objetos coleccionables, activamos esta
  directiva para ahorrar espacio.

* `ONLY_ONE_OBJECT`: activa el modo en el que el jugador sólo puede llevar un objeto. Al
  recoger un objeto, `player.objs` se pone a 1 y no podremos coger más.

* `OBJECT_COUNT`: indica qué flag se usa para contar los objetos (lo dibuja el motor como
  un contador de dos dígitos).

* `DEACTIVATE_EVIL_TILE`: si nuestro juego no emplea tiles que matan, puedes ahorrar
  espacio activándola.

* `PLAYER_BOUNCES`: el jugador rebota si le da un enemigo.

* `PLAYER_FLICKERS`: el jugador parpadea durante 50 frames si le da un enemigo. (En la
  v3.2 el número de frames está fijado a 50 en el motor; la macro no toma valor.)

* `ENABLE_BOTIBOINS`: el comportamiento 2 (esconde) hace además que el jugador rebote y
  suene un beep.

### Motor de disparos

En esta versión de **MTE MK1** sólo funciona en vista lateral.

```c
    //#define PLAYER_CAN_FIRE                   // If defined, shooting engine is enabled.
    //#define PLAYER_BULLET_SPEED      8       // Pixels/frame.
    //#define MAX_BULLETS             3       // Max number of bullets on screen. Be careful!.
    //#define PLAYER_BULLET_Y_OFFSET  4       // vertical offset from the player's top.
    //#define ENEMS_LIFE_GAUGE        5       // Amount of shots needed to kill enemies.
    //#define PLAYER_AX_RECOIL       128      // Push player back a bit when firing
    //#define FIRING_DRAINS_LIFE              // If defined, firing drains life (oi!)
    //#define FIRING_DRAIN_AMOUNT     2       // what to substract when firing.
```

* `PLAYER_CAN_FIRE` activa el motor de disparos. **Ojo**: activa también el cambio de la
  tecla de salto: con disparos, el salto es la tecla "arriba" sola; sin disparos, hay que
  pulsar "arriba" **y** "fuego" a la vez.

* `PLAYER_BULLET_SPEED` define la velocidad de los proyectiles en píxeles por frame.

* `MAX_BULLETS` define el máximo número de balas en pantalla. Hay que tener cuidado
  porque a más balas, menos velocidad y más memoria dinámica tendremos que reservar (ver
  el capítulo 8).

* `PLAYER_BULLET_Y_OFFSET`: número de píxels desde el borde superior del jugador desde el
  que se dispara la bala.

* `PLAYER_AX_RECOIL`: si se define, se añadirá esta aceleración al eje horizontal en
  sentido contrario al sentido en el que mire el jugador cada vez que se dispare
  (retroceso del arma).

* `ENEMS_LIFE_GAUGE` especifica el número de veces que hay que darle con un disparo a un
  enemigo para matarlo. Con esta opción, la estructura del enemigo crece un byte.

* `FIRING_DRAINS_LIFE` (opcional): si se define, disparar resta vida (como en **Cheril
  the Goddess**). `FIRING_DRAIN_AMOUNT` define cuánta.

### Random Respawn Fanties

```c
    //#define RANDOM_RESPAWN                    // If defined, automatic flying enemies spawn on killed enemies
    //#define FANTY_MAX_V              256     // Flying enemies max speed.
    //#define FANTY_A                   12     // Flying enemies acceleration.
    //#define FANTIES_LIFE_GAUGE        10     // Amount of shots needed to kill flying enemies.
```

### Enemigos custom

```c
    #define ENABLE_CUSTOM_ENEMS                 // Create your custom enemies in `custom.h`
```

Ver el capítulo 8.

### Scripting

```c
    #define ACTIVATE_SCRIPTING                  // Activates msc scripting and flag related stuff.
    #define SCRIPTING_DOWN                      // Use DOWN as the action key.
    //#define SCRIPTING_KEY_M                    // Use M as the action key instead.
                                                // If none are selected, FIRE is used.
    //#define NO_INDEXED_SCRIPTING               // You aren't using * SCREEN * sections.
    #define ENABLE_ENCODED_TEXT                 // Add support for TEXT BOX
    #define ENABLE_DIALOG                       // Add support for DIALOG
```

Ver el capítulo 7.

### Vista cenital o vista lateral

```c
    //#define PLAYER_MOGGY_STYLE                 // Enable top view.
    //#define FAKE_SIDE_VIEW                     // For gravity 0 games! Needs PLAYER_MOGGY_STYLE enabled.

    #define PLAYER_HAS_JUMP                     // If defined, player is able to jump.
    //#define PLAYER_HAS_JETPAC                  // If defined, player can thrust a vertical jetpac
    //#define PLAYER_KILLS_ENEMIES               // If defined, stepping on enemies kills them
    //#define PLAYER_MIN_KILLABLE        3       // Only kill enemies with id >= PLAYER_MIN_KILLABLE
    #define SIMPLE_PLATFORMS                   // No type 4 tiles or no type 8s over type 4s.
    #define AVOID_PLATFORM_HOP                  // Only works if SIMPLE_PLATFORMS is DISABLED!
    //#define JUMP_DOWN_PLATFORM                 // Use down on type 4s to jump down.
    #define DEACTIVATE_MOVING_PLATFORMS          // No moving platforms, type 4s are normal linear.
```

* `PLAYER_MOGGY_STYLE`: vista cenital (o "genital", que tiene más gracia). En vista
  cenital no hay gravedad: arriba y abajo se mueven como izquierda y derecha, y el jugador
  mira en cuatro direcciones.

* `FAKE_SIDE_VIEW`: para juegos sin gravedad, pero manteniendo los valores de orientación
  del jugador de la vista lateral. Requiere `PLAYER_MOGGY_STYLE`.

* `PLAYER_HAS_JUMP`: el jugador puede saltar. Para saltar hace falta estar apoyado en
  algo (`possee`) o sobre una plataforma móvil (`gotten`).

* `PLAYER_HAS_JETPAC`: **no está implementado** en la v3.2 (el código es un comentario).
  No lo actives.

* `PLAYER_KILLS_ENEMIES`: el jugador puede matar enemigos saltando encima.
  `PLAYER_MIN_KILLABLE` y `PLAYER_MAX_KILLABLE` acotan qué tipos se pueden matar así.
  Ojo: `PLAYER_MIN_KILLABLE` no tiene por qué ser un número constante: **Sir Ababol** lo
  define como `(flags[31])` para que el juego pueda "desbloquear" la ability de matar
  enemigos en tiempo de ejecución.

* `SIMPLE_PLATFORMS`: no hay tiles de tipo 4, o no hay tiles de tipo 8 encima de tiles de
  tipo 4. Simplifica las colisiones.

* `AVOID_PLATFORM_HOP`: en vista lateral, el jugador no "salta" al subir a una plataforma:
  sólo se posa en ella si lleva al menos un frame de velocidad vertical suficiente. Sólo
  funciona si `SIMPLE_PLATFORMS` está desactivada.

* `JUMP_DOWN_PLATFORM`: se puede usar "abajo" para bajar de las plataformas.

* `DEACTIVATE_MOVING_PLATFORMS`: las plataformas móviles (tipo 4) se tratan como tiles
  normales.

### Custom

```c
    //#define PLAYER_CUSTOM_VENG                 // Replace Vertical axis for your own
    //#define PLAYER_CUSTOM_HENG                 // Replace Horizontal axis for your own
    #define PLAYER_CUSTOM_FRAME                 // Replace frame number selection for your own
    //#define PLAYER_CUSTOM_BG_HIT               // Implement your own bounce.
    //#define ENABLE_CUSTOM_HOTSPOTS             // Add your own hotspots!
    //#define ENABLE_CUSTOM_FLICK                // define flick_* in custom_flick.h, return 0 to override flick
    //#define CUSTOM_SPRITE_CELLS                // Use your own sprite cell luts from custom_sprite_cells.h
    //#define ENEMS_CELL_OFFSET         8        // If the above is defined, this is the 1st cell for the enemies.
```

Ver el capítulo 8.

### Formato de los enemigos

```c
    //#define PACKED_ENEMS           // Store x1, y1, x2, y2 as two XY bytes.
    //#define FIXED_ENEMS_LIMITS      // If x1, y1, x2, y2 NEVER change.
    //#define INDEXED_ENEMS           // Don't include empty enems & use index
```

Estas tres directivas tienen que coincidir con las opciones que le pases a `ene2h.exe`
(`compacted` e `indexed`), y su efecto es puramente de tamaño: `PACKED_ENEMS` ahorra 2
bytes por enemigo, e `INDEXED_ENEMS` hace que las pantallas sin enemigos no ocupen nada
en el binario.

### Depuración

```c
    //#define DEBUG_NAVIGATOR         // s + directions (ZX) or x + directions (CPC) to nav map
```

**Desactívalo antes de publicar**: con `DEBUG_NAVIGATOR` se puede saltar de pantalla en
pantalla y, además, el jugador gana objetos extra (`player.objs ++`) al pulsar FIRE.

### Específico de CPC

```c
    #define BLACK_PEN                1    // For CPC, which pen is black?
    //#define AUTO_SPLIT                     // For CPC, but hud must be 32 raster on top and MODE 1
    //#define ALWAYS_SPLIT                   // Useful for DIRECT_TO_PLAY
    //#define NO_PAL_MAP                     // For CPC, with AUTO_SPLIT, only one pal (my_inks)
```

Ver el capítulo 9.

## III. Configuración de la pantalla

```c
    #define VIEWPORT_X                   1       //
    #define VIEWPORT_Y                   4       // Viewport character coordinates
    #define LIFE_X                      20       //
    #define LIFE_Y                       2       // Life gauge counter character coordinates
    #define OBJECTS_X                    5       //
    #define OBJECTS_Y                    2       // Objects counter character coordinates
    //#define OBJECTS_ICON_X              9       //
    //#define OBJECTS_ICON_Y             21       // Objects icon character coordinates (use with ONLY_ONE_OBJECT)
    //#define KEYS_X                     17       //
    //#define KEYS_Y                      0       // Keys counter character coordinates
    //#define KILLED_X                   20       //
    //#define KILLED_Y                   21       // Kills counter character coordinates
    #define ITEM_X                      28       //
    #define ITEM_Y                       1       //
    #define ITEM_FLAG                    0       // Show item @ flag
    //#define TEXT_X                      4       //
    //#define TEXT_Y                      3       // To use TEXT from the script
    //#define TEXT_A                     71       // (Speccy only) text atribute.
```

`VIEWPORT_X` y `VIEWPORT_Y` son las coordenadas **en caracteres** de la esquina superior
izquierda del área de juego. Con `VIEWPORT_X 1` y `VIEWPORT_Y 4`, el área de juego ocupa
las columnas 1..30 y las filas 4..23, dejando las cuatro primeras filas para el HUD.

`TEXT_X`, `TEXT_Y` y `TEXT_A` son para el comando `TEXT` del script (que dibuja una línea
de texto en el HUD). En Spectrum, `TEXT_A` es el atributo del texto; en CPC no se usa.
Ojo: la rutina `draw_line_of_text` **sólo se compila** si defines `TEXT_X` (y
`ACTIVATE_SCRIPTING`), así que si usas `TEXT` en el script sin definirlas, el juego no
enlazará. Como `TEXT` es una opción "legacy", lo normal es usar `TEXT BOX` y no definir
nada de esto.

## Efectos gráficos

```c
    //#define USE_AUTO_SHADOWS                    // Automatic shadows made of darker attributes
    //#define USE_AUTO_TILE_SHADOWS                // Automatic shadows made of specially designed tiles 32-47.
    //#define UNPACKED_MAP                          // Full, uncompressed maps. Shadows settings are ignored.
    #define RLE_MAP                     53        // Use RLE compression (values 44, 53, 62; see docs)
    //#define NO_MASKS            sp_OR_SPRITE     // Sprites are rendered using OR instead of masks.
    //#define PLAYER_ALTERNATE_ANIMATION          // If defined, animation is 1,2,3,1,2,3...
    #define NO_ALT_BG                             // No subs 0 for 19 at random
    //#define HOTSPOTS_FIRST_TILE      32          // Override default value of 16
    //#define REVERSE_OBJECT_COUNT                 // Count objects left rather than objects got
    //#define ENABLE_ANIMATED_TILES      46       // Must be EVEN
    //#define TILANIM_PERIOD             16       // Must be power of two
```

* `RLE_MAP`: el valor es el "formato" del RLE (44, 53 o 62), y tiene que coincidir con
  el conversor. Si no se define, el mapa es packed o unpacked según el conversor.
  (Ojo: el comentario de `config.h` dice "44, 54, 62", pero el valor bueno es el 53,
  que es el del conversor `rle53map_sp.exe`.)

* `USE_AUTO_SHADOWS` / `USE_AUTO_TILE_SHADOWS`: sombras automáticas. Úsalas con caution:
  la implementación de la v3.2 es sencilla.

* `NO_ALT_BG`: sin el tile decorativo aleatorio (el `HOTSPOTS_FIRST_TILE + 3`) en el
  fondo. Sin esta directiva, el motor sustituye el tile 0 por ese tile con una
  probabilidad de 2/16 en cada tile de la pantalla.

* `ENABLE_ANIMATED_TILES`: tiles animados. El valor es el **primer tile** que se anima:
  todos los tiles de la pantalla con número >= `ENABLE_ANIMATED_TILES` se registran como
  animados, y cada `TILANIM_PERIOD` frames se les hace `tile XOR 1` (por eso el número
  tiene que ser par). Como mucho se pueden registrar `MAX_ANIMATED_TILES` (16) tiles
  animados a la vez.

* `TILANIM_PERIOD`: periodo de la animación, en frames. Tiene que ser una potencia de
  dos.

## IV. Movimiento del jugador

```c
    #define PLAYER_MAX_VY_CAYENDO   512     // Max falling speed (512/64 = 8 pixels/frame)
    #define PLAYER_G                 64     // Gravity acceleration (32/64 = 0.5 píxeles/frame^2)

    #define PLAYER_VY_INICIAL_SALTO 160     // Initial junp velocity (64/64 = 1 píxel/frame)
    #define PLAYER_MAX_VY_SALTANDO  240     // Max jump velocity (320/64 = 5 píxels/frame)
    #define PLAYER_INCR_SALTO        72     // acceleration while JUMP is pressed (48/64 = 0.75 píxeles/frame^2)

    #define PLAYER_INCR_JETPAC        32     // Vertical jetpac gauge
    #define PLAYER_MAX_VY_JETPAC    256     // Max vertical jetpac speed

    #define PLAYER_MAX_VX           256     // Max velocity (192/64 = 3 píxels/frame)
    #define PLAYER_AX                48     // Acceleration (24/64 = 0,375 píxels/frame^2)
    #define PLAYER_RX                64     // Friction (32/64 = 0,5 píxels/frame^2)
```

Todas las velocidades y posiciones están en unidades de **1/64 de píxel**. Los valores de
`config.h` ya están en esas unidades (el comentario te dice el equivalente en píxeles).

* `PLAYER_G` es la gravedad: se suma a `vy` en cada frame (en vista lateral).
  `PLAYER_MAX_VY_CAYENDO` es la velocidad máxima de caída.
* `PLAYER_VY_INICIAL_SALTO` es el impulso inicial del salto; `PLAYER_INCR_SALTO` es la
  fuerza extra mientras se mantiene pulsada la tecla (y se va apagando);
  `PLAYER_MAX_VY_SALTANDO` es la velocidad máxima mientras se salta.
* `PLAYER_AX` es la aceleración horizontal, `PLAYER_RX` la fricción (se aplica cuando no
  se pulsa la dirección) y `PLAYER_MAX_VX` es la velocidad máxima.
* `PLAYER_INCR_JETPAC` y `PLAYER_MAX_VY_JETPAC` no se usan (no hay jetpac).

## V. Comportamiento de los tiles

Lo vimos en el capítulo 1. Se define en el array `comportamiento_tiles` de `config.h`, con
un valor por tile (48 en total).

---

# Capítulo 7 - El scripting (MSC4)

La v3.2 trae un motor de scripting propio, el **MSC4** (*Mojon Script Compiler 4*), que
compila un script escrito en un lenguaje sencillo a un *bytecode* interpretado por un
intérprete que se genera automáticamente.

**La especificación completa del lenguaje está en [`msc4.md`](msc4.md)** (en español) y en
[`msc4-scripting.md`](msc4-scripting.md) (en inglés). Aquí sólo vamos a ver cómo se
integra con el motor de la v3.2.

El esqueleto de `src/` viene **sin scripting activado**: en `config.h` están comentadas
`ACTIVATE_SCRIPTING`, `SCRIPTING_DOWN`, `SCRIPTING_KEY_M`, `NO_INDEXED_SCRIPTING` y
`ENABLE_ENCODED_TEXT` (sólo `ENABLE_DIALOG` está puesto). Para activarlo, descomenta
`ACTIVATE_SCRIPTING` (y `ENABLE_ENCODED_TEXT` si vas a usar `TEXT BOX`), descomenta el
bloque de compilación del script en `comp.bat` y añade `msc4i.asm` a la línea de `zcc`.

## El proceso de compilación

El compilador es `utils/msc4.exe`. Su código fuente está en `msc4/`, en la raíz del
repositorio (ver el apéndice A): la copia que usa cada juego es la de su propio `utils/`,
así que cuando reconstruyas el compilador no olvides copiar el `.exe` nuevo a los
juegos que lo usen.

Se invoca desde `comp.bat`:

```bat
    cd ..\script
    ..\utils\msc4.exe in=script.spt v=3 target=%om% interpreter=msc4i.asm text=text.bin jumptable debug >..\dev\msc.txt
    copy script.spt.bin ..\dev > nul
    copy text.bin ..\dev > nul
    move msc4i.asm ..\dev > nul
    cd ..\dev
```

* `in=script.spt`: el script de entrada.
* `v=3`: la versión del motor (3 para la v3.2).
* `target=cpc|zx`: para qué destino se genera el intérprete (afecta a la acción `SHOW`).
* `interpreter=msc4i.asm`: el nombre del intérprete generado.
* `text=text.bin`: el nombre del fichero con los textos codificados.
* `jumptable`: usa una tabla de saltos en vez de un índice para las secciones por
  pantalla. Es lo habitual en juegos con muchas pantallas y pocas con script.
* `debug`: vuelca la compilación a `msc.txt`. **Úsalo siempre**: es la única manera de ver
  el bytecode que genera el compilador.

El compilador produce tres cosas:

| Salida | Contenido |
|---|---|
| `script.spt.bin` | El bytecode del script (índice de secciones + código). Se integra con `BINARY "script.spt.bin"`. |
| `text.bin` | Los textos codificados en 5 bits que usan `TEXT BOX` y `DIALOG`. |
| `msc4i.asm` | El intérprete: un fichero de ensamblador Z80 **generado** y **especializado** a los opcodes que usa tu script. |

Ese último punto es la clave: el intérprete no es fijo, se genera con sólo las
comprobaciones y los comandos que tu script utiliza. Un script que no use `DIALOG` no
gastará ni un byte en código de diálogo.

**No olvides recompilar el intérprete cuando cambies el script.** Y recuerda que el
`msc4i.asm` hay que añadirlo a la lista de ficheros que compila `zcc`:

```bat
    zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin msc4i.asm churromain.c -DSPECCY > nul
```

## El interface: `msc4i.h`

El puente entre el motor y el script es `dev/msc4i.h`, un fichero que se escribe a mano
(una vez por juego) y que incluye:

* Las constantes de número de sección:

```c
    #define SC_ENTERING_GAME 0
    #define SC_ENTERING_ANY 1
    #define SC_PRESS_FIRE_AT_ANY 2
    #define SC_PLAYER_GOT_SOMETHING 3
    #define SC_PLAYER_KILLS_ENEMY 4
    #define SC_SPECIAL_TILE_TOUCHED 5
    #define SC_ENEMY_TOUCHED 8
    #define SC_ENTERING_SCREEN 16
    #define SC_PRESS_FIRE_AT_SCREEN 17
```

* El punto de entrada `script (unsigned char n)`.
* El decodificador de textos de 5 bits y el pegamento de `textbox()` y `dialog()`.
* Las directivas `BINARY` que incrustan `script.spt.bin` y `text.bin` en el binario.

El `script()` es éste:

```c
    void __FASTCALL__ script (unsigned char a) {
        #asm
                ld  a, l
                ld  (_script_n), a          // qué sección ejecutar

                ld  a, (_tpx)              // refresca TN con el tile
                ld  c, a                   //   del centro del jugador
                ld  a, (_tpy)
                call qtile_do
                ld  a, l
                ld  (_tqt), a

                ld  a, 0xff                // PARAM = $FF al entrar
                ld  (_script_param), a

                call _script_do

                // gpx, gpy pueden haber cambiado, así que actualiza player.x/player.y
                call _shl_player_coords
        #endasm
    }
```

Fíjate en la última línea: el script **puede mover al jugador** (con `PY = 32`, por
ejemplo), y el motor actualiza las coordenadas del jugador después de la llamada.

## Cuándo se dispara cada sección

| Evento | Dónde | Llamada |
|---|---|---|
| Empieza una partida | `mainloop.h` | `script (SC_ENTERING_GAME)` |
| Entramos en una pantalla | `engine.h` (en `draw_scr`) | `script (SC_ENTERING_ANY)` y luego `script (SC_ENTERING_SCREEN + (n_pant << 1))` |
| Se pulsa la tecla de acción | `mainloop.h` | `script (SC_PRESS_FIRE_AT_ANY)` y luego `script (SC_PRESS_FIRE_AT_SCREEN + (n_pant << 1))` |
| El jugador coge algo | `mainloop.h` | `script (SC_PLAYER_GOT_SOMETHING)` |
| Muere un enemigo | `engine.h` | `script (SC_PLAYER_KILLS_ENEMY)` |
| El jugador pisa un tile de comportamiento 128 | `engine.h` | `script (SC_SPECIAL_TILE_TOUCHED)` |
| El jugador toca un enemigo | `engine.h` | `script (SC_ENEMY_TOUCHED)` |

La tecla de acción se configura con `SCRIPTING_DOWN` (abajo), `SCRIPTING_KEY_M` (la M) o,
si no se define ninguna, FIRE. El disparo se hace **al pulsar** la tecla, no mientras se
mantiene.

Las secciones por pantalla (`ENTERING SCREEN n` y `PRESS FIRE AT SCREEN n`) se pueden
desactivar en el motor con `NO_INDEXED_SCRIPTING`, y en el compilador con la directiva
`NOINDEXED` del script.

El orden importa: cuando entramos en una pantalla se ejecuta primero `ENTERING ANY` y
después `ENTERING SCREEN n`. Lo mismo con la tecla de acción.

Un detalle que puede sorprender: `OPANT` (la pantalla anterior) vale `0xFF` mientras la
partida está empezando, porque el motor inicializa `on_pant` a 255. Por eso hay scripts
que comprueban `OPANT < 60` antes de fiarse del valor (así hace **Sir Ababol**).

## El lenguaje, resumido

Para la gramática completa, [`msc4.md`](msc4.md). Un resumen de lo más importante:

Un script tiene directivas (fuera de las secciones) y secciones. Cada sección contiene
cláusulas; cada cláusula es una lista de condiciones `IF` seguidas de acciones, y las
acciones sólo se ejecutan si **todas** las condiciones son ciertas:

```
    ENTERING SCREEN 3
        IF %FLAG_LEAVE = 0
        THEN
            SET TILE (7, 4) = 16
            SOUND 7
        END
    END
```

* Las secciones se nombran `ENTERING GAME`, `ENTERING ANY`, `ENTERING SCREEN n`, `PRESS
  FIRE AT ANY`, `PRESS FIRE AT SCREEN n`, `PLAYER GOT SOMETHING`, `PLAYER KILLS ENEMY`,
  `SPECIAL TILE TOUCHED` y `ENEMY TOUCHED`. Se pueden agrupar varias pantallas:
  `ENTERING SCREEN 0, 2, 20`.
* Las variables del script son **flags**, un array de bytes (`flags[MAX_FLAGS]`, 32 por
  defecto) que vive en el motor. Se referencian como `$n`, o con un alias: `ALIAS
  %CARRYING = 0` y luego `%CARRYING`.
* `&%ALIAS` da el *número* del flag (útil para hacer tablas de puertas y medallones, como
  en **Sir Ababol**).
* Los valores especiales (`NPANT`, `PX`, `PY`, `TX`, `TY`, `TN`, `OBJS`, `LIFE`, `KILLED`,
  `HOTSPOT`, `EN_T`, `EN_N`, `EN_X`, `EN_Y`, `OPANT`, `PARAM`) dan acceso al estado del
  motor. `TX`, `TY` y `TN` son el tile del centro del jugador.
* Las expresiones `TILE AT (x, y)` y `BEH AT (x, y)` leen el mapa; `DIALOG "a", "b", "c"`
  presenta un menú.
* Los comandos más usados: `SET TILE (x, y) = t`, `SET BEH (x, y) = b`, `INC`/`DEC`,
  `SOUND n`, `TEXT BOX "..."`, `EXTERN n, m`, `GET ITEM SET $F`, `WARP TO n, x, y`,
  `BREAK`, `WIN GAME`, `GAME OVER`, `RERUN v` y `CALL sub`.
* Las directivas del script: `ROOMS = N`, `ALIAS %X = N`, `ITEM SLOT = N`, `ITEM EMPTY =
  N`, `TEXT WIDTH = N`, `NOINDEXED`, `FASTNPANT`.

## `EXTERN`: el puente con el C

El comando `EXTERN n, m` llama a la función `do_extern_action (n, m)` que tú escribes en
`extern.h`. Es la forma de hacer cosas que el script no puede:

```c
    void do_extern_action (unsigned char n, unsigned char m) {
        switch (n) {
            case 0: /* reset de la persistencia */ break;
            case 1: /* marcar el tile actual como cogido */ break;
            case 2: /* leer la persistencia y borrar los tiles */ break;
            case 3: /* actualizar el HUD */ break;
        }
    }
```

El segundo parámetro (`m`) está ahí para que puedas pasar un dato más (un número de
pantalla, un valor...). Si no lo das, vale 0.

Un caso real: **Papaya** usa las cuatro acciones para implementar la persistencia de las
papayas cogidas (un bitmap `persistence[]` de 640 bytes, 16 por pantalla, para las 40
pantallas del mapa). **Sir Ababol** usa `EXTERN 0` para dar un impulso vertical al
salir del agua, `EXTERN 1` para el teletransporte y `EXTERN 2` para un efecto de sonido.

## Cajas de texto y diálogos

Con `ENABLE_ENCODED_TEXT` (y `ENABLE_DIALOG`, que lo implica) el motor sabe dibujar
texto codificado en 5 bits:

* `TEXT BOX "..."` decodifica el texto y llama a tu `textbox (unsigned char *ptr)`, que
  dibuja la caja.
* `DIALOG "a", "b", "c"` decodifica las tres opciones y llama a tu `dialog (unsigned char
  *ptr)`, que debe **devolver en `L`** la opción elegida (1, 2 o 3).

El texto se decodifica en un buffer en `TEXT_BUFFER` (que por defecto es `BASE_FREE`, la
memoria libre que queda tras los arrays del motor), y las tres opciones de un diálogo van
en 32 bytes cada una.

El texto de las cajas de texto se codifica en 5 bits: las minúsculas se convierten en
mayúsculas, los acentos y los caracteres no soportados **se pierden**, y las líneas se
parten a `TEXT WIDTH` caracteres (24 por defecto). Los espacios y las comas sí se pueden
usar; en los diálogos, en cambio, las comas separan opciones.

## El objeto que llevas: `GET ITEM SET`

La v3.2 tiene una comodidad para los juegos en los que el jugador sólo puede llevar un
objeto (una llave, un medallón...). Se combina con las directivas `ITEM SLOT` y `ITEM
EMPTY` del script, con la sección `SPECIAL TILE TOUCHED` y con `ITEM_X`/`ITEM_Y`/
`ITEM_FLAG` de `config.h`.

* El flag `ITEM SLOT` (por defecto el 0) guarda el número de tile del objeto que llevas.
  `ITEM EMPTY` (0 por defecto) es el valor que significa "no llevo nada".
* Los objetos se colocan en el mapa como tiles con comportamiento **128**, y el motor
  dispara `SPECIAL TILE TOUCHED` cuando el jugador los pisa, con `TN` = el tile y
  `TX`/`TY` = sus coordenadas.
* `GET ITEM SET $F` comprueba que el hueco esté vacío, pone el tile en el hueco, borra el
  tile del mapa y pone el flag `$F` a 1.
* El motor dibuja el flag `ITEM_FLAG` como un tile en `(ITEM_X, ITEM_Y)`, así que se ve en
  el HUD.

## Los `flags` y el motor

El array de flags lo usan el motor y el script:

* `flags[ITEM_FLAG]` se dibuja como el "objeto que llevas".
* `flags[OBJECT_COUNT]` se dibuja como contador de objetos.

Y se **borra al empezar cada partida** (no al cambiar de pantalla).

## Victoria y derrota

* `WIN GAME` en el script pone `script_result = 1`, y el motor da la partida por ganada
  (y hace la pantalla de final).
* `GAME OVER` pone `script_result = 2`, y el motor hace el game over.

**Con `ACTIVATE_SCRIPTING` activado, las condiciones de victoria `PLAYER_NUM_OBJETOS` y
`SCR_FIN` se compilan fuera**: la única forma de ganar es con `WIN GAME` en el script.

## El log de compilación

El `msc.txt` que genera `comp.bat` (con `debug`) es tu mejor amigo: en él se ve, línea a
línea, los tokens que el compilador ha entendido y el bytecode que ha generado. Si una
cosa no hace lo que esperas, es lo primero que hay que mirar.

---

# Capítulo 8 - Código custom

## Los ficheros que son tuyos: `custom.h`, `custom_flick.h` y `extern.h`

Antes de nada, una aclaración importante que conviene tener muy presente: **`custom.h`,
`custom_flick.h` y `extern.h` no son parte del motor**. Son ficheros *tuyos*, con *tu*
código. El motor no los implementa: sólo **llama** a las funciones que therein, y lo hace
únicamente si has activado la directiva de `config.h` correspondiente.

Para no perderte, esto es lo que hay y lo que hay que hacer con cada cosa:

| Fichero | ¿Quién lo pone? | ¿Cuándo lo usa el motor? | Qué tienes que escribir tú |
|---|---|---|---|
| `dev/custom.h` | **Tú**, partiendo del esqueleto de `src/dev/custom.h` | Siempre se incluye (`churromain.c` hace `#include "custom.h"`), pero sus funciones sólo se llaman si la directiva de `config.h` está activa | La implementación de los hooks que te interesen |
| `dev/custom_flick.h` | **Tú**, partiendo del esqueleto de `src/dev/custom_flick.h` | Sólo se incluye si `ENABLE_CUSTOM_FLICK` está activo | `flick_left`, `flick_right`, `flick_up`, `flick_down` |
| `dev/extern.h` | **Tú**, partiendo del esqueleto de `src/dev/extern.h` | Sólo se incluye si `ACTIVATE_SCRIPTING` está activo | `do_extern_action`, y `textbox` / `dialog` si usas `TEXT BOX` / `DIALOG` |
| `dev/msc4i.h` | **El framework** (identical en el esqueleto y en **Papaya**) | Con `ACTIVATE_SCRIPTING` | Nada... salvo que tu juego necesite adaptarlo |

Los **esqueletos** están en `src/dev/`: son ficheros con todas las funciones vacías y con
los `#ifdef` de las directivas de `config.h` ya puestos, para que te limites a rellenar
los cuerpos. Por ejemplo, el esqueleto de `custom.h` es esto:

```c
    void before_vsync (void) {
    }

    #ifdef PLAYER_CUSTOM_FRAME
        unsigned char player_custom_frame (void) {
            // Return a frame NUMBER (0-7)
        }
    #endif
```

Y el de `extern.h`:

```c
    void do_extern_action (unsigned char n, unsigned char m) {
    }

    #ifdef ENABLE_ENCODED_TEXT
        // Text has been decoded in a 256 bytes buffer pointed by ptr.
        void __FASTCALL__ textbox (unsigned char *ptr) {
        }

        #ifdef ENABLE_DIALOG
            // Options have been decoded in three 32 bytes buffers pointed by ptr.
            unsigned char __FASTCALL__ dialog (unsigned char *ptr) {
                return 0;
            }
        #endif
    #endif
```

Y el de `custom_flick.h`:

```c
    unsigned char flick_left (void) {
        return 1;
    }
    // ... flick_right, flick_up, flick_down
```

Y **ejemplos completos** de cómo se rellenan están en los juegos de `games/`. Los tres
que más te van a interesar:

* **`games/papaya/dev/custom.h`**: implementa `before_vsync()` (que reinicia la música
  cuando el player WYZ avisa con `wyz_signal`) y `player_custom_frame()` (que elige el
  frame del muñeco según esté quieto, andando, o "empujando" hacia arriba o abajo). Es un
  buen ejemplo de cómo se escriben estas cosas en ensamblador embebido en C.
* **`games/papaya/dev/extern.h`**: implementa las cuatro acciones de `do_extern_action` de
  **Papaya** (que usan un array `persistence[]` para recordar qué papayas se han recogido
  en cada pantalla), la `textbox()` (la caja de texto con su marco, el redibujado del
  fondo y la espera de una tecla) y la `dialog()`.
* **`games/ababol/dev/custom.h`**: implementa `player_custom_veng()` (el motor de
  natação de las filas 40-59 del mapa), `player_custom_frame()` (andar / nadar) y
  `player_custom_bg_hit()`, además de los enemigos custom (los peces, tipos 5 a 8).

Y ejemplos mínimos, para games sin script: `games/perils/dev/extern.h` o
`games/cheril/dev/extern.h`.

Dos corolarios prácticos:

* Si activas una directiva de `config.h` (por ejemplo `PLAYER_CUSTOM_FRAME`) y **no** has
  implementado la función correspondiente, el juego no enlazará. Si la implementas pero
  **no** activas la directiva, el motor no la llamará y no hará nada.
* Como `custom.h` se incluye siempre, puedes usarlo también para tus propias variables y
  funciones, aunque no actives ninguna directiva.

## La API del motor

Lo que sigue es lo que el motor **sí** te pone: las variables globales (en
`definitions.h`) y las funciones que puedes llamar desde tu código custom.

### El jugador

El jugador es una estructura `INERCIA`:

```c
    typedef struct {
        signed int x, y, cx;                     // 0, 2, 4
        signed int vx, vy;                        // 6, 8
        unsigned char g, ax, rx;                  // 10, 11, 12
        unsigned char salto, cont_salto;          // 13, 14
        unsigned char *current_frame, *next_frame; // 15, 17
        unsigned char saltando;                   // 19
        unsigned char frame, subframe, facing;    // 20, 21, 22
        unsigned char estado;                     // 23
        unsigned char ct_estado;                  // 24
        unsigned char gotten;                     // 25
        unsigned char possee;                     // 26
        signed char objs, keys;                   // 27, 28
        signed int life;                          // 29
        unsigned char fuel;                       // 31
        unsigned char killed;                     // 32
        unsigned char disparando;                 // 33
        unsigned char killingzone_framecount;      // 34
        unsigned char killingzone_beepcount;      // 35
        unsigned char is_dead;                    // 36
        unsigned char ceiling;                    // 37
        unsigned char grab_block;                 // 38
        signed int max_vx;                        // 39
        unsigned char hovering;                   // 41
        unsigned char just_hovered;               // 42
        unsigned char just_jumped;                // 43
        unsigned char sword_g;                    // 44
        unsigned char coins;                      // 45
        unsigned char drain_amount;               // 46
        unsigned char hit_ladder;                 // 47
        unsigned char on_ladder;                  // 48
    } INERCIA;

    INERCIA player;
```

Los offsets están en el comentario, y son importantes: el intérprete del script los usa
(por eso el intérprete generado no compila si cambias la estructura sin avisar).

Los campos que de verdad se usan en la v3.2 son:

| Offset | Campo | Significado |
|---|---|---|
| 0, 2 | `x`, `y` | Posición, en 1/64 de píxel |
| 6, 8 | `vx`, `vy` | Velocidades, en 1/64 de píxel por frame |
| 13, 14 | `salto`, `cont_salto` | Frames de salto |
| 19 | `saltando` | 1 si está en el aire por un salto |
| 20, 22 | `frame`, `facing` | Frame de animación y orientación |
| 23, 24 | `estado`, `ct_estado` | Estado (`EST_NORMAL`, `EST_PARP`) y su contador |
| 25 | `gotten` | 1 si va montado en una plataforma móvil |
| 26 | `possee` | 1 si está apoyado en algo (sólo vista lateral) |
| 27, 28 | `objs`, `keys` | Objetos y llaves cogidos |
| 29 | `life` | Vida (**con signo**: se acaba cuando baja de 0) |
| 32 | `killed` | Enemigos matados |
| 36 | `is_dead` | Flag de muerte (y la causa) |
| 46 | `drain_amount` | Cuánta vida se pierde al morir |

(`hit_ladder` y `on_ladder` son el principio de un motor de escaleras que todavía no
está terminado; ver `src/diary.md`.)

### Otras globales útiles

| Variable | Significado |
|---|---|
| `gpx`, `gpy` | Posición del jugador en píxeles |
| `tpx`, `tpy` | Tile del centro del jugador |
| `tqt`, `tat` | Tile y comportamiento de ese tile |
| `n_pant`, `on_pant` | Pantalla actual y pantalla mostrada |
| `map_attr[150]`, `map_buff[150]` | Comportamientos y tiles de la pantalla actual |
| `hotspot_t`, `hotspot_x`, `hotspot_y` | El hotspot que se está tocando |
| `scenery_info` | `hide_hotspots`, `dont_make_rr` |
| `en_an_next_frame[enit]` | El gráfico del enemigo actual |
| `_en_x`, `_en_y`, `_en_t`, `_en_mx`, `_en_my` | Estado del enemigo actual (copiado a variables temporales) |
| `enit` | Índice del enemigo en proceso |
| `gpit` | Contador general de propósito |
| `half_life` | Alterna cada frame (sirve para el parpadeo) |
| `maincounter` | Contador de frames |

### El mapa

Dos accessors para leer el mapa de la pantalla actual:

```c
    unsigned char qtile (unsigned char x, unsigned char y);   // el tile
    unsigned char attr  (unsigned char x, unsigned char y);   // el comportamiento
```

`attr()` devuelve 0 fuera de la pantalla; la variante `_attr_enems` devuelve 8
(obstáculo), que es lo que usan los enemigos.

Y para escribir:

```c
    void set_map_tile (unsigned char x, unsigned char y, unsigned char t, unsigned char n);
```

que además redibuja el tile.

### Dibujar

```c
    void draw_coloured_tile (unsigned char x, unsigned char y, unsigned char t);
```

Las coordenadas son **en caracteres**, no en píxeles.

## Los hooks de `custom.h`

Todos los hooks de `custom.h` **son tuyos**: el motor sólo los llama, y sólo si has
activado la directiva correspondiente en `config.h`. Lo que sigue es el esqueleto (lo que
hay en `src/dev/custom.h`) con una explicación de cada hook.

### `before_vsync()`

```c
    void before_vsync (void) {
        // Run every game loop right before vsync.
    }
```

Se llama **en cada frame**, justo antes del vsync. Es el sitio para el código "de
fondo": cambiar la música, mover algo que no depende del jugador, etc.

### `player_custom_veng()` y `player_custom_heng()`

```c
    #ifdef PLAYER_CUSTOM_VENG
        void player_custom_veng (void) {
            // Modify vy as needed
        }
    #endif
```

Se llaman en cada frame, y sirven para modificar la velocidad vertical (`vy`) y horizontal
(`vx`) del jugador antes de moverlo. Con `PLAYER_CUSTOM_VENG` el motor desactiva su
propio cálculo de gravedad.

Ojo: la gravedad no es lo único que se desactiva. **Sir Ababol** implementa aquí *todo*
el eje vertical (gravedad, salto, y hasta el "salto hacia abajo" de las plataformas),
porque su motor de natação necesita otro comportamiento.

### `player_custom_frame()`

```c
    #ifdef PLAYER_CUSTOM_FRAME
        unsigned char player_custom_frame (void) {
            // Return the frame number (0-7) to use
        }
    #endif
```

Devuelve el frame de animación del jugador (0-7: 0-3 es "hacia la derecha", 4-7 "hacia
la izquierda" en vista lateral). Se usa mucho para hacer que el muñeco "mire" en la
dirección en la que se mueve, o para que se caiga en vertical.

### `custom_bg_hit()`

```c
    #ifdef PLAYER_CUSTOM_BG_HIT
        unsigned char custom_bg_hit (void) {
            // check hit_v, hit_h and modify player.x / player.y as you need
            // Return 1 if hit registered
        }
    #endif
```

Sustituye por completo la reacción del motor ante los tiles que matan. `hit_v` y `hit_h`
indican si el golpe ha sido vertical u horizontal.

### Enemigos custom

Con `ENABLE_CUSTOM_ENEMS` se pueden añadir tipos de enemigo o personalizar los que ya
existen:

```c
    void extra_enems_init (void) {
        // Runs for each enemy when entering a new screen. You should check _en_t
        // Current malote is malotes [enoffsmasi], iterator is enit
    }

    void extra_enems_move (void) {
        // Check _en_t and update your custom enemies.
        // write a proper frame pointer to en_an_next_frame
    }

    void extra_enems_checks (void) {
        // Called at the end of the enems update loop, after collisions
    }

    void extra_enems_killed (void) {
        // Called after enemy number enit is killed.
        // _en_t has been already "marked as dead" (OR 128)
    }

    unsigned char this_enemy_kills (void) {
        // return 0 for enemies which shouldn't collide.
        // Collide means they will kill the player or get squashed.
    }
```

El orden es: `extra_enems_move()` se llama *antes* de las comprobaciones de colisión, y
`extra_enems_checks()` *después*.

En `extra_enems_init()` el enemigo vivo está en `malotes [enoffsmasi]`, y el índice en
`enit`. En las demás, los valores del enemigo actual están copiados a `_en_x`, `_en_y`,
`_en_t`, etc., y puedes modificarlos.

`this_enemy_kills()` es muy útil: si devuelves 0, ese enemigo no hace daño y no se puede
aplastar. En **Papaya** se usa para que el "viejo" (el tendero) no mate al jugador.

Para que un enemigo custom tenga su propio gráfico, hay que llamar a
`enems_en_an_calc (tipo - 1)` en `extra_enems_init()`, que calcula el cell base a partir
del tipo (ver `ENEMS_CELL_OFFSET`).

### Hotspots custom

```c
    #ifdef ENABLE_CUSTOM_HOTSPOTS
        unsigned char custom_hotspots (void) {
            // hotspot type is hotspot_t.
            // set rdi to prevent clearing the hotspot.
        }
    #endif
```

Se llama cuando el jugador toca un hotspot, antes de que se resuelva. `hotspot_t` es el
tipo del hotspot (1 = objeto, 2 = llave, 3 = recarga). Si pones `rdi = 1`, el hotspot
**no** se borra y el script no se ejecuta (útil para "no puedes coger esto hasta haber
hablado con alguien").

## `custom_flick.h`: las transiciones de pantalla

Con `ENABLE_CUSTOM_FLICK` el motor te pregunta antes de dejar al jugador pasar a la
pantalla de al lado:

```c
    unsigned char flick_left (void) {
        return 1;
    }
```

Si devuelves 0, el cambio de pantalla **no** se hace (y el jugador se queda donde
estaba). Si devuelves 1 (o no defines la directiva), el motor hace el cambio de pantalla
normalmente.

El esqueleto de `src/dev/custom_flick.h` devuelve 1 en las cuatro direcciones, es decir,
"haz todo lo normal".

## `extern.h`

Si el scripting está activo, `extern.h` (que es **tu** fichero) debe contener:

```c
    void do_extern_action (unsigned char n, unsigned char m);   // el comando EXTERN

    #ifdef ENABLE_ENCODED_TEXT
        void textbox (unsigned char *ptr);   // el comando TEXT BOX
    #endif

    #ifdef ENABLE_DIALOG
        unsigned char dialog (unsigned char *ptr);   // el comando DIALOG
    #endif
```

`textbox()` recibe el texto ya decodificado en ASCII, y dibuja la caja. `dialog()` recibe
las tres opciones (32 bytes cada una) y devuelve la elegida.

El motor no implementa ninguna de las tres: si activas `ENABLE_ENCODED_TEXT` y no
escribes `textbox()`, el juego no enlazará. El esqueleto de `src/dev/extern.h` te da las
tres con el cuerpo vacío; `games/papaya/dev/extern.h` es un ejemplo completo con las cajas
de texto, los diálogos y las acciones externas de **Papaya**.

## Memoria

En el Spectrum, el motor reserva su memoria con el asignador de bloques de splib2:

```c
    sp_AddMemory (0, NUMBLOCKS, 14, AD_FREE);
```

`NUMBLOCKS` (40) bloques de 14 bytes en `AD_FREE` (61440 - 40*15 = 60840). Cada sprite
16x16 gasta 10 bloques y uno de 8x8, 5. Si usas disparos, hay que subir `NUMBLOCKS` en 5
por cada bala.

En el CPC **no hay memoria dinámica**: las estructuras de sprite son un array estático.

El resto de la memoria está repartida en constantes `BASE_*` (ver `definitions.h`), que
en el Spectrum son direcciones fijas a partir de 23300 (`BASE_ROOM_BUFFERS`) y 23600
(`BASE_ARRAYS`). En el CPC son direcciones absolutas en la RAM expandida, entre `0x0100`
(el tilemap) y `0xF800` (las LUTs).

---

# Capítulo 9 - La versión CPC

La v3.2 tiene port a CPC, y es el que se está usando actualmente para hacer los juegos
nuevos (Papaya, Trabajo Basura, Perils, Sir Ababol...). Las diferencias principales:

| Zona | Spectrum (`speccy.h`) | CPC (`cpc.h`) |
|---|---|---|
| Pantalla | La de 32x24 caracteres se dibuja directamente; `unpack_screen` desempaqueta en `16384` | Un **superbuffer** en `0x9000` más una zona visible en `0xC000`; `cpc_ShowTileMap(1)` la copia |
| Sprites | splib2: estructuras `sp_SS`, `SPMoveSprAbs` | Estructuras `SPR` de 16 bytes escritas a mano, con punteros `invfunc`/`updfunc` |
| Colores | Atributos de la pantalla | Paletas de hardware de 16 colores: `my_inks` (juego), `pal_general` (área de juego), `pal_hud` (HUD), `palmap[]` (una por pantalla) |
| Música | AY con el player de Shiru (`music.h`) | WYZ (`wyz/wyz_player.h`) con los macros `AY_INIT` / `AY_PLAY_MUSIC` / `AY_STOP_SOUND` |
| Modo | 32x24 caracteres (IM2) | `cpc_SetMode(1)` (MODE 1) o `cpc_SetMode(0)` |
| Memoria dinámica | El asignador de bloques de splib2 | No hay (todo estático) |

Para compilar para CPC, `comp.bat` usa `-DCPC -DMODE_%mode%` y enlaza contra
`-lcpcrslib_mt` (o `-lcpcrslib_fg_mt`, que es la variante con "fg"). En Spectrum, las
directivas son `-DSPECCY`.

## El raster split

En CPC, para tener paletas distintas en el área de juego y en el HUD hace falta un
*raster split*:

```c
    #define AUTO_SPLIT     // For CPC, but hud must be 32 raster on top and MODE 1
    #define ALWAYS_SPLIT   // Useful for DIRECT_TO_PLAY
    #define NO_PAL_MAP     // For CPC, with AUTO_SPLIT, only one pal (my_inks)
```

* `AUTO_SPLIT`: el ISR de CPC cambia a la paleta del juego en la línea 3 y a la del HUD a
  partir de la 6, y ejecuta el player de música.
* `ALWAYS_SPLIT`: el split es permanente (imprescindible con `DIRECT_TO_PLAY`).
* `NO_PAL_MAP`: se usa una sola paleta para todo.

Sin `NO_PAL_MAP`, `cpc_UpdateNow` parchea la paleta por pantalla usando el array
`palmap[]` (en `cpc/palmap.h`), que contiene una rutina de paleta por pantalla.

Para que todo esto funcione, las paletas tienen que estar en formato ensamblador (se
generan con `mkts_om.exe ... mode=palsasassembly`), y el HUD tiene que estar en las 32
primeras líneas de raster y en MODE 1.

### Paletas por pantalla

Si quieres una paleta distinta por zona del mapa (como hace **Sir Ababol**, que tiene
cuatro: fuera, cueva, agua y la última cueva), hay que:

1. Generar un fichero de ensamblador por paleta con `mkts_om.exe ... mode=palsasassembly
   prefix=... out=cpc/palN.h`.
2. Incluirlos en `config.h`, al final:

```c
    #if defined CPC && defined MODE_1 && defined AUTO_SPLIT
        #include "cpc/pal0.h"
        #include "cpc/pal1.h"
        #include "cpc/pal2.h"
        #include "cpc/pal3.h"
    #endif
```

3. Cambiar `cpc/palmap.h` para que llame a la paleta correcta en cada pantalla.

**Ojo**: el esqueleto de `src/dev/cpc/` **no** incluye los ficheros `pal0.h` a `pal3.h`,
de modo que si activas `AUTO_SPLIT` en MODE 1 tendrás que crearlos tú (o copiar los de
**Sir Ababol**).

## El loader

El Spectrum usa un loader de una línea (`dev/system/loader.bas`) que carga la pantalla de
carga (`SCREEN$`), parchea el flag de inversión de caracteres (`POKE 23739`) y carga el
CODE en 24200, saltando allí (`USR 24200`).

El CPC usa un loader de dos etapas, ensamblado con `pasmo` y empaquetado en un `.cdt` con
`cpc2cdt.exe`:

* `dev/system/preloadercpc.asm`: el preloader, en `$400`, desactiva las interrupciones,
  carga el loader real con `cpct_miniload_asm` y salta a él.
* `dev/system/loadercpc.asm`: el loader real, en `$BA00`: pone todas las tintas a negro,
  pone el modo, carga la pantalla de carga (comprimida con zx7) y la desempaqueta en
  `$C000`, carga el binario principal (comprimido con zx7) y lo desempaqueta en `$0400`, y
  salta allí.

Los dos ficheros se generan a partir de las plantillas `*-orig` con `imanol.exe`, que
sustituye `%%%scrc_size%%%`, `%%%mainbin_size%%%`, `%%%loader_size%%%` y
`%%%loader_mode%%%`.

---

# Capítulo 10 - El *toolchain*

Todas las herramientas están en `utils/` (y una copia en el `utils/` de cada juego). Las
más importantes:

| Herramienta | Para qué sirve |
|---|---|
| `ts2bin.exe` | Convierte la fuente y el tileset del Spectrum a `tileset.bin`. |
| `sprcnv.exe` / `sprcnv2.exe` | Convierten el *spriteset* del Spectrum a un `.h` con los punteros a los sprites. `sprcnv2` admite más de 16 sprites. |
| `png2scr.exe` | Convierte un `.png` de 256x192 a un `.scr` del Spectrum. |
| `zx0.exe` / `zx7.exe` | Compresores. `zx0` para las pantallas fijas y el mapa RLE; `zx7` para el loader de CPC. |
| `mkts_om.exe` | El conversor de gráficos del CPC: fuente (`mode=chars`), tiles (`mode=strait2x2`), sprites (`mode=sprites`), pantallas (`mode=superbuffer`), carga (`mode=scr`) y paletas en ensamblador (`mode=palsasassembly`). |
| `mapcnv.exe` | El conversor clásico de mapas (packed / unpacked). |
| `rle44map_sp.exe`, `rle53map_sp.exe`, `rle62map_sp.exe` | Los conversores de mapa comprimido con RLE (4, 5 y 6 bits de tile). |
| `ene2h.exe` | Convierte `enems.ene` en `enems.h`. |
| `ponedor.exe` | El editor de enemigos y hotspots. |
| `msc4.exe` | El compilador de scripts MSC4. |
| `pasmo.exe` | El ensamblador Z80 (para los loaders y el intérprete generado). |
| `wyzTrackerParser.exe` | Convierte el `instrumentos.asm` del tracker OGT en `wyz/instrumentos.h`. |
| `imanol.exe` | Sustituye plantillas en los loaders de CPC. |
| `printsize.exe` | Imprime el tamaño del binario compilado. |
| `bas2tap.exe`, `bin2tap.exe`, `GenTape.exe` | Construyen las cintas de ZX Spectrum. |
| `cpctbin2sna.exe` | Convierte el binario de CPC en un `.sna`. |
| `cpc2cdt.exe` | Empaqueta los bloques en una cinta de CPC (`.cdt`). |

Los ficheros `.bas` de algunas herramientas están junto a su `.exe` (`mkts_om.bas`,
`ponedor.bas`, `imanol.bas`, `rle53map_sp.bas`...), así que se pueden reconstruir con
FreeBASIC. El del compilador de scripts está en `msc4/` (ver el apéndice A).

---

# Apéndice A - Reconstruir el compilador de scripts

El compilador `msc4.exe` está escrito en FreeBASIC y su código está en `msc4/` en el
raíz del repositorio. Para reconstruirlo:

```
    fbc msc4.bas mtparser.bas cmdlineparser.bas mt5bitEsc.bas
```

Los cuatro ficheros hay que compilarlos juntos (los `.bi` son las declaraciones). Cualquier
versión reciente de FreeBASIC sirve; se desarrolló con la 1.08.1.

Después de reconstruirlo, hay que copiar el `msc4.exe` nuevo a la carpeta `utils` de los
juegos que lo usen, y regenerar los intérpretes (`.asm`) y el bytecode (`.bin`) de esos
juegos.

En `msc4/` hay además herramientas de apoyo: `decodeFromTextBin.exe` (decodifica un
`text.bin` a texto legible) y `jumptable_msc.bas` (ayuda a generar tablas de saltos).

# Apéndice B - La estructura del repositorio

```
mk1v32/
  docs/       <- este manual y la especificación de MSC4
  env/        <- el entorno: z88dk10, Mappy, WYZ Tracker, la paleta de CPC, XVI32
  games/      <- los juegos de ejemplo (papaya, ababol, cheril, perils, ...)
  lib/        <- splib2 y cpcrslib, con updatelib.bat para instalarlos
  msc4/       <- el compilador de scripts MSC4
  src/        <- el esqueleto: copia esto para hacer un juego nuevo
```

Dentro de `src/`:

```
src/
  dev/        <- el código: config.h, custom.h, engine.h, churromain.c, comp.bat, ...
  dev/cpc/    <- los ficheros específicos de CPC (tileset, sprites, paletas)
  dev/speccy/ <- los ficheros específicos de Spectrum (tileset, sprites)
  dev/system/ <- los loaders (ZX de una línea y CPC de dos etapas)
  dev/wyz/    <- el player de música WYZ (CPC)
  enems/      <- enems.ene y el .MAP de Mappy con la colocación de enemigos
  gfx/        <- los gráficos, en speccy/ y cpc/
  map/        <- mapa.fmp (Mappy) y el .MAP exportado
  ogt/        <- la música: .mus del tracker OGT y sus exports comprimidos
  script/     <- el script de MSC4, si lo usas
  show/       <- el resultado compilado
  utils/      <- el toolchain
```

Y dentro de un juego, el `dev/` es el mismo que el `src/dev/`, más lo que hayas añadido
tú (`custom.h`, `extern.h`, `custom_flick.h`, `custom_sprite_cells.h`, `samples.h`...).

# Apéndice C - Lo que la v3.2 **no** tiene

Para que no pierdas tiempo buscändalo: esta versión del motor no implementa nada de esto.

* **Cajas empujables** completas. Lo único es `PLAYER_PUSH_BOXES`, que mueve el tile 14
  cuando el jugador choca contra él. No hay gravedad para las cajas, ni `PUSH_AND_PULL`,
  ni cajas que maten.
* **Motor de la espada.** `ENABLE_SWORD` existe, pero sólo hace dos cosas: hace que la
  estructura del enemigo tenga el campo `life` y que se reserve un sprite en el CPC. No
  hay ni código de espada ni colisiones con ella: si la quieres, la escribes tú (y para
  eso está `sprites_sword.png`).
* **Tipos de enemigo** aparte del 1, 2, 3 y 4: todos los demás son enemigos custom (ver
  el capítulo 5).
* **Motor de pantallas fijas** (con niveles, `SHOW_LEVEL_INFO`, `RESPAWN_REENTER`...).
  La v3.2 tiene las cuatro pantallas fijas del capítulo 4 y poco más.
* **Motor de monedas** y **motor de persistencia**. La persistencia la tienes que hacer
  tú, como hace **Papaya** con su array `persistence[]` y las acciones `EXTERN`.
* **Tiles rompibles**, **tiles resbalosos** y **zonas malvadas** (el comportamiento 3 no
  existe). Los comportamientos implementados son 0, 1, 2, 4, 8 y 128.
* **Jetpac** (`PLAYER_HAS_JETPAC` está en el código, pero su rama está vacía).
* **Disparo en cuatro direcciones** (`SHOOT_4_WAY`): está en `src/diary.md` como idea,
  no implementado.
* **Escaleras**: `player.hit_ladder` y `player.on_ladder` están en la estructura del
  jugador, pero el motor no hace nada con ellas (ver `src/diary.md`).
* **Motor de scripting** aparte del MSC4 que se describe en el capítulo 7.

Y al revés, la v3.2 tiene cosas que no te vas a encontrar en otros motores:

* `possee` y `gotten`, los campos del jugador que te dicen si está apoyado o montado en
  una plataforma.
* El buffer de mapa por pantalla (`map_attr` + `map_buff`), que hace que los cambios del
  script se puedan deshacer.
* El modo `FAKE_SIDE_VIEW` para juegos sin gravedad.
* `DEBUG_NAVIGATOR`, un truco para navegar por el mapa (¡y que hay que desactivar antes de
  publicar!).
* Los hooks de `custom_flick.h`, que te permiten impedir una transición de pantalla.
