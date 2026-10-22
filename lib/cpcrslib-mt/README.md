# CPCRSLIB "Mojon Tilemap" (cpcrslib-mt)

Versión recortada de CPCRSLIB, **orientada al motor MTE MK1 v3.2**. No es la
CPCRSLIB original: es sólo la porción que el motor usa de verdad.

Original version by Artaburu. Modificaciones para el tilemap y los *dirty
cells* (bitfield) por na_th_an.

## Alcance

Quedan 50 módulos (`cpcrslib.lst`):

* **Núcleo**: `cpc_ShowTileMap`, `cpc_UpdScr`, `cpc_ShowTouchedTiles`,
  `cpc_ResetTouchedTiles`, `cpc_UpdTileTable`, `cpc_InvalidateRect`,
  `cpc_KeysData`, `cpc_TestKeyboard`, `cpc_TestKey`, `cpc_SetMode`,
  `cpc_MakeM1RotationLUTs`.
* **Dependencias internas** (no se declaran en la cabecera): `cpc_Bit2Mask`,
  `cpc_TblLookup`, `cpc_bitleft2mask`, `cpc_bitright2mask`.
* **Sprites**: la matriz completa de tamaños y formatos de pixel que el
  generador de assets (`mkts_om.exe mode=sprites`) puede pedir: 4x8, 8x8,
  8x16, 8x24, 16x16, 16x24, 12x24, 2Bx8, 4Bx16, 4Bx24, en sus variantes
  `Px` / `PxM1` / `GPx` / `G` / `LUT` / `CA`.

Se ha eliminado todo lo que no es tilemap ni *dirty cells*: gestión de teclado
(`cpc_AssignKey`, `cpc_DeleteKeys`, `cpc_RedefineKey`, `cpc_ScanKeyboard`,
`cpc_AnyKeyPressed`, `cpc_TestKeyF`), impresión de texto y gráficos
(`cpc_PrintStr`, `cpc_PrintGphStr*`, `cpc_PutTiles`, `cpc_GetTiles`,
`cpc_TouchTiles`, `cpc_GetScrAddress*`, `cpc_PutSp*` genéricos), el gestor de
tilemaps (`cpc_UpdateTileMap`), los actualizadores de pantalla alternativos
(`cpc_UpdScrM1P`, `cpc_UpdScrP`) y `cpc_SetColour`.

Las rutinas eliminadas siguen en el historial de git, por si hay que
resucitar alguna.

## Contrato con el motor

La librería no es autonomous: usa símbolos que le aporta el juego.

* `dev/system/tilemap_conf.asm` (se ensambla con el juego) exporta:
  `tiles`, `pantalla_juego`, `tiles_tocados`, `tiles_tocados_ptr`,
  `posiciones_pantalla`, `posiciones_super_buffer`,
  `posicion_inicial_area_visible`, `posicion_inicial_superbuffer`,
  `ancho_pantalla_bytes`, `alto_pantalla_bytes`,
  `ancho_pantalla_bytes_visible` y `behindtilemasks` (éste último, sólo en la
  variante FG; ver más abajo).
* `dev/cpc.h` exporta `viewport_x` y `viewport_y`, que se usan para el
  *clipping* (`CHURRERA_CLIPPING`).
* `dev/cpc.h` importa con `LIB`: `cpc_KeysData`, `cpc_UpdTileTable`,
  `cpc_InvalidateRect`, `cpc_TestKeyboard`.

### `behindtilemasks` (variante FG)

Con `PASARPORDETRAS` (variante "fg"), la librería dibuja los sprites **por
detrás** de los tiles que el juego marque como "en primer plano". Para
saberlo, lee una tabla de **256 bytes, uno por tile**, en el símbolo
`behindtilemasks`:

* `0` → tile normal: el sprite va por delante.
* `1` → tile en primer plano: el sprite va por detrás.

La tabla **la pone el juego**, en `dev/system/tilemap_conf.asm`:

```asm
    .behindtilemasks
    XDEF behindtilemasks
    ; TODO: generate this automaticly
        defb 0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0
        ...                                ; 256 bytes
```

Los juegos que usan la variante "fg" (**Papaya** y **Sir Ababol**) la
rellenan. Los que usan la variante normal no la necesitan: la librería no
la mira. El esqueleto de `src/` la deja vacía, porque enlaza con la
variante normal.

## Variantes

* `cpcrslib_mt.lib`: la normal (`CPCconfig_normal.def`).
* `cpcrslib_fg_mt.lib`: la variante "fg" (`CPCconfig_fg.def`), que activa
  `PASARPORDETRAS` para que los sprites puedan quedar detrás de ciertos
  tiles. La usan **Papaya** y **Sir Ababol**. Necesita la tabla
  `behindtilemasks` (ver más abajo).

## Reconstruir

```
make.bat        REM variante normal
make.bat fg     REM variante fg
```

`make.bat` copia la configuración correspondiente a `CPCconfig.def`, ensambla
todos los módulos de `cpcrslib.lst` con `z80asm -x`, genera las dos librerías
y las copia a `c:\z88dk10-orig\lib\clibs`. `borra.bat` borra los `.obj` y
`.sym` intermedios.

El `CPCconfig.def` versionado es el de la variante normal; `make.bat` lo
restablece al terminar.

Para instalar la cabecera: `copy cpcrslib_mt.h c:\z88dk10-orig\include`.

## Nota

Las 35 rutinas de sprite se eligen por tamaño desde el
`spriteset_mappings.h` que genera `mkts_om.exe`, así que la matriz completa
tiene que estar en la librería: si un juego usa un tamaño o un formato de
pixel que no esté listado arriba, hay que recuperar el módulo correspondiente
del historial de git.
