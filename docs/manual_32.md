# MTE MK1 v3.2

Copyleft 2011, 2026 by The Mojon Twins

## Qué es este documento

Es el manual de la MK1v3.2. Si ya sabes hacer cosas con la churrera y con la v4 puedes saltarte los capítulos 1 a 5 (las herramientas son casi las mismas) e irte directamente al 6.

El capítulo 7 habla del scripting, pero **no** sustituye a la especificación del lenguaje: para la gramática completa del lenguaje ve a [`msc4.md`](msc4.md). Aquí sólo explicamos cómo se engancha el script con el motor de la v3.2.

## Antes de empezar

Los güegos de **MTE MK1** se compilan con **splib2** (Spectrum) y con **cpcrslib** (CPC). El entorno de desarrollo es el `z88dk10` que viene en `env/z88dk10-orig.zip`: descomprímelo en `C:\` y ejecuta `C:\z88dk10-orig\setenv.bat` para tener el compilador de Z80 (`zcc`), el ensamblador (`pasmo`) y las librerías en el `PATH`.

El esqueleto de un güego nuevo está en **`src_old/`**. Copia ese directorio (con todo lo que hay dentro) a donde quieras y renómbralo con el nombre de tu juego. Si entras en `dev/` y ejecutas `comp.bat` obtendrás el güego de ejemplo.

Antes de que te copies nada, echa un vistazo a cómo se monta un güego:

```
src_old/
  dev/        <- el código del güego: config.h, custom.h, engine.h, ...
  enems/      <- la colocación de enemigos y hotspots (enems.ene + mapa.MAP)
  gfx/        <- los gráficos (png) del güego
  map/        <- el mapa (fmp de Mappy, y el .MAP exportado)
  ogt/        <- la música (OGT tracker + exports)
  script/     <- el script de MSC4 (script.spt), si lo usas
  show/       <- el resultado compilado
  utils/      <- el toolchain
  system/     <- los loaders y cosas del backend
```

El proceso de crear un güego se basa en sustituir los recursos del ejemplo, cambiar la configuración, y quizá programar un poquito en C (en `custom.h` / `extern.h`) o en el lenguaje de scripts.

Ya estás listo para funcionar.

---

# Capítulo 1 - El tileset

El tileset es la colección de archivos de 16x16 que se emplea para dibujar las pantallas del güego. Cada tile puede tener un comportamiento diferente que se define en la configuración del proyecto (obstáculo, plataforma, etc). **MTE MK1 v3.2** maneja *tilesets* de 48 tiles, de los cuales, y dependiendo del formato del mapa, los 16 primeros o todos ellos podrán ser empleados en dibujar la pantalla.

Los tilesets se dibujan siguiendo las restricciones del ZX Spectrum en un archivo .png de 256x48 que, por orden, guardaremos en `gfx/` con el nombre `work.png`.

Vayas a utilizar 16 o 48 tiles para el mapa, hay algunos tiles especiales, y contando los tiles de 0 a 47:

* El tile **14** (el penúltimo de la primera fila) será la *caja empujable* si activas `PLAYER_PUSH_BOXES`. Si no, puedes usarlo normalmente.
* El tile **15** (el último de la primera fila) será el *tile cerrojo* (una llave que abre una puerta). Lo detecta el conversor de mapas y lo guarda aparte.
* El tile **16** será la *recarga de vida* (los *hotspots* de tipo refill).
* El tile **17** representará los *objetos coleccionables*.
* El tile **18** representará las *llaves*.
* El tile **19** (el cuarto de la segunda fila) representa un tile alternativo por el que se sustituirá el tile 0 de forma aleatoria, para hacer bonito. Esto se puede desactivar con `NO_ALT_BG`, en cuyo caso puedes usarlo normalmente.

El primer tile de los *hotspots* (16) se puede cambiar con la directiva `HOTSPOTS_FIRST_TILE`; a partir de ahí, los tres siguientes son refill, objeto y llave.

## El comportamiento de los tiles

El comportamiento de cada tile se define en el array `comportamiento_tiles`, al final de `config.h`:

```c
    unsigned char comportamiento_tiles [] = {
        0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 0,
        128, 4, 4, 128, 4, 4, 4, 8, 8, 8, 8, 8, 0, 0, 0, 0,
        128, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0
    };
```

Los valores que la v3.2 entiende son:

| Valor | Significado |
|---|---|
| 0 | Transitable (nada) |
| 1 | Transitable y **mata** |
| 2 | Transitable y **esconde** (con `ENABLE_BOTIBOINS`, además **rebota**) |
| 4 | **Plataforma** (sólo frena al jugador si cae encima) |
| 8 | **Obstáculo** (bloquea al jugador en todas las direcciones) |
| 128 | **Tile especial**: dispara la sección `SPECIAL TILE TOUCHED` del script |

Los valores 16, 32 y 64 **no están implementados** en la v3.2 (no hay tiles resbalosos ni rompibles), y los comportamientos no se pueden combinar: un valor 5 (mata + plataforma) mata, pero un 12 (plataforma + obstáculo) es un obstáculo. Ojo con esto: el motor comprueba los bits con `&`, así que 12 se comporta como 4 y como 8 a la vez.

El bit 7 (128) se puede combinar con los demás: un comportamiento 132 es "tile especial que además es plataforma". El motor lo comprueba con `& 128`, así que funciona.

## `ts2bin.exe` y los colores del Spectrum

Los tilesets se convierten con la herramienta `ts2bin.exe`. **MTE MK1** espera que queden en `dev/tileset.bin`, por lo que, si abres `comp.bat` con tu editor de textos, verás que la llamada es:

```cmd
    ..\utils\ts2bin.exe ..\gfx\speccy\font.png ..\gfx\speccy\work.png tileset.bin 7 > nul
```

`ts2bin.exe` necesita también un archivo png de 256x16 con la fuente del juego. El cuarto parámetro (un número) es la tinta por defecto que se emplea si un cuadro de 8x8 tiene un solo color. Si se pasa "-1", el conversor elegirá blanco para los colores oscuros o negro para los claros.

La forma de detectar los colores del Spectrum tiene ciertas particularidades. Los colores serán detectados a partir de valores R, G o B mayores o iguales de 128, con un nivel mayor o igual que 192 para los colores sin brillo y mayor o igual a 240 para los colores con brillo.

**MTE MK1 v3.2** no soporta flash, pero puede utilizar el bit 7 del atributo (el de flash) para hacer algo muy interesante: las celdas con este bit a 1 no mostrarán sprites sobre ellas, con lo que parecerá que los sprites pasan *por detrás* de estos cuadros. Para que el conversor levante el bit 7 hay que utilizar valores diferentes para R, G o B: entre 128 y 159 para colores sin brillo y entre 160 y 191 para los colores con brillo.

Para guiarte puedes usar esta plantilla. Los colores de arriba representan a los colores del Spectrum sin y con BRIGHT. Las dos filas de abajo representan esos mismos colores pero harán que el conversor levante el bit 7.

![Colores para el conversor](https://github.com/mojontwins/MK1v4/blob/master/docs/images/colours.png)

Por último, si vas a usar esta funcionalidad tendrás que linkar contra `splib2f` en lugar de `splib2` modificando la linea que llama a `zcc` en `comp.bat`:

```cmd
    zcc +zx -vn churromain.c -o %game%.bin -lsplib2f -zorg=24200  > nul
