# Uwol, Quest for Money (MK1 v3.2)

Conversión de Uwol (Mojon Twins, 2009) a MK1 v3.2, para ZX Spectrum 48K y Amstrad CPC 464.

## Construir

Para ZX:

```
dev/comp.sh   (o comp.bat)
```

Produce `uwol.tap`.

Para CPC:

```
dev/comp_cpc.sh   (o comp_cpc.bat)
```

Produce `uwol_cpc.sna`, `uwol_cpc.cdt` (se carga con `RUN""`) y `uwol_cpc.dsk` (se carga con `RUN"UWOL`).

`tools/uwol_conv.py` es de un solo uso: convierte los datos y los gráficos del original (`orig/`).

## Qué es vanilla y qué es custom

**Vanilla:**

- **Física, salto y colisiones del jugador:** las constantes son las del original.
- **Enemigos:** lineales para Wolfy, Franky y Vampy.
- **Hotspot de la camiseta:** tipo 1 (objeto).
- **Sombras:** `USE_AUTO_TILE_SHADOWS`.
- **Vidas:** vanilla, con marcador vanilla.
- **Golpes:** `PLAYER_BOUNCES` y `PLAYER_FLICKERS`.
- **Game over.**

**Motor:** usa el de `src/dev`, con unos cambios propios de Uwol que no son para reutilizar (buscar `UWOL ONLY`):

- **Por encima del área de juego** (`engine.h`, `mainloop.h`): para que sea más fácil saltar a las plataformas de arriba, Uwol puede subir hasta 15 píxeles por encima de la fila de arriba, por un hueco del techo. `player.y` llega a -15 × 64 y `gpy` vale entonces 241-255. Con -15 sus pies siguen en la fila 0, así que las colisiones horizontales con el techo siguen funcionando: no puede salir del hueco ni andar por encima del techo. El resto sale solo de que `gpy` es de 8 bits: las colisiones de arriba miran la fila 15 (fuera del mapa). Además:
  - al aterrizar con `gpy` en 240 (no debería pasar), `player.y` vale -16 × 64 y no +240 píxeles;
  - ahí arriba no se cogen hotspots: `gpy` 241-255 coincidiría con la `y` de "no hay hotspot" (240), y la camiseta se cogía en cada frame.
- **El sprite ahí arriba** se recorta al área de juego: en ZX, con `and 31` en la fila del sprite (`speccy.h`) y el recorte de siempre; en CPC ya pasa solo, porque la librería no marca las celdas de fuera. En CPC, el tiempo no se imprime mientras Uwol esté encima de sus cifras (si no, saldría su parte oculta), y se imprime al irse (`custom.h`).
- **Sombras de la columna 1** (`speccy.h`, `cpc.h`): la columna 0 (paredes invisibles, sólidas) no echa sombra salvo en la fila 0, para que la esquina (0, 0) sombree entera la (1, 1). Así el tile 46 es sólido desde el principio (`comportamiento_tiles`).

De lo añadido al motor para Uwol, usa:

- la llamada a `player_move_override ()` en `mainloop.h`;
- el reproductor de efectos (ver "Sonido");
- en CPC, `CPC_CUSTOM_SPRITES_POST`: tras pintar los sprites, `cpc_custom_sprites_post ()` (en `custom.h`) quita las marcas de las celdas de las columnas laterales en las filas que ocupan Uwol y el fanty, así que esa parte no se copia a la pantalla y quedan tapados como en ZX.

**Custom** (`custom.h`, `custom_flick.h`, `pantallas.h`):

- **Renderer** (`custom_draw_scr_background`, en asm):
  - Lee los objetos y las monedas de `rooms.h` y monta `map_buff` y `map_attr`.
  - Pone las columnas laterales a negro (47), con suelos y techos (46) donde una plataforma llega al borde.
  - Dibuja con el `draw_and_advance` vanilla.
  - Las monedas se redibujan sin sombra. Para ello su `map_attr` vale 8 mientras se pintan.
- **Hooks:**
  - `entering_game`: estado inicial y pirámide.
  - `entering_screen`: Uwol vestido abajo a la izquierda, tiempo = monedas × 2, marcador.
  - `before_vsync`:
    - el tiempo y el fanty;
    - las monedas;
    - las salidas;
    - los golpes: el primero quita la ropa y saca la camiseta; el segundo mata;
    - las vidas extra a los 1000 y 2500 puntos.
- **Frames:** `player_custom_frame` es la selección vanilla + 8 si Uwol va desnudo.
- **Muerte:** `player_move_override` hace la secuencia de muerte del original: salto, caída fuera de la pantalla y vuelta al inicio parpadeando.
- **Fanty:** es un enemigo custom de tipo 6, con el movimiento original pasado a asm en `extra_enems_move`. Se pone en un hueco libre o en lugar del enemigo 2.
- **Salidas:** cuando se acaban las monedas salen las flechas (puertas en el último nivel) y se quitan los enemigos, salvo el fanty. Se marcan como muertos con `ENEMIES_MAY_DIE`; `init_malotes` los recupera al empezar otra partida.
- **Flicks:** salir por un lado es entrar por el otro (al estar del todo dentro de la columna negra); caerse por abajo mata.
- **Pantallas:** título, pirámide (con tiles), final (un dibujo y dos textos impresos) y game over (vanilla).

