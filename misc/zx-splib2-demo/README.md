# splib2 demo -- ZX Spectrum

Ejemplo **standalone** que usa la librería de sprites **splib2** (Sprite Pack
2.2, la de `lib/splib2/`) exactamente igual que lo hace el motor MTE MK1
v3.2:

- el fondo vive en la *display list* de splib2 y lo pinta `SPUpdateNow`, que
  sólo redibuja las celdas *sucias*;
- los gráficos de los tiles se registran en la tabla de tiles de splib2
  (`SPTileArray`), de modo que la display list sólo guarda números de tile;
- los sprites son sprites de splib2: `sp_CreateSpr` + `sp_AddColSpr`, se
  mueven con `sp_MoveSprAbs` y se recortan al área visible;
- la memoria de las estructuras de sprite sale del asignador de bloques de
  splib2 (`sp_AddMemory` / `sp_BlockAlloc`), enganchado con los punteros
  `u_malloc` / `u_free` que la librería espera;
- la animación usa el truco del *desplazamiento de bitdef* del motor: el
  frame nuevo está a distancia constante del viejo, y esa distancia se le
  pasa a `sp_MoveSprAbs` como parámetro `animate`.

Qué hace: llena la pantalla de tiles al azar y mueve dos sprites de 16x16
por encima.

## Compilar

```
build.bat
```

Necesita el z88dk (`zcc`) y las herramientas de `..\..\src\utils`
(`bas2tap.exe`, `bin2tap.exe`). Genera `demo.tap`.

Equivalente a mano:

```
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o demo.bin main.c
```

## Mapa de memoria

El ejemplo usa las mismas direcciones fijas que el motor (ver
`src/dev/speccy.h`):

- `23296`: el contador de frames que incrementa el ISR.
- `24200`: el binario principal (`-zorg=24200`).
- `60840-61439`: la memoria del asignador de bloques de splib2 (`AD_FREE`).
- `61440-61696`: la tabla de vectores IM2 de splib2.
- `61936`: la pila (`STACK_ADDR`).
- `61952-65535`: las tablas de rotación horizontal de splib2.

Por eso el binario tiene que empezar en 24200: si no, se solaparía con la
tabla IM2 y con las tablas de rotación, que están en direcciones fijas.

## Detalles del código

- `system_init()` deja el ZX en modo IM2 con el ISR genérico de splib2
  (`SPInitIM2`, `SPCreateGenericISR`, `SPRegisterHook`) y nuestro contador
  de frames como *hook*.
- `init_tiles()` genera los 256 tiles de 8x8 y los registra con
  `sp_TileArray()`.
- `random_background()` pinta la display list con `sp_PrintAtInv()`.
- `build_frame()` construye los bitdefs de los sprites: cada columna son 24
  líneas de 2 bytes (gráfico, máscara), 16 líneas del sprite y 8 de
  relleno, porque splib2 desplaza el puntero hasta 7 líneas para el
  posición sub-pixel vertical.
- `move_sprites()` mueve los sprites y alterna el frame: el frame nuevo
  está a `SPR_FRAME_BYTES` de distancia, así que el desplazamiento que se
  le pasa a `sp_MoveSprAbs` sirve para las dos columnas del sprite.

Ver [`docs/lib-splib2.md`](../../docs/lib-splib2.md) para los detalles de
la API.
