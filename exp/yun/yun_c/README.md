# Yun ZX y CPC, tercera carga (fases 7-8)

Las fases 7 y 8 de Yun (NES v5), con el mismo motor, el mismo `custom.h` y la misma
cadena de herramientas que la primera carga (ver su README para el formato de los
recursos, el render de CPC y los cambios en MK1).

- **Fase 7:** mapa 6 de NES (6x4 pantallas), 62 KO, 3 cerrojos, cañones y pezones.
- **Fase 8:** mapa 7 (9x3), 56 KO, sin cerrojos, a oscuras.

## Fase 8, a oscuras (`YUN_DARK_LEVEL`)

- Cada sala se ve oscura hasta que se coge su bombilla (hotspot tipo 4, tile 20); desde
  entonces, con luz para siempre (`lit []`).
- Mientras está oscura, de vez en cuando cae un rayo: la sala se ve un momento con sus
  colores.
- **ZX:** la sala se repinta con `DARK_ATTR` (azul sobre negro, en `yun_level.h`) con
  `TILE_ATTR_MASK`; los sprites salen del mismo color. Repintar cuesta un frame cada vez.
- **CPC:** solo cambian las tintas de fondo (`pal_dark.png`, y `pal_light.png` para el
  rayo).
- `tsx.png` lleva 5 tiles: 16-18 como siempre, 19 sin usar, 20 la bombilla.

## Tamaño

- **ZX:** 33640 bytes, de 24200 a 57840: quedan 1,5 KB.
- **CPC:** 29811 bytes, de 0x0400 a 0x7873.
- La zona de nivel es de 27 pantallas (el mapa de la fase 8): 2975 bytes en ZX.

## Enemigos nuevos (custom.h, con `YUN_EXTRA_ENEMS`)

| MK1 | NES | Qué hace | Se atrapa |
|---|---|---|---|
| 7 pezón | 0xAx (zurully) | Espera en (x1, y1) 36 frames y salta con la tabla `pezon_incs` reescalada (`pz_y`) | sí, cuenta para KO |
| 8 monococo | 0x9B | El monococo de la pared: igual que el 6 con otras celdas de escondido/asomando | sí, fuera |
| 9 gyrosaw | 0xBx | Da vueltas a un cuadrado de 32 px; en sentido horario si la trayectoria del ponedor va a algún sitio (mx distinto de 0) | no; quita vida siempre, no se rebota encima |
| 10 cañón | 0x80 | Dispara un coco recto cada N segundos: dirección y N salen de la trayectoria y la velocidad del ponedor | no |

- **Coco:** sigue habiendo uno a la vez para monococos y cañones (en NES, hasta 3).
- **Lineales:** tres clases por fase (`spritesN.png`). Si la fase de NES tiene más,
  las de menos enemigos se pintan con las celdas de otra: lo dice `nes2load.txt`.
- Los parámetros salen del `.ene`: se editan en el ponedor como cualquier enemigo.

**Celdas fijas** (`sprites.png`, 32): las 22 de la carga 1 (las tuyas), más 22-23
monococo de la pared (escondido, asomando), 24-25 pezón (y atrapado), 26-27 gyrosaw,
28-31 cañón mirando a izquierda, arriba, derecha, abajo. Las nuevas son placeholders;
en CPC, todas.

## Mapas

Cada fase tiene su ancho de mapa (`VARIABLE_MAP_W`, `lvl_map_w` en `yun_load.h`): los
mapas van tal cual salen del conversor, sin rellenar a 4x6. La zona de nivel tiene sitio
para el más grande y `mapcnv`/`ene2h` rellenan los demás (`screens=`).

Arenas movedizas de NES (bit 2): aquí plataforma normal (4). Cintas (bit 32): las mueve
`custom_pre_move`, como en la carga 1.

## Compilar

Igual que la carga 1 (`dev/comp.bat`, `dev/comp_cpc.bat` y sus `.sh`); arriba de cada
script está lo propio de esta carga. Después, `python3 tools/check_tiles.py dev zx|cpc`
comprueba los tiles (opcional, solo para probar).

Los mapas, enemigos y placeholders los hizo una vez `tools/nes2load.py` (como
`nes2png.py` en la carga 1), que no forma parte de la compilación: `nes2load.txt` tiene
lo que sacó para las cabeceras escritas a mano.

## Probado (zxheadless, cpcheadless)

- Todas las fases cargan en ZX y CPC.
- Ganar fase → siguiente → final → título (forzando los KO).
- Gyrosaws dando vueltas y cañones disparando.
- Fase 8: sala oscura y rayos (ZX y CPC), sin coger la bombilla.
- **No probado:** jugar las fases enteras, los cerrojos, recoger hotspots, las cintas.