```

## El tileset en CPC

En CPC las cosas son distintas: no hay atributos, hay paletas de hardware. El conversor es `mkts_om.exe`, que genera la fuente, el tileset y los sprites por separado:

```cmd
    ..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal.png mode=chars greyordered in=..\gfx\cpc\font.png out=font.bin silent > nul
    ..\utils\mkts_om.exe platform=cpc cpcmode=%mode% pal=..\gfx\cpc\pal_cnvtiles.png mode=strait2x2 greyordered in=..\gfx\cpc\work.png out=work.bin silent > nul
```

Los tiles del CPC se dibujan en "strait2x2" (rectos de 2x2) y las paletas se generan con `mode=palsasassembly`, que produce ficheros de ensamblador con las secuencias de `out (c),a` (ver el capítulo 9).

## El tileset para mappy

Mappy tiene su historia y necesita que el primer tile sea completamente negro. En esta versión de **MTE MK1** tendrás que generar un segundo tileset específico para Mappy con el primer tile negro si el primer tile no lo es. Puedes grabarlo como `mappy.png` en `gfx`.

![Tileset para Mappy](https://github.com/mojontwins/MK1v4/blob/master/docs/images/c01-002.png)

---

# Capítulo 2 - Mapeando

Para hacer el mapa puedes usar cualquier cosa siempre que te curres un conversor. Si quieres ir por el método estándar, tendrás que hacer los mapas con **Mappy** (en `env/mappy-mojon.zip`). Es interesante que asocies el tipo de archivo fmp a `mapwin.exe` para más comodidad de vida.

## Definiendo nuestro mapa

Lo primero es definir el tamaño de nuestro mapa para saber con qué nos enfrentamos. Hay que tener en cuenta que estamos en 48K y sería conveniente que nuestro mapa no emplease demasiado, ya que a lo que ocupe el mapa habrá que sumar lo que ocupen los enemigos.

* Si vamos a usar sólo 16 tiles diferentes, un número de tile se codifica sólo con 4 bits, con lo que en cada byte podemos almacenar dos tiles. De ese modo las pantallas, que son de 15 x 10 = 150 tiles ocuparían 75 bytes.

* Si vamos a usar los 48 tiles, ya no podemos meter dos tiles por byte, por lo que cada pantalla ocupará 150 bytes, el doble.

Los enemigos, que ya veremos más adelante, ocupan 7 bytes por pantalla si usamos `PACKED_ENEMS` (o 9 si no), y los *hotspots* 3 bytes por pantalla.

## Creando un proyecto en Mappy

Vamos a `File->New Map`, y rellenaremos el cuadro de diálogo que aparece con los datos de nuestro mapa. En el ejemplo, el mapa es de 8 pantallas de ancho y 5 de alto. Como cada pantalla tiene 15x10 tiles, eso significa que mi mapa será de 8 * 15 = 120 tiles de ancho y 5 * 10 = 50 tiles de alto. Además, nuestros tiles serán de 16 x 16 píxels. Por tanto, rellenamos estos datos sobre nuestro mapa en el cuadro de diálogo, dejando todo lo demás como estaba.

![Nuevo proyecto de Mappy](https://github.com/mojontwins/MK1v4/blob/master/docs/images/c02-001.png)

Pulsamos OK y el programa nos dirá que deberíamos cargar un tileset. Aceptamos y vamos a hacerle caso. Vamos a `File->Import`, lo que nos abrirá un cuadro de diálogo de selección de archivo. Buscamos nuestro tileset preparado para Mappy (el `mappy.png` que preparamos en el primer paso) y lo cargamos. La ventana de Mappy debería mostrarse así:

![Tileset cargado en Mappy](https://github.com/mojontwins/MK1v4/blob/master/docs/images/c02-002.png)

Por último, vamos a colocar unas guías que nos ayuden a saber dónde empieza y acaba cada pantalla. Para ello, nos vamos a `Maptools->Dividers`, marcamos la casilla Enable dividers y rellenamos en `Pixel gap X` y `Pixel gap Y` los valores 240 y 160, que son las dimensiones en pixels de cada pantalla. Pulsamos OK, y con esto estamos preparados para trabajar. Para empezar, grabamos el mapa en nuestro directorio `map` con el nombre `mapa.fmp`. Ahora sólo hay que ponerse a colocar tiles... Y acordarnos de grabar como `mapa.fmp` de cuando en cuando, por si las moscas.

![Un mapa en Mappy](https://github.com/mojontwins/MK1v4/blob/master/docs/images/c02-003.png)

La forma de colocar cerrojos (para ser abiertos con llaves) es sencilla: sencillamente pon el tile del cerrojo (el 15) en el mapa, y el conversor los detectará y los almacenará aparte. Hay que tener cuidado con una cosa: **en esta versión los cerrojos deberían ir a ras de suelo (sobre una plataforma sobre la que podamos caminar), ya que el motor puede no detectarlos si el muñeco no está posado y con una coordenada "justa"**.

## Exportando nuestro mapa

Para exportar nuestro mapa, sólo hay que irse a `File->Save As...` y salvarlo como `mapa.map`, especificando el `.map` de forma explícita, en el subdirectorio `map`.

## Importando nuestro mapa

Hay dos conversores para los mapas en la v3.2, y la elección depende de lo que queramos que ocupe el binario.

### `rle53map_sp.exe` (RLE, lo que se usa ahora)

```cmd
    ..\utils\rle53map_sp.exe in=..\map\mapa.map out=mapa.bin size=8,5 scrsize=15,10 tlock=15 mk1h=mapa.h mk1locks fixmappy > nul
```

* `in=` / `out=`: rutas del `.map` de Mappy y del binario de mapa.
* `size=8,5`: ancho y alto del mapa en pantallas.
* `scrsize=15,10`: dimensiones de la pantalla en tiles.
* `tlock=15`: el tile que se considera cerrojo.
* `mk1h=mapa.h`: genera el `mapa.h` con los `#define` y la estructura de los cerrojos.
* `mk1locks`: genera además el binario con la lista de cerrojos.
* `fixmappy`: resta 1 a cada byte del mapa (Mappy exporta los tiles numerados desde 1).

El formato binario es: un índice de `2 * MAP_W * MAP_H` bytes con el offset de los datos RLE de cada pantalla, y luego los datos RLE en sí. Con `RLE_MAP 53` cada byte es `(tile & 31) | ((cuenta - 1) << 5)`: 5 bits de tile y 3 de longitud (1 a 8). Las pantallas vacías tienen offset 0 y las pantallas idénticas comparten offset (el conversor las detecta y las guarda una sola vez).

Las variantes del RLE son:

| Constante | Bits de tile | Bits de cuenta | Rango de tiles |
|---|---|---|---|
| `RLE_MAP 44` | 4 | 4 | 0-15, cuentas de 1 a 16 |
| `RLE_MAP 53` | 5 | 3 | 0-31, cuentas de 1 a 8 |
| `RLE_MAP 62` | 6 | 2 | 0-63, cuentas de 1 a 4 |

Si no defines `RLE_MAP`, el mapa se guarda *packed* (dos tiles de 4 bits por byte, 75 bytes por pantalla); con `UNPACKED_MAP` se guarda sin comprimir (150 bytes por pantalla). El cambio de formato se hace en `config.h` y tiene que coincidir con el conversor.

### `mapcnv.exe` (packed, el clásico)

```cmd
    ..\utils\mapcnv.exe ..\map\mapa.map mapa.h 4 6 15 10 15 packed  > nul
```

