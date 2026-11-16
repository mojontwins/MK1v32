# Uwol: gráficos

`work.png` (el tileset) está hecho a mano. El resto lo genera `tools/uwol_conv.py` a partir del original (`../orig`).

## work.png (48 tiles)

| Tiles | Contenido |
|---|---|
| 0-3 | Fondos de las 4 zonas de la pirámide (niveles 1-3, 4-6, 7-8, 9-10) |
| 4-11 | Los 8 tipos de plataforma |
| 12-15 | Moneda sobre cada fondo |
| 17 | Camiseta (hotspot de tipo 1) |
| 18 | Puerta (salidas del último nivel) |
| 20-21 | Flechas de salida |
| 22-24 | Ladrillos de la pirámide: sin visitar, visitada, actual |
| 25-26 | Iconos del marcador |
| 27-30 | Logo |
| 32-35 | Fondos sombreados (`USE_AUTO_TILE_SHADOWS`) |
| 46 | Negro, obstáculo: suelos y techos sobre las columnas laterales |
| 47 | Negro puro (`ts2bin ... pureblack47`): columnas laterales, tapa los sprites |

Los colores de fondo (tiles 0-3, 12-15 y 32-35) dependen de cada pantalla:

- **ZX:** el renderer escribe sus atributos en el tileset en RAM. También pone a 0 los atributos del tile 46, para que tape los sprites como el 47.
- **CPC:** usan solo las tintas 1 (paper) y 2 (ink), y el renderer cambia esas dos tintas en cada pantalla.

## sprites.png (25 celdas)

| Celdas | Contenido |
|---|---|
| 0-7 | Uwol vestido: derecha `uwol_r_1`, `_2`, `_3`, `_3` (salto); izquierda, igual |
| 8-15 | Uwol desnudo (`uwolpelot`), mismo orden |
| 16-21 | Wolfy, Franky, Vampy (2 celdas cada uno; en el original tienen un solo frame) |
| 22-23 | Fanty derecha / izquierda |
| 24 | Uwol muerto |

## gfx/cpc

Placeholders en modo 0 hechos a partir de los gráficos de ZX, a media anchura. La paleta está en `pal.png`:

| Tintas | Uso |
|---|---|
| 0 | Transparente (sprites) |
| 1, 2 | Colores de fondo de la pantalla |
| 3 | Negro |
| 4-10 | Colores brillantes de ZX |
| 11-15 | Rojo, amarillo, cyan, blanco y verde normales |