## Sonido (ZX)

`SFX_IDLE_PLAYER` (en `config.h`) activa `dev/speccy/sfx_player.h`, que reproduce efectos de beeper sin bloquear el juego.

**Cómo funciona:**

- El bucle principal ya no espera con `halt` a que acabe el frame. Llama a `sfx_idle_wait`, que hace sonar el efecto actual hasta la interrupción que cierra el frame.
- La ISR (50 Hz) avanza el efecto: pasos, deslizamientos de tono, final.
- `peta_el_beeper (n)` llama a `sfx_play (n)`. Así las llamadas del motor dejan de bloquear sin tocar nada más.
- Un efecto solo corta a otro de igual o menor prioridad.
- `espera_activa` y `no_break` también suenan mientras esperan, de modo que los jingles de las pantallas fijas se oyen.
- El código del bucle va al final del binario, fuera de la memoria contendida.

**Tiempo libre:** Uwol deja poco tiempo libre por frame. En el emulador es un ~10%; en una máquina real, menos. Por eso `SFX_MIN_SLICE` asegura un mínimo de sonido por frame, en unidades de ~21 T-states (1500 ≈ 22% del frame).

- Si el juego ya deja ese tiempo libre, no cuesta nada.
- Si no, ese frame se alarga, y el siguiente lo compensa con lo que quede hasta su segunda interrupción.
- Con 0 (o sin definir) solo suena en el tiempo libre.

**Los efectos:**

- Están en `sfx/sfx.txt` (el formato se explica en el propio fichero).
- `utils/sfxcnv` genera `dev/speccy/sfx.h`; `comp.sh` / `comp.bat` lo llaman.
- Los 10 primeros son los de MK1 (`peta_el_beeper` 0-9 en el motor). Los demás son de Uwol: `SFX_COIN`, `SFX_SHIRT`, `SFX_DEATH`, `SFX_FANTY`, `SFX_EXITS`, `SFX_EXTRA_LIFE` y `SFX_ENDING`.

**Para escucharlos:**

- `sfx/preview.sh` / `preview.bat [idle] [slice]` escribe `sfx/wav/*.wav` con la misma temporización que el reproductor.
- Con `idle` y `slice` se oyen como en el juego. Por ejemplo, `preview.sh 10 1500`.

**CPC:** no usa este sistema; sigue con los efectos AY de WYZ. En `custom.h`, los `SFX_*` de Uwol se traducen a los efectos más parecidos.

Los cambios al motor ya están en `src/dev` del repo; los ficheros del motor de `dev/` son copia de los de allí.

## Música y efectos AY (CPC y ZX 128K)

Las canciones originales (`orig/mus`, el reproductor WYZ antiguo de la versión 128K) están convertidas al formato del reproductor WYZ de MK1 v3.2 con `tools/uwol_music.py`. Es de un solo uso y genera:

- `ogt/uwol_*.mus`;
- `dev/wyz/instrumentos.h` (las pautas y las baterías originales);
- `dev/wyz/songs.h` y `dev/wyz/songs_ids.h` (los `MUSIC_*`);
- `dev/wyz/efectos.h`: los efectos originales para CPC;
- `ogt/ram1/ram1.asm`: todo lo del 128K (ver abajo).

Los canales A y B de las canciones están intercambiados: el reproductor de MK1 toca las baterías por el B y el antiguo por el A, así que ahora tapan la misma línea que en el original. Los efectos van por el canal A (el B del original).

`comp_cpc` las comprime con zx0. Los detalles de la conversión están en la cabecera del script. La conversión se ha comprobado decodificando ambos formatos: las 10 canciones dan exactamente las mismas notas, duraciones, instrumentos y baterías, y los canales empiezan donde el reproductor los busca.

| N.º | Canción | Original | Abierta | zx0 | ¿Repite? |
|---|---|---|---|---|---|
| 0 | `MUSIC_TITLE` | UWOLMenu | 579 | 175 | sí |
| 1 | `MUSIC_ZONE1` | UWOLZona1 | 258 | 78 | sí |
| 2 | `MUSIC_GOVER` | UWOLGameOver | 277 | 80 | no |
| 3 | `MUSIC_ZONE2` | UWOLZona2 | 234 | 86 | sí |
| 4 | `MUSIC_ZONE3` | UWOLZona3 | 167 | 95 | sí |
| 5 | `MUSIC_ZONE4` | UWOLZona4 | 186 | 79 | sí |
| 6 | `MUSIC_FANTY` | UWOLFantasma | 517 | 123 | sí |
| 7 | `MUSIC_PYRAMID` | UWOLPiramide | 132 | 82 | sí |
| 8 | `MUSIC_ENDING_KO` | UWOLEndingKO | 240 | 113 | no |
| 9 | `MUSIC_ENDING_OK` | UWOLEndingOK | 355 | 104 | no |

