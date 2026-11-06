# zxheadless / cpcheadless

Spectrum 48K mínimo y sin pantalla para probar juegos de z88dk desde la línea
de comandos: carga binarios directamente en RAM, simula teclas por frames,
saca capturas de pantalla y vuelca memoria.

    make
    ./zxheadless -l yun.bin@24200 -f 400 -k 60-63:1 -k 150-200:P,Q -s 300:shot.ppm
    python3 sheet.py hoja.png 4 shot*.ppm        # hoja de contacto (PIL)

Opciones: `-l fichero@dir`, `-pc`, `-sp` (por defecto 24200 / 24199, como
`CLEAR 24199: RANDOMIZE USR 24200`), `-f frames`, `-k F1-F2:teclas`,
`-s F:fichero.ppm`, `-every N:prefijo`, `-poke dir=val`, `-pokeat F:dir=val`,
`-dump dir:long@F`, `-prof` (PCs más calientes), `-border`.

Limitaciones, a sabiendas:

- **Sin ROM.** La zona 0-16383 está llena de `RET` y en 0x38 hay `EI; RET`.
  Basta para el crt0 de z88dk (que sólo hace `CALL 5633`) y para juegos que
  montan su propia IM2 (splib2). Lo que dependa de verdad de la ROM no va.
- **Sin contención de memoria** ni temporización exacta de la ULA: los tiempos
  son T-states sin contender, así que el juego va algo más rápido que en una
  máquina real (calcula un 10-20 % de margen).
- Sin sonido (los `OUT` al beeper se ignoran, salvo el borde).
- El núcleo Z80 incluye los opcodes no documentados habituales (IXH/IXL,
  SLL, DDCB con copia a registro) y los flags X/Y; no se ha pasado zexall.

Para descubrir direcciones, usa el `.map` de z88dk:
`grep -E "^_PLAYER\s" juego.map`.

## cpcheadless (Amstrad CPC 464)

    ./cpcheadless -sna juego.sna -f 500 -k 120-124:1 -k 300-360:P -s 420:shot.ppm

Carga `.sna` (v1-v3, como los de `cpctbin2sna`/`tools/mksna.py`) o un binario
(`-l fichero@dir -pc dir`). Emula lo justo para juegos que apagan el firmware:
Gate Array (tintas, modo, contador de interrupciones de 52 líneas con resincronía
en el VSYNC y su ACK), registros del CRTC (la pantalla se pinta con R1, R6, R9,
R12/R13; VSYNC con R7/R3), PPI + AY para el teclado (registro 14). Guarda tintas
y modo por línea, así que los raster splits (HUD con otra paleta) salen bien.
Cada instrucción se redondea a NOPs enteros (tiempos del CPC); 312 líneas de 64.

Teclas: A..Z, 0..9, SPACE, RETURN, ENTER, ESC, TAB, SHIFT, CTRL, COPY, DEL,
UP, DOWN, LEFT, RIGHT, F0..F9, J_UP, J_DOWN, J_LEFT, J_RIGHT, J_FIRE1, J_FIRE2.

Medidas:

- `-mark dir` (repetible): imprime `frame línea nop` cada vez que el PC pasa por
  esa dirección. Con las direcciones del `.map` de z88dk se ve en qué línea del
  haz empieza cada fase del bucle y cuánto dura.
- `-scrwrites F1-F2`: por frame, cuántas escrituras a la memoria de pantalla
  caen en la fila de caracteres que el haz está pintando en ese momento (riesgo
  de tearing).
- `-prof`, `-dump`, `-pokeat`, `-every`, como en `zxheadless` (hasta 4096
  `-dump`).
- `-beam`: las capturas enseñan cada línea tal como la pintó el haz (se
  copia de memoria al empezar la línea) en vez de la memoria de pantalla al
  final del frame. Sirve para ver el tearing de un render que compite con el
  haz.

Sin ROMs (si el juego activa una, avisa), sin sonido, CRTC tipo 0 simplificado.
Validado con `games/papaya` de MK1 v3.2 (el `.sna` publicado y uno recompilado).