Los dos primeros parámetros indican las rutas de entrada y salida. Los dos siguientes indican el **ancho** y el **alto** del mapa en pantallas. Los dos siguientes indican las dimensiones de cada pantalla, que para **MTE MK1** siempre son 15 y 10. El siguiente dígito indica el tile que debe ser detectado como cerrojo. El último parámetro `packed` indica que debe meter dos tiles en cada byte; si tu mapa usa los 48 tiles, deberás omitirlo.

## El `mapa.h`

El conversor genera un `mapa.h` con la información del mapa:

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

`MAX_CERROJOS` es el número de cerrojos del mapa. Si vale 0, el motor desactiva las llaves automáticamente (`churromain.c` define `DEACTIVATE_KEYS`).

---

# Capítulo 3 - Sprites

Nuestro juego empleará 8 sprites para el personaje principal y 8 más (4 enemigos con 2 frames cada uno) para los enemigos. Todos los sprites son de 16 x 16. El orden de estos sprites en nuestro *spriteset* depende de si tenemos un juego de vista cenital o de vista lateral.

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

El motor calcula solo el cell base de cada tipo de enemigo: `((tipo - 1) << 1) + ENEMS_CELL_OFFSET`, con `ENEMS_CELL_OFFSET` = 8 por defecto. Así, el tipo 1 usa los cells 8 y 9, el tipo 2 los 10 y 11, etc.

Para crear nuestro *spriteset*, partiremos de una imagen vacía de 256 x 32 píxels. Tendremos una hilera con los sprites del personaje principal y sus máscaras, y otra justo debajo con los demás sprites, con sus máscaras. Las máscaras irán intercaladas, es decir, tendremos un sprite seguido de su máscara, seguido del siguiente sprite seguido de su máscara, y así sucesivamente. Para pintarlas podemos usar cualquier color. El otro color, que indica "vacío", debe ser NEGRO PURO (RGB = 0, 0, 0) para que el conversor lo detecte sin problemas.

Grabaremos nuestro *spriteset* en `gfx` con nombre `sprites.png`.

## Importanto el *spriteset*

La importación se hace con `sprcnv.exe`:

```cmd
    ..\utils\sprcnv.exe ..\gfx\speccy\sprites.png speccy\sprites.h  > nul
```

Si hemos seguido las instrucciones no necesitarás tocar nada.

## Sprites extra

Hay tres ficheros de sprites adicionales, cada uno con su propio conversor:

* `sprites_extra.png`: los sprites que quieras añadir (2 de momento). Se convierten con `mkts_om.exe ... mode=sprites ... out=sprites_extra.bin max=2`.
* `sprites_bullet.png`: el sprite de la bala, de 8x8. `max=1 metasize=1,1`.
* `sprites_sword.png`: el sprite de la espada, de 8x8. `max=4`. Ojo: **la v3.2 no tiene motor de espada**; este sprite sólo se usa si escribes tú el código.

---

# Capítulo 4 - Pantallas fijas

En un juego de **MTE MK1** hay cuatro pantallas fijas. Una se almacena tal cual, en formato Spectrum: la pantalla de carga. Las otras tres se comprimen y forman parte del binario: se trata de la pantalla de título, el marco y la pantalla con la escena final. Cuanto más compleja sea una pantalla, más ocupará comprimida, por lo que una forma de liberar memoria es simplificar las pantallas que van en el binario.

Aunque sea obvio decirlo, las pantallas deben ocupar 256x192 pixels y respetar las restricciones del ZX Spectrum.

## La pantalla de carga

Si no modificas el paquete, la pantalla de carga se almacena como un archivo binario con formato Spectrum, esto es, 6912 bytes, por lo que no importa lo compleja o simple que sea. La pantalla de carga debe almacenarse en `gfx` como `loading.png`.

## La pantalla de título

La pantalla de título se presentará a modo de menú y sobre ella sonará la música de presentación. Deberían salir título, créditos y un menú con tres opciones 1: Keyboard, 2: Kempston, 3: Sinclair.

Esta pantalla debe guardarse en `gfx` como `title.png`.

## El marco

El marco se mostrará alrededor del área de juego mientras jugamos. Se utiliza para contener los marcadores. En el tendremos que hacer sitio para los diferentes valores del juego, dependiendo siempre de la configuración del motor. Los espacios donde van los marcadores numéricos deben estar alineados a carácter y dejar sitio para dos dígitos.

También tiene que haber espacio para la ventana de juego, igualmente alineado a carácter. Esta ventana ocupa 240x160 píxels (o 15x10 tiles).