- **En total:** 1015 bytes comprimidas, más 229 de instrumentos y baterías y 20 de la tabla.
- **Numeración:** 0, 1 y 2 son las que toca el MK1 vanilla (título, inicio de partida, game over). Las demás las pone `custom.h`, como el original:
  - la de la zona al entrar en cada pantalla;
  - la del fanty cuando sale;
  - la de la pirámide;
  - la del final bueno o malo.
- **Buffer:** el de la canción descomprimida (`WYZ_SONG_BUFFER`, ahora configurable en `config.h`) solo tiene que caber la mayor, la del título (579 bytes). Pasa de `0x8800` a `0x8DB0`, y termina en `0x9000` como antes.
- **Memoria libre:** el binario ocupa `0x0400-0x6CAA`, así que quedan **8454 bytes** libres hasta el buffer. Antes eran 7257, con las canciones de ejemplo y el buffer en `0x8800`.

Diferencias con el original:

- **Repetición:** el game over y los finales no se repiten (en el original se repetía todo y el juego cortaba la música).

### Efectos

El original tiene 9 efectos (`EFECTO0-8` en `orig/mus/wyzplayer.asm`, 2 bytes por paso). Pasan al formato de MK1 (3 bytes: periodo, periodo alto/volumen, ruido). El juego original usa cuatro: salto (7), golpe (1), camiseta y vida extra (3), moneda (5).

- **CPC:** los 10 números que usa MK1 apuntan al efecto original más parecido (o a uno mudo). Los periodos se escalan al reloj del AY del CPC (1 MHz).
- **ZX 128K:** los mismos números que los efectos del beeper (`sfx/sfx.txt`), así que `peta_el_beeper (n)` toca el beeper en un 48K y el efecto AY en un 128K. Los periodos no se tocan: el reloj es el mismo que el del original.

### ZX 128K (`MODE_128K_DUAL`)

Una sola cinta para 48K y 128K, como el `loader_128` de MK1v4 (ramiro4):

1. BASIC (`system/loader128.bas`; `loader128.tok` es el mismo, tokenizado) carga `system/zxloader.asm` en 24000.
2. El cargador carga sin cabecera la pantalla y el juego (24200).
3. Si es un 128K (al paginar la ROM cambia el byte de `0x0001`), carga el último bloque, `ram1.bin`, en la RAM1 en `0xC000`. En un 48K no lo carga y arranca.

`ogt/ram1/ram1.asm` es el reproductor WYZ de MK1 (el de `dev/wyz/wyz_player.h`) con los puertos del AY del Spectrum, las notas originales, las pautas, baterías y efectos, y las 10 canciones sin comprimir: 5192 bytes, sin buffer de descompresión. Empieza con una tabla de saltos fija, así que no hay que copiar direcciones a `config.h`:

| Dirección | Función |
|---|---|
| `C000` | ISR (cada frame) |
| `C003` | inicialización |
| `C006` | canción (A) |
| `C009` | efecto (A) |
| `C00C` | parar |

En el motor (`speccy.h`, `mainloop.h`, con `MODE_128K_DUAL`):

- **Arranque:** `main` detecta el 128K (`is128k`) y entonces pone la pila bajo el programa (24200), porque con la RAM1 paginada desaparecen `0xC000-0xFFFF`: el final del binario, la pila de siempre y la tabla IM2 de splib2. Después inicializa el reproductor.
- **Paginación:** `SetRAMBank`, la ISR, `ay_play_music`, `ay_stop_sound` y `peta_el_beeper` están por debajo de `0xC000` y paginan con las interrupciones desactivadas.
- **ISR:** en un 128K toca la música y los efectos AY; en un 48K, el `sfx_tick` del beeper.
- **Título:** en un 128K suena la canción original; en un 48K, la de Beepola.

`comp.sh` / `comp.bat` ensamblan `ram1.bin` y el cargador con z80asm y montan la cinta: `tools/gentape.py` en Linux, `GenTape.exe` en Windows, con los mismos bloques.

Comprobado en el emulador sin ROM (con paginación 128K y registro del AY añadidos): en un 128K suena la música en los tres canales y el salto toca el efecto 7 del original por el canal A; en un 48K no se escribe nada en el AY y suenan los efectos del beeper. El cargador usa la rutina de cinta de la ROM y queda por probar en un emulador de verdad.

## Pendiente

- Música de Beepola para el 48K. En un 48K, el título usa la canción de ejemplo.
- Probar la cinta (48K y 128K) en un emulador de verdad.
- Gráficos definitivos del CPC.
- En CPC los sprites no se tapan en las columnas negras: no hay atributos.
