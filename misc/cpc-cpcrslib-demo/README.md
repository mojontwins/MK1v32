# cpcrslib-mt demo -- Amstrad CPC

Ejemplo **standalone** que usa la librería **CPCRSLIB "Mojon Tilemap"**
(`cpcrslib-mt`, la de `lib/cpcrslib-mt/`) exactamente igual que lo hace el
motor MTE MK1 v3.2:

- la librería manda en la pantalla: el fondo vive en la nametable (`$100`)
  y los sprites se componen en el superbuffer (`$9000`); a la zona visible
  (`$C000`) sólo se copian las celdas *tocadas*;
- las celdas que cambian se marcan en un bitfield de 96 bytes con
  `cpc_UpdTileTable` / `cpc_InvalidateRect`;
- cada sprite tiene un par de rutinas: una que marca las celdas que cubre
  (`invfunc`) y otra que lo dibuja (`updfunc`);
- el `cpc_UpdateNow()` **del propio motor** se usa tal cual para todo lo
  anterior (está copiado en `main.c`).

Qué hace: llena la pantalla de tiles al azar y mueve dos sprites de 16x16
por encima.

## Compilar

```
build.bat
```

Necesita el z88dk (`zcc`) y `..\..\src\utils\cpctbin2sna.exe`. Genera
`demo.sna`.

Equivalente a mano:

```
zcc +cpc -m -vn -unsigned -zorg=1024 -lcpcrslib_mt -o demo.bin tilemap_conf.asm main.c -DCPC -DMODE_1
```

`tilemap_conf.asm` es (casi) una copia del del motor
(`src/dev/system/tilemap_conf.asm`): es el contrato con la librería, le
dice dónde están la nametable, el superbuffer, la zona visible y el
bitfield de celdas tocadas.

## Mapa de memoria

Las direcciones fijas son las de la librería (y las del motor):

- `$0100`: la nametable.
- `$0400`: el binario principal (`-zorg=1024`).
- `$9000`: el superbuffer.
- `$C000`: la zona visible.
- `$CE00`: el bitfield de celdas tocadas.
- `$D700`, `$DE00`, `$E700`: las LUTs de rotación de modo 1
  (`cpc_MakeM1RotationLUTs`).
- `$E600`: las estructuras de sprite.
- `$FE00`: la LUT de píxeles transparentes.

El ejemplo genera esa LUT en tiempo de ejecución (`init_lut()`) en vez de
descomprimirla de un binario, que es lo que hace el motor.

## Detalles del código

- **No hay variables locales.** z88dk deja los locales de una función en la
  pila, y el código que genera para llegar a ellos es lento (hay que
  cargar el puntero de pila y desplazarlo en cada acceso). El motor
  trabaja sólo con globales, y este ejemplo hace lo mismo: todos los
  locales se han convertido en globales, y cada función tiene las suyas
  propias para que no se pisen entre ellas. Los parámetros de las
  funciones propias del ejemplo también son globales (el `cpc_UpdateNow()`
  del motor, copiado aquí, usa el global `upd_sprites` en vez del
  parámetro `sprites`).
- El modo 1 empaqueta 4 píxeles de 2 bits en cada byte, en el orden
  "garbled" del CPC: el bit alto del píxel de la izquierda está en el bit
  3, y su bit bajo en el 7. `encode_4()` lo hace con los 4 píxeles del
  global `enc_px[]`.
- La LUT de transparencia dice, para cada byte posible de un sprite, qué
  bits del fondo se conservan: los píxeles de color 0 (transparentes) no
  tocan el fondo. `keep_mask()` la genera; es la misma tabla que el motor
  tiene en `src/dev/system/cpc_TrPixLutM1.asm`.
- Las rutinas de sprite elegidas son las de modo 1 al pixel:
  `cpc_PutSpTileMap16x16PxM1` (marca las celdas) y
  `cpc_PutTrSp16x16TileMapPxM1LUT` (dibuja). `cx` va en píxeles (0..255) y
  `cy` en scanlines.
- La librería recorta las marcas de celda al *viewport* (30x20 celdas), que
  el motor exporta como `viewport_x` / `viewport_y`.

Ver [`docs/lib-cprslib.md`](../../docs/lib-cprslib.md) para los detalles de
la API.