![Marco de juego](https://github.com/mojontwins/MK1v4/blob/master/docs/images/c04-002.png)

Esta pantalla deber guardarse en `gfx` como `marco.png`.

## Título y marco

Para ahorrar memoria, **MTE MK1** permite juntar título y marco en una sola pantalla. El título y el menú se dibujarán dentro del area de juego. Si eliges esta configuración, graba la pantalla combinada de título y marco en `gfx` como `title.png` y deja `marco.png` en negro, y define `DIRECT_TO_PLAY` en `config.h` (con `DIRECT_TO_PLAY`, la pantalla de título *es* el marco del juego y no se vuelve a mostrar al empezar una partida nueva).

## La pantalla del final

La pantalla del final se mostrará cuando el jugador haya conseguido su objetivo final en el juego. Debe guardarse en `gfx` como `ending.png`.

## Convirtiendo a formato Spectrum

Para ello, desde `comp.bat` se utiliza `png2scr.exe` y `zx0.exe` para comprimir:

```cmd
    ..\utils\png2scr.exe ..\gfx\speccy\title.png ..\gfx\speccy\title.scr  > nul
    ..\utils\zx0.exe ..\gfx\speccy\title.scr titlec.bin > nul
```

En CPC se usa `mkts_om.exe ... mode=superbuffer` y luego `zx0.exe` también.

---

# Capítulo 5 - Enemigos y *hotspots*

En este capítulo usaremos `ponedor.exe` para colocar en cada pantalla de nuestro juego hasta tres enemigos y un *hotspot*, que no es más que una casilla donde poner un objeto coleccionable o una llave, o donde aparecerán las recargas.

## Los enemigos

En **MTE_MK1 v3.2** los enemigos también se refieren a las plataformas móviles. Los enemigos tienen asociados una serie de valores, entre ellos un **tipo**, que define el comportamiento y de forma indirecta el gráfico con el que se pinta. En la v3.2 sólo existen los siguientes tipos:

| Tipo | Comportamiento |
|---|---|
| 1, 2, 3 | Enemigo **lineal**: se mueve de un punto a otro rebotando en sus límites. Todos se comportan igual; lo que cambia es el gráfico. |
| 4 | **Plataforma móvil** (sólo en vista lateral): el jugador puede subirse y es arrastrado por ella. |
| 5 | **Fanty** de *Random Respawn*: no se coloca en el mapa, aparece solo cuando un enemigo muere (ver más abajo). |

Los gráficos de los enemigos se eligen automáticamente por su tipo (ver el capítulo 3).

### Colocación y edición: `ponedor.exe`

El editor de enemigos es `ponedor.exe`, que vive en `utils/`. Se puede lanzar arrastrando el fichero `enems.ene` sobre `enems/ponedor.bat`, o desde la línea de comandos:

```
    ponedor.exe file.ene
```

* Con `file.ene` como único argumento, abre el fichero para editarlo.
* `ponedor.exe new out=file.ene map=file.map tiles=file.png|bmp [adjust=n] size=w,h [scrsize=w,h] [nenems=n] [x2]` crea un fichero nuevo.

**Ojo con el formato**: el fichero `.ene` debe guardarse en el formato *legacy* de 2 bytes por hotspot. El `ponedor` lo detecta automáticamente en los ficheros originales, pero si creas uno nuevo hay que especificarlo. El modo de compatibilidad se comprueba examinando la parte superior de la ventana: si aparece `2b`, el `.ene` está en formato *legacy*; si aparece `3b`, en el formato actual. Para cambiar de uno al otro basta con pulsar la tecla `L`.

Los tipos de enemigo se introducen **en hexadecimal** en el diálogo. El campo `Attr` es la velocidad en píxeles por frame (potencias de 2: 1, 2, 4).

### De `.ene` a `enems.h`: `ene2h.exe`

```cmd
    ..\utils\ene2h.exe ..\enems\enems.ene enems.h 2bytes compacted indexed > nul
```

* `2bytes`: el formato legacy de 2 bytes por hotspot.
* `compacted`: el layout "packed" de los enemigos (`PACKED_ENEMS`), que guarda `x1,y1` y `x2,y2` en dos bytes con nibbles en vez de en cuatro bytes. Ahorra 2 bytes por enemigo.
* `indexed`: genera el array `enoffs_index[]`, con el índice del primer enemigo de cada pantalla (`INDEXED_ENEMS`). Con esto, las pantallas sin enemigos no ocupan nada.

El `enems.h` generado contiene la estructura `MALOTE`:

```c
    typedef struct {
        unsigned char x, y;
        unsigned char xy1, xy2;        // PACKED_ENEMS
        signed char mx, my;
        signed char t;
        #if (defined PLAYER_CAN_FIRE || defined ENABLE_SWORD) && ENEMS_LIFE_GAUGE > 1
        unsigned char life;
        #endif
    } MALOTE;
```

### El comportamiento de los enemigos lineales

El enemigo se mueve a velocidad `mx`/`my` (píxeles por frame) y **rebota** entre los dos puntos límite (`x1,y1` y `x2,y2`). Si los dos puntos no están alineados a un eje, el enemigo traza una diagonal "escalonada", porque los rebotes en X y en Y son independientes.

Si se define `PLAYER_PUSH_BOXES` o `ENABLE_CUSTOM_ENEMS`, los enemigos además colisionan con el escenario: se paran ante los obstáculos y ante los tiles que matan (según la máscara `ENEMIES_COLLIDE_MASK`, que vale 9 por defecto: obstáculo + mata).

### Random Respawn Fanties

Con `RANDOM_RESPAWN` se activa un motor en el que, cada vez que un enemigo muere, puede aparecer un *fanty* (un enemigo volador) en su lugar. La probabilidad es de 1/32 por frame y por enemigo muerto. El fanty **acelera hacia el jugador** con `FANTY_A` hasta una velocidad máxima de `FANTY_MAX_V`, y **se aleja** si el jugador está escondido. Se puede destruir con `FANTIES_LIFE_GAUGE` disparos.

## Los *hotspots*

Cada pantalla tiene **un** *hotspot* como máximo. Hay tres tipos:

| Tipo | Tile | Efecto |
|---|---|---|
| `HOTSPOT_TYPE_OBJECT` (1) | 17 | +1 objeto (y sonido 7) |
| `HOTSPOT_TYPE_KEY` (2) | 18 | +1 llave (y sonido 8) |
| `HOTSPOT_TYPE_REFILL` (3) | 16 | +`PLAYER_REFILL` de vida (y sonido 9) |

El primer tile de los hotspots se define con `HOTSPOTS_FIRST_TILE` (16 por defecto).

Con `ONLY_ONE_OBJECT`, el jugador sólo puede llevar un objeto a la vez: si ya lleva uno, el hotspot **no** desaparece y el script no se ejecuta.

Con `DEACTIVATE_REFILLS` no hay recargas. Y ojo: si **no** se define `DEACTIVATE_REFILLS`, cada vez que el jugador recoge un objeto, ese hueco tiene un 25 % de probabilidad de mostrar una **recarga** en su lugar (el "refill aleatorio" clásico de MK1, por el que las recargas van apareciendo donde han quedado los objetos).

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

* El marcador de objetos (`OBJECTS_X`) sólo se dibuja si no defines `DEACTIVATE_OBJECTS`. Con `REVERSE_OBJECT_COUNT` muestra los que *quedan* en vez de los que llevas. Con `ONLY_ONE_OBJECT` y `OBJECTS_ICON_X` se dibuja un icono en vez de dos dígitos.
* El marcador de llaves (`KEYS_X`) sólo se dibuja si defines `KEYS_X` y no `DEACTIVATE_KEYS`.
* El marcador de muertes (`KILLED_X`) sólo se dibuja con `PLAYER_KILLS_ENEMIES`.
* El "objeto que llevas" (`ITEM_X`/`ITEM_Y`) se dibuja a partir del flag `ITEM_FLAG`, y **sólo si el scripting está activado**. Es el mecanismo que usa `GET ITEM SET` (ver el capítulo 7).

---

# Capítulo 6 - Configurando el güego

El comportamiento básico del juego se define en el archivo `config.h` que hay situado en `dev`. Luego podremos tunear mediante el scripting o el código inyectable de `custom.h`, pero hay que empezar por el principio.

## I. Configuración general

En la primera sección configuramos el comportamiento general del juego: las dimensiones del mapa, la pantalla de inicio, la pantalla de final (si aplica), la posición del muñeco al inicio, el número de objetos que hay que recoger, etcétera.

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

* `MIN_FAPS_PER_FRAME` coloca un limitador de frames. El valor que le asignes será el máximo número de frames que dejará pasar por cuadro de juego. El ZX Spectrum funciona a 50 fps, por lo que, por ejemplo, un valor de 2 limitaría el juego a 25 fps, y uno de 3 a 16.6 fps. Obviamente si hay mucha carga en una escena el frame rate del juego bajará.

* `MAP_W` y `MAP_H` definen el tamaño del mapa en pantallas. Deben ser los mismos valores que usas en `comp.bat` como parámetros del conversor de mapas.

* `SCR_INICIO` es el número de pantalla en la que se empieza a jugar, contando de izquierda a derecha y de arriba abajo, empezando por 0. `PLAYER_INI_X` y `PLAYER_INI_Y` son las coordenadas de inicio a nivel de tiles.

* `SCR_FIN`, `PLAYER_FIN_X` y `PLAYER_FIN_Y` definen la pantalla y las coordenadas en las que el juego se da por terminado. **Cuidado**: si activas `ACTIVATE_SCRIPTING`, estas condiciones de victoria **desaparecen** (ver más abajo) y sólo gana quien ejecute `WIN GAME` en el script.

* `PLAYER_NUM_OBJETOS` especifica cuántos *hotspots* de tipo 1 tendremos que coleccionar para ganar. Con el scripting activado, esta condición también desaparece.

* `PLAYER_LIFE` es la cantidad de vida con la que empezamos a jugar, `PLAYER_REFILL` es cuánto se recarga la vida al coger una recarga. **Ojo**: la vida es un entero *con signo*, y el juego se acaba cuando `life < 0`, así que en realidad se puede perder `PLAYER_LIFE + 1` vidas.

* `MAX_ENEMS` y `MAX_FLAGS` **no** están en `config.h`: se definen en `churromain.c` (3 y 32 por defecto). `MAX_FLAGS` es el tamaño del array de flags del script, y `MAX_ENEMS` el número de enemigos por pantalla.

## II. Tipo de motor

Esta sección se usa para definir el comportamiento del motor. Muchas directivas están relacionadas, y otras son excluyentes entre sí.

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

* `PLAYER_PUSH_BOXES`: el tile 14 se puede empujar. Ojo: para que se pueda empujar, su comportamiento en `comportamiento_tiles` debe ser 8 (obstáculo), porque el motor sólo intenta empujar cuando el jugador choca con un muro.

* `DIRECT_TO_PLAY`: la activamos cuando compartimos título y marco en la misma pantalla. La pantalla de título no se vuelve a mostrar al empezar una partida.

* `DEACTIVATE_KEYS`: si nuestro juego no usa llaves ni cerrojos, activamos esta directiva para ahorrar espacio. Se activa automáticamente si `MAX_CERROJOS` es 0.

* `FIRE_TO_USE_KEY`: la llave sólo se usa si además se pulsa FIRE.

* `DEACTIVATE_OBJECTS`: si nuestro juego no usa objetos coleccionables, activamos esta directiva para ahorrar espacio.

* `ONLY_ONE_OBJECT`: activa el modo en el que el jugador sólo puede llevar un objeto. Al recoger un objeto, `player.objs` se pone a 1 y no podremos coger más.

* `OBJECT_COUNT`: indica qué flag se usa para contar los objetos (lo dibuja el motor como un contador de dos dígitos).

* `DEACTIVATE_EVIL_TILE`: si nuestro juego no emplea tiles que matan, puedes ahorrar espacio activándola.

* `PLAYER_BOUNCES`: el jugador rebota si le da un enemigo.

* `PLAYER_FLICKERS`: el jugador parpadea durante 50 frames si le da un enemigo. (En la v3.2 el número de frames está fijado a 50 en el motor; la macro no toma valor.)

* `ENABLE_BOTIBOINS`: el comportamiento 2 (esconde) hace además que el jugador rebote y suene un beep.

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

* `PLAYER_CAN_FIRE` activa el motor de disparos. **Ojo**: activa también el cambio de la tecla de salto: con disparos, el salto es la tecla "arriba" sola; sin disparos, hay que pulsar "arriba" **y** "fuego" a la vez.

* `PLAYER_BULLET_SPEED` define la velocidad de los proyectiles en píxeles por frame.

* `MAX_BULLETS` define el máximo número de balas en pantalla. Hay que tener cuidado porque a más balas, menos velocidad y más memoria dinámica tendremos que reservar (ver el capítulo 8).

* `PLAYER_BULLET_Y_OFFSET`: número de píxels desde el borde superior del jugador desde el que se dispara la bala.

* `PLAYER_AX_RECOIL`: si se define, se añadirá esta aceleración al eje horizontal en sentido contrario al sentido en el que mire el jugador cada vez que se dispare (retroceso del arma).

* `ENEMS_LIFE_GAUGE` especifica el número de veces que hay que darle con un disparo a un enemigo para matarlo. Con esta opción, la estructura del enemigo crece un byte.

* `FIRING_DRAINS_LIFE` (opcional): si se define, disparar resta vida (como en **Cheril the Goddess**). `FIRING_DRAIN_AMOUNT` define cuánta.

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
    //#define SIMPLE_PLATFORMS                   // No type 4 tiles or no type 8s over type 4s.
    #define AVOID_PLATFORM_HOP                  // Only works if SIMPLE_PLATFORMS is DISABLED!
    //#define JUMP_DOWN_PLATFORM                 // Use down on type 4s to jump down.
    #define DEACTIVATE_MOVING_PLATFORMS          // No moving platforms, type 4s are normal linear.
```

* `PLAYER_MOGGY_STYLE`: vista cenital (o "genital", que tiene más gracia). En vista cenital no hay gravedad: arriba y abajo se mueven como izquierda y derecha, y el jugador mira en cuatro direcciones.

* `FAKE_SIDE_VIEW`: para juegos sin gravedad, pero manteniendo los valores de orientación del jugador de la vista lateral. Requiere `PLAYER_MOGGY_STYLE`.

* `PLAYER_HAS_JUMP`: el jugador puede saltar. Para saltar hace falta estar apoyado en algo (`possee`) o sobre una plataforma móvil (`gotten`).

* `PLAYER_HAS_JETPAC`: **no está implementado** en la v3.2 (el código es un comentario). No lo actives.

* `PLAYER_KILLS_ENEMIES`: el jugador puede matar enemigos saltando encima. `PLAYER_MIN_KILLABLE` y `PLAYER_MAX_KILLABLE` acotan qué tipos se pueden matar así.

* `SIMPLE_PLATFORMS`: no hay tiles de tipo 4, o no hay tiles de tipo 8 encima de tiles de tipo 4. Simplifica las colisiones.

* `AVOID_PLATFORM_HOP`: en vista lateral, el jugador no "salta" al subir a una plataforma: sólo se posa en ella si lleva al menos un frame de velocidad vertical suficiente. Sólo funciona si `SIMPLE_PLATFORMS` está desactivada.

* `JUMP_DOWN_PLATFORM`: se puede usar "abajo" para bajar de las plataformas.

* `DEACTIVATE_MOVING_PLATFORMS`: las plataformas móviles (tipo 4) se tratan como tiles normales.

### Custom

```c
    //#define PLAYER_CUSTOM_VENG                 // Replace Vertical axis for your own
    //#define PLAYER_CUSTOM_HENG                 // Replace Horizontal axis for your own
    #define PLAYER_CUSTOM_FRAME                 // Replace frame number selection for your own
    //#define PLAYER_CUSTOM_BG_HIT               // Implement your own bounce.
    //#define ENABLE_CUSTOM_HOTSPOTS             // Add your own hotspots!
    //#define CUSTOM_SPRITE_CELLS                // Use your own sprite cell luts from custom_sprite_cells.h
    //#define ENEMS_CELL_OFFSET         8        // If the above is defined, this is the 1st cell for the enemies.
```

Ver el capítulo 8.

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

`VIEWPORT_X` y `VIEWPORT_Y` son las coordenadas **en caracteres** de la esquina superior izquierda del área de juego. Con `VIEWPORT_X 1` y `VIEWPORT_Y 4`, el área de juego ocupa las columnas 1..30 y las filas 4..23, dejando las cuatro primeras filas para el HUD.

`TEXT_X`, `TEXT_Y` y `TEXT_A` son para el comando `TEXT` del script (que dibuja una línea de texto en el HUD). En Spectrum, `TEXT_A` es el atributo del texto; en CPC no se usa. Ojo: la rutina `draw_line_of_text` **sólo se compila** si defines `TEXT_X` (y `ACTIVATE_SCRIPTING`), así que si usas `TEXT` en el script sin definirlas, el juego no enlazará. Como `TEXT` es una opción "legacy", lo normal es usar `TEXT BOX` y no definir nada de esto.

## Efectos gráficos

```c
    //#define USE_AUTO_SHADOWS                    // Automatic shadows made of darker attributes
    //#define USE_AUTO_TILE_SHADOWS                // Automatic shadows made of specially designed tiles 32-47.
    //#define UNPACKED_MAP                          // Full, uncompressed maps. Shadows settings are ignored.
    #define RLE_MAP                     53        // Use RLE compression (values 44, 54, 62; see docs)
    //#define NO_MASKS            sp_OR_SPRITE     // Sprites are rendered using OR instead of masks.
    //#define PLAYER_ALTERNATE_ANIMATION          // If defined, animation is 1,2,3,1,2,3...
    #define NO_ALT_BG                             // No subs 0 for 19 at random
    //#define HOTSPOTS_FIRST_TILE      32          // Override default value of 16
    //#define REVERSE_OBJECT_COUNT                 // Count objects left rather than objects got
    //#define ENABLE_ANIMATED_TILES      46       // Must be EVEN
    //#define TILANIM_PERIOD             16       // Must be power of two
```

* `RLE_MAP`: el valor es el "formato" del RLE (44, 53 o 62), y tiene que coincidir con el conversor. Si no se define, el mapa es packed o unpacked según el conversor.

* `USE_AUTO_SHADOWS` / `USE_AUTO_TILE_SHADOWS`: sombras automáticas. En la v3.2 el código existe pero **no** tiene las reglas completas de la v4, así que úsalo con caution.

* `NO_ALT_BG`: sin el tile decorativo aleatorio (el 19) en el fondo.

* `ENABLE_ANIMATED_TILES`: tiles animados. El número tiene que ser par, y `TILANIM_PERIOD` una potencia de dos.

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

Todas las velocidades y posiciones están en unidades de **1/64 de píxel**. Los valores de `config.h` ya están en esas unidades (el comentario te dice el equivalente en píxeles).

* `PLAYER_G` es la gravedad: se suma a `vy` en cada frame (en vista lateral). `PLAYER_MAX_VY_CAYENDO` es la velocidad máxima de caída.
* `PLAYER_VY_INICIAL_SALTO` es el impulso inicial del salto; `PLAYER_INCR_SALTO` es la fuerza extra mientras se mantiene pulsada la tecla (y se va apagando); `PLAYER_MAX_VY_SALTANDO` es la velocidad máxima mientras se salta.
* `PLAYER_AX` es la aceleración horizontal, `PLAYER_RX` la fricción (se aplica cuando no se pulsa la dirección) y `PLAYER_MAX_VX` la velocidad máxima.
* `PLAYER_INCR_JETPAC` y `PLAYER_MAX_VY_JETPAC` no se usan (no hay jetpac).

## V. Comportamiento de los tiles

Lo vimos en el capítulo 1. Se define en el array `comportamiento_tiles` de `config.h`, con un valor por tile (48 en total).

---

# Capítulo 7 - El scripting (MSC4)

La v3.2 trae un motor de scripting propio, el **MSC4** (*Mojon Script Compiler 4*), que compila un script escrito en un lenguaje sencillo a un *bytecode* interpretado por un intérprete que se genera automáticamente.

**La especificación completa del lenguaje está en [`msc4.md`](msc4.md)** (en español) y en [`msc4-scripting.md`](msc4-scripting.md) (en inglés). Aquí sólo vamos a ver cómo se integra con el motor de la v3.2.

## El proceso de compilación

El compilador es `utils/msc4.exe`, y se invoca desde `comp.bat`:

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
* `jumptable`: usa una tabla de saltos en vez de un índice para las secciones por pantalla. Es lo habitual en juegos con muchas pantallas y pocas con script.
* `debug`: vuelca la compilación a `msc.txt`. **Úsalo siempre**: es la única manera de ver el bytecode que genera el compilador.

El compilador produce tres cosas:

| Salida | Contenido |
|---|---|
| `script.spt.bin` | El bytecode del script (índice de secciones + código). Se integra con `BINARY "script.spt.bin"`. |
| `text.bin` | Los textos codificados en 5 bits que usan `TEXT BOX` y `DIALOG`. |
| `msc4i.asm` | El intérprete: un fichero de ensamblador Z80 **generado** y **especializado** a los opcodes que usa tu script. |

Ese último punto es la clave: el intérprete no es fijo, se genera con sólo las comprobaciones y los comandos que tu script utiliza. Un script que no use `DIALOG` no gastará ni un byte en código de diálogo.

**No olvides recompilar el intérprete cuando cambies el script.** Y recuerda que el `msc4i.asm` hay que añadirlo a la lista de ficheros que compila `zcc`:

```bat
    zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin msc4i.asm churromain.c -DSPECCY > nul
```

## El interface: `msc4i.h`

El puente entre el motor y el script es `dev/msc4i.h`, un fichero que se escribe a mano (una vez por juego) y que incluye:

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

Fíjate en la última línea: el script **puede mover al jugador** (con `PY = 32`, por ejemplo), y el motor actualiza las coordenadas del jugador después de la llamada.

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

La tecla de acción se configura con `SCRIPTING_DOWN` (abajo), `SCRIPTING_KEY_M` (la M) o, si no se define ninguna, FIRE. El disparo se hace **al pulsar** la tecla, no mientras se mantiene.

Las secciones por pantalla (`ENTERING SCREEN n` y `PRESS FIRE AT SCREEN n`) se pueden desactivar en el motor con `NO_INDEXED_SCRIPTING`, y en el compilador con la directiva `NOINDEXED` del script.

El orden importa: cuando entramos en una pantalla se ejecuta primero `ENTERING ANY` y después `ENTERING SCREEN n`. Lo mismo con la tecla de acción.

## El lenguaje, resumido

Para la gramática completa, [`msc4.md`](msc4.md). Un resumen de lo más importante:

Un script tiene directivas (fuera de las secciones) y secciones. Cada sección contiene cláusulas; cada cláusula es una lista de condiciones `IF` seguidas de acciones, y las acciones sólo se ejecutan si **todas** las condiciones son ciertas:

```
    ENTERING SCREEN 3
        IF %FLAG_LEAVE = 0
        THEN
            SET TILE (7, 4) = 16
            SOUND 7
        END
    END
```

* Las secciones se nombran `ENTERING GAME`, `ENTERING ANY`, `ENTERING SCREEN n`, `PRESS FIRE AT ANY`, `PRESS FIRE AT SCREEN n`, `PLAYER GOT SOMETHING`, `PLAYER KILLS ENEMY`, `SPECIAL TILE TOUCHED` y `ENEMY TOUCHED`. Se pueden agrupar varias pantallas: `ENTERING SCREEN 0, 2, 20`.
* Las variables del script son **flags**, un array de bytes (`flags[MAX_FLAGS]`, 32 por defecto) que vive en el motor. Se referencian como `$n`, o con un alias: `ALIAS %CARRYING = 0` y luego `%CARRYING`.
* `&%ALIAS` da el *número* del flag (útil para hacer tablas de puertas y medallones, como en **Sir Ababol**).
* Los valores especiales (`NPANT`, `PX`, `PY`, `TX`, `TY`, `TN`, `OBJS`, `LIFE`, `KILLED`, `HOTSPOT`, `EN_T`, `EN_X`, `EN_Y`, `OPANT`, `PARAM`) dan acceso al estado del motor. `TX`, `TY` y `TN` son el tile del centro del jugador.
* Las expresiones `TILE AT (x, y)` y `BEH AT (x, y)` leen el mapa; `DIALOG "a", "b", "c"` presenta un menú.
* Los comandos más usados: `SET TILE (x, y) = t`, `SET BEH (x, y) = b`, `INC`/`DEC`, `SOUND n`, `TEXT BOX "..."`, `EXTERN n, m`, `GET ITEM SET $F`, `WARP TO n, x, y`, `BREAK`, `WIN GAME`, `GAME OVER`, `RERUN v` y `CALL sub`.
* Las directivas del script: `ROOMS = N`, `ALIAS %X = N`, `ITEM SLOT = N`, `ITEM EMPTY = N`, `TEXT WIDTH = N`, `NOINDEXED`, `FASTNPANT`.

## `EXTERN`: el puente con el C

El comando `EXTERN n, m` llama a la función `do_extern_action (n, m)` que tú escribes en `extern.h`. Es la forma de hacer cosas que el script no puede:

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

El segundo parámetro (`m`) está ahí para que puedas pasar un dato más (un número de pantalla, un valor...). Si no lo das, vale 0.

## Cajas de texto y diálogos

Con `ENABLE_ENCODED_TEXT` (y `ENABLE_DIALOG`, que lo implica) el motor sabe dibujar texto codificado en 5 bits:

* `TEXT BOX "..."` decodifica el texto y llama a tu `textbox (unsigned char *ptr)`, que dibuja la caja.
* `DIALOG "a", "b", "c"` decodifica las tres opciones y llama a tu `dialog (unsigned char *ptr)`, que debe **devolver en `L`** la opción elegida (1, 2 o 3).

El texto se decodifica en un buffer en `TEXT_BUFFER` (que por defecto es `BASE_FREE`, la memoria libre que queda tras los arrays del motor), y las tres opciones de un diálogo van en 32 bytes cada una.

El texto de las cajas de texto se codifica en 5 bits: las minúsculas se convierten en mayúsculas, los acentos y los caracteres no soportados **se pierden**, y las líneas se parten a `TEXT WIDTH` caracteres (24 por defecto). Los espacios y las comas sí se pueden usar; en los diálogos, en cambio, las comas separan opciones.

## El objeto que llevas: `GET ITEM SET`

La v3.2 tiene una comodidad para los juegos en los que el jugador sólo puede llevar un objeto (una llave, un medallón...). Se combina con las directivas `ITEM SLOT` y `ITEM EMPTY` del script, con la sección `SPECIAL TILE TOUCHED` y con `ITEM_X`/`ITEM_Y`/`ITEM_FLAG` de `config.h`.

* El flag `ITEM SLOT` (por defecto el 0) guarda el número de tile del objeto que llevas. `ITEM EMPTY` (0 por defecto) es el valor que significa "no llevo nada".
* Los objetos se colocan en el mapa como tiles con comportamiento **128**, y el motor dispara `SPECIAL TILE TOUCHED` cuando el jugador los pisa, con `TN` = el tile y `TX`/`TY` = sus coordenadas.
* `GET ITEM SET $F` comprueba que el hueco esté vacío, pone el tile en el hueco, borra el tile del mapa y pone el flag `$F` a 1.
* El motor dibuja el flag `ITEM_FLAG` como un tile en `(ITEM_X, ITEM_Y)`, así que se ve en el HUD.

## Los `flags` y el motor

El array de flags lo usan el motor y el script:

* `flags[ITEM_FLAG]` se dibuja como el "objeto que llevas".
* `flags[OBJECT_COUNT]` se dibuja como contador de objetos.

Y se **borra al empezar cada partida** (no al cambiar de pantalla).

## Victoria y derrota

* `WIN GAME` en el script pone `script_result = 1`, y el motor da la partida por ganada (y hace la pantalla de final).
* `GAME OVER` pone `script_result = 2`, y el motor hace el game over.

**Con `ACTIVATE_SCRIPTING` activado, las condiciones de victoria `PLAYER_NUM_OBJETOS` y `SCR_FIN` se compilan fuera**: la única forma de ganar es con `WIN GAME` en el script.

## El log de compilación

El `msc.txt` que genera `comp.bat` (con `debug`) es tu mejor amigo: en él se ve, línea a línea, los tokens que el compilador ha entendido y el bytecode que ha generado. Si una cosa no hace lo que esperas, es lo primero que hay que mirar.

---

# Capítulo 8 - Código custom

## La API

Las variables globales del motor están en `definitions.h`, y lo que es específico de tu güego va en `custom.h` y `extern.h`.

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
    } INERCIA;

    INERCIA player;
```

Los offsets están en el comentario, y son importantes: el intérprete del script los usa (por eso el intérprete generado no compila si cambias la estructura sin avisar).

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

`attr()` devuelve 0 fuera de la pantalla; la variante `_attr_enems` devuelve 8 (obstáculo), que es lo que usan los enemigos.

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

La v3.2 no tiene los `hook_*` de la v4: tiene un conjunto de funciones mucho más pequeño, en `custom.h`.

### `before_vsync()`

```c
    void before_vsync (void) {
        // Run every game loop right before vsync.
    }
```

Se llama **en cada frame**, justo antes del vsync. Es el sitio para el código "de fondo": cambiar la música, mover algo que no depende del jugador, etc.

### `player_custom_veng()` y `player_custom_heng()`

```c
    #ifdef PLAYER_CUSTOM_VENG
        void player_custom_veng (void) {
            // Modify vy as needed
        }
    #endif
```

Se llaman en cada frame, y sirven para modificar la velocidad vertical (`vy`) y horizontal (`vx`) del jugador antes de moverlo. Con `PLAYER_CUSTOM_VENG` el motor desactiva su propio cálculo de gravedad.

### `player_custom_frame()`

```c
    #ifdef PLAYER_CUSTOM_FRAME
        unsigned char player_custom_frame (void) {
            // Return the frame number (0-7) to use
        }
    #endif
```

Devuelve el frame de animación del jugador (0-7: 0-3 es "hacia la derecha", 4-7 "hacia la izquierda" en vista lateral). Se usa mucho para hacer que el muñeco "mire" en la dirección en la que se mueve, o para que se caiga en vertical.

### `custom_bg_hit()`

```c
    #ifdef PLAYER_CUSTOM_BG_HIT
        unsigned char custom_bg_hit (void) {
            // check hit_v, hit_h and modify player.x / player.y as you need
            // Return 1 if hit registered
        }
    #endif
```

Sustituye por completo la reacción del motor ante los tiles que matan. `hit_v` y `hit_h` indican si el golpe ha sido vertical u horizontal.

### Enemigos custom

Con `ENABLE_CUSTOM_ENEMS` se pueden añadir tipos de enemigo o personalizar los que ya existen:

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

El orden es: `extra_enems_move()` se llama *antes* de las comprobaciones de colisión, y `extra_enems_checks()` *después*.

En `extra_enems_init()` el enemigo vivo está en `malotes [enoffsmasi]`, y el índice en `enit`. En las demás, los valores del enemigo actual están copiados a `_en_x`, `_en_y`, `_en_t`, etc., y puedes modificarlos.

`this_enemy_kills()` es muy útil: si devuelves 0, ese enemigo no hace daño y no se puede aplastar. En **Papaya** se usa para que el "viejo" (el tendero) no mate al jugador.

### Hotspots custom

```c
    #ifdef ENABLE_CUSTOM_HOTSPOTS
        unsigned char custom_hotspots (void) {
            // hotspot type is hotspot_t.
            // set rdi to prevent clearing the hotspot.
        }
    #endif
```

Se llama cuando el jugador toca un hotspot, antes de que se resuelva. `hotspot_t` es el tipo del hotspot (1 = objeto, 2 = llave, 3 = recarga). Si pones `rdi = 1`, el hotspot **no** se borra y el script no se ejecuta (útil para "no puedes coger esto hasta haber hablado con alguien").

## `extern.h`

Si el scripting está activo, `extern.h` debe contener:

```c
    void do_extern_action (unsigned char n, unsigned char m);   // el comando EXTERN

    #ifdef ENABLE_ENCODED_TEXT
        void textbox (unsigned char *ptr);   // el comando TEXT BOX
    #endif

    #ifdef ENABLE_DIALOG
        unsigned char dialog (unsigned char *ptr);   // el comando DIALOG
    #endif
```

`textbox()` recibe el texto ya decodificado en ASCII, y dibuja la caja. `dialog()` recibe las tres opciones (32 bytes cada una) y devuelve la elegida.

## Memoria

En el Spectrum, el motor reserva su memoria con el asignador de bloques de splib2:

```c
    sp_AddMemory (0, NUMBLOCKS, 14, AD_FREE);
```

`NUMBLOCKS` (40) bloques de 14 bytes en `AD_FREE` (61440 - 40*15 = 60840). Cada sprite 16x16 gasta 10 bloques y uno de 8x8, 5. Si usas disparos, hay que subir `NUMBLOCKS` en 5 por cada bala.

En el CPC **no hay memoria dinámica**: las estructuras de sprite son un array estático.

El resto de la memoria está repartida en constantes `BASE_*` (ver `definitions.h`), que en el Spectrum son direcciones fijas a partir de 23300.

---

# Capítulo 9 - La versión CPC

La v3.2 tiene port a CPC, y es el que se está usando actualmente para hacer los juegos nuevos (Papaya, Trabajo Basura, Perils...). Las diferencias principales:

| Zona | Spectrum (`speccy.h`) | CPC (`cpc.h`) |
|---|---|---|
| Pantalla | La de 32x24 caracteres se dibuja directamente; `unpack_screen` desempaqueta en `16384` | Un **superbuffer** en `0x9000` más una zona visible en `0xC000`; `cpc_ShowTileMap(1)` la copia |
| Sprites | splib2: estructuras `sp_SS`, `SPMoveSprAbs` | Estructuras `SPR` de 16 bytes escritas a mano, con punteros `invfunc`/`updfunc` |
| Colores | Atributos de la pantalla | Paletas de hardware de 16 colores: `my_inks` (juego), `pal_general` (área de juego), `pal_hud` (HUD), `palmap[]` (una por pantalla) |
| Música | AY con el player de Shiru (`music.h`) | WYZ (`wyz/wyz_player.h`) con los macros `AY_INIT` / `AY_PLAY_MUSIC` / `AY_STOP_SOUND` |
| Modo | 32x24 caracteres (IM2) | `cpc_SetMode(1)` (MODE 1) o `cpc_SetMode(0)` |
| Memoria dinámica | El asignador de bloques de splib2 | No hay (todo estático) |

## El raster split

En CPC, para tener paletas distintas en el área de juego y en el HUD hace falta un *raster split*:

```c
    #define AUTO_SPLIT     // For CPC, but hud must be 32 raster on top and MODE 1
    #define ALWAYS_SPLIT   // Useful for DIRECT_TO_PLAY
    #define NO_PAL_MAP     // For CPC, with AUTO_SPLIT, only one pal (my_inks)
```

* `AUTO_SPLIT`: el ISR de CPC cambia a la paleta del juego en la línea 3 y a la del HUD a partir de la 6, y ejecuta el player de música.
* `ALWAYS_SPLIT`: el split es permanente (imprescindible con `DIRECT_TO_PLAY`).
* `NO_PAL_MAP`: se usa una sola paleta para todo.

Sin `NO_PAL_MAP`, `cpc_UpdateNow` parchea la paleta por pantalla usando el array `palmap[]` (en `cpc/palmap.h`), que contiene una rutina de paleta por pantalla.

Para que todo esto funcione, las paletas tienen que estar en formato ensamblador (se generan con `mkts_om.exe ... mode=palsasassembly`), y el HUD tiene que estar en las 32 primeras líneas de raster y en MODE 1.

## El loader

El Spectrum usa un loader de una línea (`system/loader.bas`) que carga la pantalla de título, parchea el flag de inversión de caracteres y carga el CODE en 24200.

El CPC usa un loader de dos etapas, ensamblado con `pasmo` y empaquetado en un `.cdt` con `cpc2cdt.exe`:

* `system/preloadercpc.asm`: el preloader, en `0x400`, desactiva las interrupciones, carga el loader real con `cpct_miniload_asm` y salta a él.
* `system/loadercpc.asm`: el loader real, en `$BA00`: pone todas las tintas a negro, pone el modo, carga la pantalla de carga (comprimida con zx7) y la desempaqueta en `$C000`, carga el binario principal (comprimido con zx7) y lo desempaqueta en `$0400`, y salta allí.

Los dos ficheros se generan a partir de las plantillas `*-orig` con `imanol.exe`, que sustituye `%%%scrc_size%%%`, `%%%mainbin_size%%%`, `%%%loader_size%%%` y `%%%loader_mode%%%`.

---

# Apéndice A - Diferencias con la v4

Si vienes de la v4, esto es lo que **no** existe en la v3.2:

* El motor de **cajas empujables** completo (con gravedad, `PUSH_AND_PULL`, cajas que matan...). En la v3.2 sólo hay `PLAYER_PUSH_BOXES`, y es bastante más simple.
* El **motor de la espada** (`ENABLE_SWORD` y su familia). En la v3.2 la macro sólo afecta al tamaño de la estructura del enemigo y al número de sprites del CPC: no hay espada.
* Los **tipos de enemigo** 6 (voladores), 7-10 (cuadradores) y 11-14 (patrulleros marrulleros genitales).
* El motor de **pantallas fijas** (`FIXED_SCREENS`, `SHOW_LEVEL_INFO`, `RESPAWN_REENTER`...).
* El motor de **monedas** (`USE_COINS` y compañía) y el de **persistencia** (`ENABLE_PERSISTENCE`).
* Los **tiles rompibles**, los **tiles resbalosos**, las **zonas malvadas** (comportamiento 3) y las **clasificaciones de enemigos**.
* Los **hooks de código** (`ENABLE_CODE_HOOKS` y la familia `hook_*`). En la v3.2 se sustituyen por `before_vsync()` y los hooks de enemigos custom.
* `CUSTOM_SCREEN_CONNECTIONS` con las cuatro direcciones: en la v3.2 sólo existe `override_flick_down()`.
* `USE_SUICIDE_KEY` y `SHOW_LEVEL_INFO` están **vestigiales**: las macros existen pero no hacen nada. `WIN_ON_SCRIPTING` también es un resto de la v4: en la v3.2 sólo sirve para que `pant_final` no se defina.
* El **scripting** de la v4 (msc) con su lenguaje mucho más rico: la v3.2 usa MSC4 (ver el capítulo 7 y `msc4.md`).

Y al revés, la v3.2 tiene cosas que la v4 perdió:

* `possee` y `gotten`, que son-fields del jugador que te dicen si está apoyado o montado en una plataforma.
* El buffer de mapa por pantalla (`map_attr` + `map_buff`), que hace que los cambios del script se puedan deshacer.
* El modo `FAKE_SIDE_VIEW` para juegos sin gravedad.
* `DEBUG_NAVIGATOR`, un truco para navegar por el mapa (¡y que hay que desactivar antes de publicar!).

# Apéndice B - Reconstruir el compilador de scripts

El compilador `msc4.exe` está escrito en FreeBASIC. Para reconstruirlo:

```
    fbc msc4.bas mtparser.bas cmdlineparser.bas mt5bitEsc.bas
```

Los cuatro ficheros hay que compilarlos juntos (los `.bi` son las declaraciones). Cualquier versión reciente de FreeBASIC sirve.

Después de reconstruirlo, hay que copiar el `msc4.exe` nuevo a la carpeta `utils` de los juegos que lo usen, y regenerar los intérpretes (`.asm`) y el bytecode (`.bin`) de esos juegos.
