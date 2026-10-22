# MSC4 — Mojon Script Compiler 4
## Especificación del lenguaje de scripts y de su integración con el motor

*Aplica a:* `msc4/msc4.bas` / `msc4/msc4.exe` — versión `msc v4.6.20260729`.
*Implementación de referencia:* `msc4/`.
*Juego de referencia:* `games/papaya` (motor MK1 v3.2, CPC + ZX Spectrum).
*Versión en inglés:* [msc4-scripting.md](./msc4-scripting.md).

> Este documento describe **MSC4**, el "Mojon Script Compiler 4", tal y como está
> realmente implementado en `msc4/msc4.bas`. No es una descripción del
> lenguaje "msc" antiguo documentado en `docs/manual_32.md` (capítulo 7) ni de las
> notas de diseño abandonadas de `msc4/LANGSPECS.md`. Cuando la
> implementación y las notas de diseño no coincidan, **manda la implementación**;
> todos esos casos se señalan en
> [§16. Trucos y errores corregidos](#16-trucos-y-errores-corregidos).

---

## Índice

1. [Para qué sirve MSC4](#1-para-qué-sirve-msc4)
2. [El proceso de compilación](#2-el-proceso-de-compilación)
3. [Línea de comandos](#3-línea-de-comandos)
4. [Estructura léxica](#4-estructura-léxica)
5. [Directivas](#5-directivas)
6. [Las secciones](#6-las-secciones)
7. [Las cláusulas](#7-las-cláusulas)
8. [Valores, expresiones e indirecciones](#8-valores-expresiones-e-indirecciones)
9. [Variables especiales](#9-variables-especiales)
10. [Las comprobaciones](#10-las-comprobaciones)
11. [Las acciones](#11-las-acciones)
12. [Subrutinas](#12-subrutinas)
13. [Texto y diálogos](#13-texto-y-diálogos)
14. [El inventario de un solo hueco](#14-el-inventario-de-un-solo-hueco)
15. [Integración con el motor](#15-integración-con-el-motor)
16. [Trucos y errores corregidos](#16-trucos-y-errores-corregidos)
17. [Ejemplos](#17-ejemplos)
18. [Apéndice — formato del binario](#18-apéndice--formato-del-binario)

---

## 1. Para qué sirve MSC4

MSC4 es un lenguaje de scripts pequeño y deliberadamente tonto que te permite
añadir comportamiento a un juego MK1 sin escribir C ni ensamblador. Es un
lenguaje de *reacción*: el motor dispara un **evento** (el jugador entra en una
pantalla, el jugador pulsa la tecla de acción, el jugador toca un tile
especial…), y el script reacciona comprobando unas pocas condiciones y
ejecutando unas pocas acciones.

El modelo mental es:

```
evento  ──►  sección  ──►  cláusula 1  ──►  cláusula 2  ──►  cláusula 3  ──►  ...
                        (IF…THEN…)    (IF…THEN…)    (IF…THEN…)
```

* Una **sección** está ligada a un tipo de evento. Hay una sección por cada
  tipo de evento, más una sección por pantalla para los dos eventos
  "indexados" (*entrar en una pantalla*, *pulsar acción en una pantalla*).
* Una **sección** contiene una lista ordenada de **cláusulas**.
* Una **cláusula** es una lista de **condiciones** seguida de una lista de
  **acciones**. Las acciones se ejecutan **sólo si todas las condiciones de la
  cláusula son ciertas**.
* Las cláusulas de una sección se evalúan **en orden**. Una cláusula cuyas
  condiciones fallan se salta. Una cláusula puede parar la sección entera con
  `BREAK`.

Lo que MSC4 *no* es: no es un lenguaje de propósito general. No hay bucles, ni
aritmética más allá de `+`/`-` sobre valores de 8 bits, ni funciones
definidas por el usuario salvo `SUB`/`CALL`, ni arrays. Todo lo que no sea
expresable se delega en el motor mediante la acción `EXTERN`.

El estado se guarda en **flags**: un array plano de valores de 8 bits
(`flags[MAX_FLAGS]`) que vive en el motor y se comparte con el script.

---

## 2. El proceso de compilación

`msc4.exe` lee uno o varios ficheros `.spt` y produce:

| Salida | Nombre | Contenido |
|---|---|---|
| Bytecode del script | `<entrada>.bin` (p.ej. `script.spt.bin`) | El índice de secciones seguido del bytecode compilado. Se integra en el juego con `BINARY "script.spt.bin"`. |
| Pool de textos | `text.bin` (opción `text=`) | Los textos codificados en 5 bits que usan `TEXT BOX` y `DIALOG`. Se integra con `BINARY "text.bin"`. |
| Intérprete | el fichero indicado por `interpreter=` (por defecto `msci.asm`) | Un fichero de **ensamblador Z80** que implementa la máquina virtual, especializado a los opcodes que tu script usa realmente. |

El intérprete es *generado*, no fijo: el compilador mantiene una tabla de qué
opcodes de condición y de acción se han usado, y emite código máquina
**sólo para esos**. Un script que nunca use `DIALOG` no contendrá ni un solo
byte de código de diálogo. Ésta es la razón principal de que los scripts de
MSC4 sean baratos.

La disposición de `script.spt.bin` es:

```
+---------------------------+  offset 0
|  índice de secciones      |  2 bytes por entrada, little endian
+---------------------------+  offset 2*indexSize
|  bytecode                 |
+---------------------------+
```

Los detalles están en el [§18](#18-apéndice--formato-del-binario).

### Receta típico de `comp.bat`

Sacado de `games/papaya/dev/comp.bat`:

```bat
cd ..\script
..\utils\msc4.exe in=script.spt v=3 target=%om% interpreter=msc4i.asm text=text.bin jumptable debug >..\dev\msc.txt
copy script.spt.bin ..\dev > nul
move msc4i.asm ..\dev > nul
rem ... y luego hay que añadir msc4i.asm a la línea de zcc
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin msc4i.asm churromain.c -DSPECCY > nul
```

Fíjate en que el mismo `msc4i.asm` generado sirve para ambos targets; lo único
que depende del target es la acción `SHOW`, que llama a `SPUpdateNow` en
Spectrum y a `_cpc_UpdateNow` en CPC.

---

## 3. Línea de comandos

```
msc4.exe in=f1.spt[,f2.spt,...] v=3 target=cpc|zx rooms=N [text=text.bin]
         [interpreter=msci.asm] [noindexed] [fastnpant] [jumptable]
         [debug] [textdebug]
```

| Opción | Obligatoria | Significado |
|---|---|---|
| `in=` | sí | Lista de ficheros `.spt` de entrada, separados por comas. Se produce un `.bin` por fichero de entrada. |
| `v=` | sí | Versión base de MK1 del juego destino. Para este motor, siempre `v=3`. De momento sólo se usa como bookkeeping. |
| `target=` | sí | `cpc` o `zx`. Elige la implementación de `SHOW`. |
| `rooms=N` | no | Número de pantallas del mapa. **Necesario** salvo que el script contenga `ROOMS = N` o se use la opción `jumptable`. |
| `text=` | no | Nombre del fichero con el pool de textos. Por defecto `text.bin`. |
| `interpreter=` | no | Nombre del fichero del intérprete generado. Por defecto `msci.asm`. |
| `noindexed` | no | Tu juego no usará las secciones por pantalla. |
| `fastnpant` | no | Emite un opcode dedicado para `IF NPANT = N` (más rápido, algo más grande). |
| `jumptable` | no | Usa una tabla de saltos en código en lugar del índice para las secciones por pantalla. |
| `debug` | no | Trazaverbose de la compilación (volcado de tokens y de bytecode). |
| `textdebug` | no | Vuelca la codificación en 5 bits de cada texto. |

`debug` es *extremadamente* útil mientras escribes un script: imprime los
tokens que ve el compilador y el bytecode que produce para cada línea. La
misma información se guarda en `msc.txt` en la compilación de papaya.

---

## 4. Estructura léxica

* **No distingue mayúsculas** en ningún sitio: cabeceras de sección, palabras
  clave, nombres de alias. (Los *números* de flag son dígitos, así que por
  supuesto tampoco hay problema.)
* Los **comentarios** empiezan por `#` y llegan hasta el final de la línea. Un
  `#` dentro de una cadena entrecomillada *no* empieza un comentario.
* Los **espacios en blanco** no son significativos, salvo que las palabras
  clave deben estar separadas de sus argumentos.
* Los operadores de comparación `=`, `<`, `>`, `!` pueden escribirse sin
  espacios alrededor: `IF $A=3` y `IF $A = 3` son equivalentes.
* `,`, `(`, `)` y `[`, `]` son *separadores de token* en el fuente, no parte de
  un token. Así que `SET TILE (5,6) = 3` y `SET TILE 5 6 = 3` son lo mismo.
* `_` es separador de token **sólo en las cabeceras de sección** (ver más
  abajo). Dentro de expresiones y comandos es un carácter normal (por eso las
  opciones de un diálogo pueden llevar espacios pero no comas — ver
  [§13](#13-texto-y-diálogos)).
* Cadenas entrecomilladas: `"..."`. Las comillas las quita el tokenizador;
  todo lo que hay entre ellas (espacios, comas, puntuación) se conserva tal
  cual.

### 4.1 El prefijo `ON`

Si una línea empieza por `ON ` (sin distinguir mayúsculas), se quitan esos tres
caracteres antes de analizarla. Existe para que una sección pueda escribirse
`ON TILE PUSHED` en vez de `TILE PUSHED`. (Secciones como `TILE PUSHED` están
reservadas pero *no están implementadas* en esta versión del compilador; ver
[§16](#16-trucos-y-errores-corregidos).)

### 4.2 Las cabeceras de sección se buscan de forma flexible

Las cabeceras de sección se comparan token a token, y `_` es un separador a
ese nivel. Por tanto, todo esto es la misma sección:

```
PRESS FIRE AT ANY
PRESS_FIRE AT ANY
press_fire at any
```

Lo mismo vale para `PLAYER_KILLS_ENEMY` / `PLAYER KILLS ENEMY`,
`PLAYER_GOT_SOMETHING` / `PLAYER GOT SOMETHING`, `SPECIAL_TILE_TOUCHED` /
`SPECIAL TILE TOUCHED`, `ENEMY_TOUCHED` / `ENEMY TOUCHED`.

---

## 5. Directivas

Las directivas se escriben en el nivel superior del script (fuera de cualquier
sección) y surten efecto desde ese punto. Normalmente se ponen al principio.

| Directiva | Sintaxis | Efecto |
|---|---|---|
| `ROOMS` | `ROOMS = N` | Número de pantallas del mapa. Necesario para dimensionar el índice de secciones (salvo que se use `jumptable`, en cuyo caso el índice tiene siempre 16 entradas). También se puede dar en la línea de comandos (`rooms=N`); manda el valor del script. |
| `ALIAS` | `ALIAS %NOMBRE = N` | Liga el nombre `%NOMBRE` al número de flag `N`. Ver [§8.3](#83-los-alias). |
| `ITEM SLOT` | `ITEM SLOT = N` | Qué flag guarda el "objeto que llevas" (ver [§14](#14-el-inventario-de-un-solo-hueco)). El número queda *quemado* en el intérprete. |
| `ITEM EMPTY` | `ITEM EMPTY = N` | Qué valor de tile significa "no llevo nada" (por defecto `0`). La palabra `EMPTY` en el script se sustituye por este número. |
| `TEXT WIDTH` | `TEXT WIDTH = N` | Ancho de línea (en caracteres) que usa el codificador de textos de 5 bits. Por defecto `24`. |
| `NOINDEXED` | `NOINDEXED` | Puramente documental: le dice al compilador (y a ti) que el juego no usa las secciones por pantalla. Ver [§16](#16-trucos-y-errores-corregidos). |
| `FASTNPANT` | `FASTNPANT` | Equivalente a la opción `fastnpant` de la línea de comandos. |

`SUB` no es una directiva en el sentido habitual, pero es una construcción de
nivel superior; ver [§12](#12-subrutinas).

### 5.1 La palabra `EMPTY`

En cualquier sitio donde se espere un valor puedes escribir `EMPTY`; el
compilador lo sustituye por el valor de la directiva `ITEM EMPTY` (por
defecto `0`). Esto hace explícita la intención:

```
%CARRYING = EMPTY      # "no llevo nada"
```

---

## 6. Las secciones

Una sección empieza con su cabecera en una línea propia y termina con una
línea que contenga sólo `END`. El cuerpo de la sección es una secuencia de
cláusulas.

### 6.1 La lista de secciones

| Cabecera | Índice | Se dispara cuando |
|---|---|---|
| `ENTERING GAME` | 0 | Una sola vez, justo antes del primer frame de una partida nueva. |
| `ENTERING ANY` | 1 | Cada vez que el jugador entra en una pantalla (incluida la primera). |
| `ENTERING SCREEN n` | 16 + 2n | Cada vez que el jugador entra en la pantalla `n`. Se ejecuta **después** de `ENTERING ANY`. |
| `PRESS FIRE AT ANY` | 2 | Cada vez que se pulsa la tecla de acción. |
| `PRESS FIRE AT SCREEN n` | 17 + 2n | Cada vez que se pulsa la tecla de acción estando en la pantalla `n`. Se ejecuta **después** de `PRESS FIRE AT ANY`. |
| `PLAYER GOT SOMETHING` | 3 | El jugador recoge un hotspot (un objeto / una moneda, según el juego). |
| `PLAYER KILLS ENEMY` | 4 | Muere un enemigo. |
| `SPECIAL TILE TOUCHED` | 5 | El jugador está sobre un tile cuyo comportamiento tiene el bit 7 puesto (comportamiento `128 + n`). |
| `ENEMY TOUCHED` | 8 | El jugador choca con un enemigo. |

Los índices 6, 7 y 9 están reservados en el layout del índice (`TIMER ZERO`,
`TILE PUSHED`, `TILE DESTROYED`) pero **no** los reconoce esta versión del
compilador: son funcionalidades *planificadas* (ver
[§16.3](#163-secciones-planificadas-pero-no-implementadas)).

`PLAYER GOT SOMETHING` es el nombre canónico de la sección 3. (El
`PLAYER GETS COIN` que menciona el borrador antiguo de `msc4/msc4.md` no
existe; la sección 3 se dispara cuando el jugador recoge un hotspot, que en
la mayoría de los juegos es un objeto o una moneda.)

Una misma pantalla puede aparecer varias veces en una cabecera, en cuyo caso se
usa el *mismo* bytecode para todas:

```
ENTERING SCREEN 0, 2, 20
PRESS FIRE AT SCREEN 2, 7, 12, 17
```

### 6.2 Orden de ejecución

Para un evento dado, el motor llama a las secciones en este orden:

```
ENTERING ANY          →   ENTERING SCREEN n
PRESS FIRE AT ANY     →   PRESS FIRE AT SCREEN n
```

Este es un hecho documentado y *fundamental*: si quieres que un comportamiento
por pantalla sobrescriba a uno global, pon el global en `ENTERING ANY` y
protege el particular con un flag (mira la subrutina `PLACEBOX` en
[§17.1](#171-la-caja-de-papaya)).

### 6.3 ¿Qué tecla es "FIRE"?

La tecla de acción se configura en el `config.h` del motor:

```c
#define ACTIVATE_SCRIPTING     // activa el motor de scripting
#define SCRIPTING_DOWN         // usa DOWN como tecla de acción
//#define SCRIPTING_KEY_M      // usa M como tecla de acción
```

Si no se define ninguna, se usa `FIRE`.

---

## 7. Las cláusulas

El esqueleto de una sección:

```
ENTERING SCREEN 3
	IF <condición>
	IF <condición>
	THEN
		<acción>
		<acción>
	END
	IF <condición>
	THEN
		<acción>
	END
END
```

Reglas de la forma:

* Una línea `IF` empieza una cláusula. **Varias líneas `IF` seguidas van
  acumulando condiciones** (AND lógico).
* `THEN` debe ir en una línea propia y cambia de "recoger condiciones" a
  "recoger acciones".
* `END` en una línea propia **cierra la cláusula actual** y vuelve al estado
  "buscar el siguiente `IF`".
* El `END` final de la sección (cuando no hay ninguna cláusula abierta) cierra
  la sección.

Evaluación:

* Las condiciones de una cláusula se evalúan en orden; en cuanto una es falsa
  la cláusula se salta entera.
* Si todas las condiciones se cumplen, las acciones se ejecutan en orden.
* Después se evalúa la siguiente cláusula. Una cláusula **no** es un `else`:
  varias cláusulas de la misma sección pueden dispararse todas.
* `BREAK` (ver [§11](#11-las-acciones)) aborta la sección entera: no se
  evalúa ninguna cláusula más.

### 7.1 Terminadores

`BREAK`, `WIN GAME`, `GAME OVER` y `RERUN` terminan la cláusula de inmediato:
el compilador no emite el marcador de cierre `0xFF` detrás de ellos (hacen
`RET` en el intérprete) y avisa si les pones más código detrás:

```
Warning! unreachable code @ 13
```

---

## 8. Valores, expresiones e indirecciones

### 8.1 Los tres tipos de valor

| Sintaxis | Significado |
|---|---|
| `123` | Un número literal, `0 … 254`. |
| `$n` | El **contenido** del flag `n`. |
| `%NOMBRE` | Azúcar sintáctico para `$n`, donde `n` es el flag ligado a `%NOMBRE`. |

`FLAG n` es otra forma de escribir `$n`, y compone:

```
FLAG n        == $n
FLAG FLAG n   == $$n
```

(`$` también se escribe `FLAG`, así que `FLAG FLAG FLAG 3` es `$$$3`.) Ojo:
no se permite `$` delante de un alias: escribe `FLAG %NOMBRE`, no `$%NOMBRE`…
excepto que `$%NOMBRE` *sí* se acepta y significa "el flag cuyo número es el
contenido del flag ligado a `%NOMBRE`" (ver [§8.3](#83-los-alias)).

### 8.2 Indirección infinita

Las cadenas de `$` no tienen límite: un `$` seguido de un índice de flag
significa "el contenido de ese flag"; el contenido de un flag es un byte, y si
ese byte es `255` (`$FF`) significa "otra indirección".

El `read_vbyte` del intérprete lo implementa así:

```asm
.read_vbyte
	call read_byte
	cp  0xff
	jr  z, read_vbyte_rec      ; 0xFF = "esto es un flag, no un literal"
	ret
.read_vbyte_rec
	call read_vbyte             ; leer el índice del flag
	...                         ; resolver variables especiales, si no flags [A]
```

Así que la codificación de un valor es:

| Secuencia de bytes | Significado |
|---|---|
| `n` | el literal `n` (0…254) |
| `FF n` | `flags[n]` |
| `FF FF n` | `flags[flags[n]]` |
| … | y así sucesivamente |

### 8.3 Los alias

Un alias da un *nombre* a un número de flag:

```
ALIAS %CARRYING = 0
```

Tras esta directiva, `%CARRYING` se puede usar en cualquier sitio donde se
espere un valor, y significa `$0`.

* El nombre no distingue mayúsculas (`%carrying` = `%CARRYING`).
* El nombre puede llevar letras, dígitos y guiones bajos.
* `&%NOMBRE` da el **número** del flag, no su contenido. Este es el operador
  "dame un puntero", y es lo que hace posible los trucos de
  `SET FLAG`/`GET ITEM` del [§14](#14-el-inventario-de-un-solo-hueco) y el
  código de puertas y medallones de `ababol`:

  ```
  %G = &%MEDA1        # %G := el número de flag de %MEDA1
  $%G = 1             # poner a 1 ese flag
  ```

* Los alias también se pueden introducir *implícitamente*: usar un `%NOMBRE`
  desconocido en el script reserva el primer hueco de flag libre para él e
  imprime la asignación al final de la compilación:

  ```
  %conseguido -> $0
  %total_papayas -> $1
  ```

  El orden de reserva es el orden de primer uso, así que **no te fíes de
  él**: declara explícitamente con `ALIAS` los aliases que te importen.

### 8.4 Expresiones

Hay tres cosas que se pueden usar donde se espera un valor, además de los
valores normales:

```
TILE AT (x, y)     el número de tile en las coordenadas de pantalla dadas
BEH AT (x, y)      el comportamiento del tile en las coordenadas dadas
DIALOG "a", "b", "c"   la opción que elige el jugador (1, 2 o 3)
```

Se escriben exactamente como una asignación a una pseudo-variable:

```
%BOXT = TILE AT (%BOXX, %BOXY)
IF BEH AT (TX, %AUX) <> 0
%RES = DIALOG "SI", "NO", "QUIZAS"
```

Las coordenadas pueden ser a su vez valores (flags, literales, variables
especiales).

`DIALOG` es el caso raro: no es un *valor* en sentido matemático, es una
llamada al motor que saca un menú y devuelve la opción elegida. Aun así se
codifica como un valor, así que se puede guardar en un flag y usar más tarde,
o pasarle directamente a `RERUN`.

---

## 9. Variables especiales

Además de los flags normales (`$0 … $254`), el compilador expone un conjunto
de **variables especiales** respaldadas por el estado del motor. Se codifican
como pseudo-flags en el rango `$E6 … `$FE`.

| Nombre | Pseudo-flag | Significado | Lectura | Escritura |
|---|---|---|---|---|
| `NPANT` (o `N_PANT`) | `$FE` | La pantalla en la que está el jugador. | ✔ | ✔ |
| `PX` | `$FD` | X del jugador, en **píxeles**. | ✔ | ✔ |
| `PY` | `$FC` | Y del jugador, en **píxeles**. | ✔ | ✔ |
| `KILLED` | `$FB` | Número de enemigos matados hasta ahora. | ✔ | ✔ |
| `OBJS` | `$FA` | Número de objetos (hotspots) recogidos. | ✔ | ✔ |
| `LIFE` | `$F9` | Vida del jugador. | ✔ | ✔ |
| `TX` | `$F8` | Tile X del centro del jugador. | ✔ | ✘ |
| `TY` | `$F7` | Tile Y del centro del jugador. | ✔ | ✘ |
| `TN` | `$F6` | El tile en el que está el centro del jugador. | ✔ | ✘ |
| `HOTSPOT` | `$F5` | El tipo del hotspot que el jugador acaba de tocar. | ✔ | ✘ |
| `HIDE_HOTSPOTS` | `$F0` | `scenery_info.hide_hotspots`. | ✔ | ✔ |
| `DONT_MAKE_FANTIES` | `$EF` | `scenery_info.dont_make_fanties`. | ✔ | ✔ |
| `EN_T` | `$EE` | El **tipo** del enemigo que se está tocando (1, 2, 3…). | ✔ | ✔ |
| `EN_N` | `$ED` | El **id** del enemigo que se está tocando. Se lee de la global `enit` del motor, que contiene el índice del enemigo en proceso (0, 1, 2… en la configuración por defecto). | ✔ | ✘ |
| `EN_X` | `$EC` | X del enemigo (píxeles). | ✔ | ✔ |
| `EN_Y` | `$EB` | Y del enemigo (píxeles). | ✔ | ✔ |
| `OPANT` (o `O_PANT`) | `$EA` | La pantalla en la que estaba el jugador antes de entrar en la actual. | ✔ | ✔ |
| `PARAM` | `$E9` | El parámetro pasado a la ejecución actual de la sección. | ✔ | ✔ |
| `TILE AT (x,y)` | `$E8` | Expresión, ver [§8.4](#84-expresiones). | ✔ | ✘ |
| `BEH AT (x,y)` | `$E7` | Expresión. | ✔ | ✘ |
| `DIALOG …` | `$E6` | Expresión. | ✔ | ✘ |

Notas:

* `TX`, `TY` y `TN` describen el *centro* del sprite del jugador
  (`(gpx + 8) >> 4`, `(gpy + 8) >> 4` y el tile en esa posición). El motor los
  refresca en cada frame, así que son válidos en cualquier sección.
* `OPANT` vale `0xFF` mientras la partida está empezando, por eso los scripts
  comprueban `OPANT < 60` antes de fiarse de él (mira `games/ababol`).
* `EN_N` lee la global `enit` del motor. En MK1 v3.2 `enit` es el índice del
  enemigo en proceso, así que `EN_N` vale 0, 1, 2… — *no* es el tipo de
  enemigo (eso es `EN_T`).

---

## 10. Las comprobaciones

Una comprobación es una línea `IF`. La forma general es

```
IF <valor> <operador> <valor>
```

con los operadores `=`, `<`, `>`, `<=`, `>=`, `<>` y `!=` (`!=` y `<>` son lo
mismo).

### 10.1 Comparaciones

| Se escribe | Significado |
|---|---|
| `IF $A = $B` | iguales |
| `IF $A <> $B` | distintos (también `!=`) |
| `IF $A < $B` | menor |
| `IF $A > $B` | mayor |
| `IF $A <= $B` | menor o igual |
| `IF $A >= $B` | mayor o igual |

Los dos operandos pueden ser literales, flags, alias o expresiones. Las
comparaciones son **de 8 bits sin signo**.

### 10.2 `IF TRUE`

`IF TRUE` es la forma idiomática de escribir una cláusula sin condiciones. Se
codifica como el opcode `$F0`, que el intérprete implementa ahora explícitamente
como una condición "siempre cierta".

### 10.3 Comprobaciones del jugador

| Se escribe | Significado |
|---|---|
| `IF PLAYER AT (x, y)` | El **centro** del sprite del jugador está en el tile `(x, y)` (coordenadas de tile). |
| `IF PLAYER TOUCHES (x, y)` | **Idéntico** a `PLAYER AT` en MSC4 (ver la nota de abajo). |
| `IF PLAYER IN_X a, b` | La X del jugador en **píxeles** está entre `a` y `b` (ambos incluidos). |
| `IF PLAYER IN_Y a, b` | La Y del jugador en **píxeles** está entre `a` y `b` (ambos incluidos). |
| `IF PLAYER FALLING` | El jugador está cayendo: `vy > 0` **y** `player.possee` es 0 (o sea, no está apoyado en nada). |
| `IF PLAYER NOT_FALLING` | La negación de lo anterior: `vy <= 0` **o** `player.possee != 0`. También se puede escribir `PLAYER NOT FALLING`. |
| `IF PLAYER STILL` | `vx == 0 && vy == 0`. |

> **`TOUCHES` no es `TOUCHES`.** En el "msc" antiguo, `PLAYER TOUCHES` significaba
> "cualquier parte del sprite solapa el tile". En MSC4 tanto `TOUCHES` como
> `AT` se compilan al mismo opcode, que compara el *tile del centro* del
> jugador con `(x, y)`. Usa `TX`/`TY` (o `PX`/`PY`) si necesitas más
> precisión.
>
> En MK1 v3.2 el sprite del jugador es de 16×16 (exactamente un tile), así que
> el tile del centro es también el único tile en el que puede estar el
> jugador, y las dos formas coinciden en la práctica. La diferencia sólo se
> nota con sprites de jugador más grandes.

> **`IN_X` / `IN_Y` van en píxeles, no en tiles.** El intérprete los compara
> con `gpx`/`gpy`. Para comprobar tiles, usa `TX`/`TY`:
>
> ```
> IF TX = 5
> IF TY = 6
> ```

> **`FALLING` y `NOT FALLING` son negaciones exactas la una de la otra**, y
> ambas miran el `player.vy` completo de 16 bits más el flag `player.possee`.
> Antes de la corrección descrita en
> [§16.2](#162-corregido-en-esta-versión) leían campos equivocados.

### 10.4 `IF NPANT = N`

`IF NPANT = N` es la forma normal de preguntar "¿estoy en la pantalla N?".
Con la directiva `FASTNPANT` (o la opción `fastnpant` de la línea de
comandos) se compila a un opcode dedicado que compara directamente con el
`n_pant` del motor, en lugar de pasar por la maquinaria genérica de
`read_vbyte`.

Sólo se optimiza la forma con `=`; `IF NPANT <> 3` usa el camino genérico.

---

## 11. Las acciones

Las acciones son las sentencias dentro de una cláusula. El lado izquierdo de
una asignación debe ser un *flag* (o una variable especial escribible).

### 11.1 Flags

| Se escribe | Efecto |
|---|---|
| `$A = B` | `flags[A] = B` |
| `SET $A = B` | lo mismo |
| `INC $A, B` | `flags[A] += B` |
| `DEC $A, B` | `flags[A] -= B` |
| `ADD FLAGS x, y` | `flags[x] += flags[y]` (forma antigua, todavía soportada) |
| `SUB FLAGS x, y` | `flags[x] -= flags[y]` (forma antigua, todavía soportada) |

`$A` puede ser un alias (`%A = 3`) o una variable especial escribible
(`PY = 32`). `B` puede ser un literal, un flag, un alias, una variable especial
o una expresión.

La aritmética es de 8 bits y **da la vuelta**: `DEC $A, 1` sobre un flag que
vale 0 da 255.

### 11.2 La pantalla

| Se escribe | Efecto |
|---|---|
| `SET TILE (x, y) = t` | Pone el tile `t` en las coordenadas de pantalla dadas. El comportamiento del tile se toma de la tabla `comportamiento_tiles`. |
| `SET BEH (x, y) = b` | Cambia el *comportamiento* del tile en `(x, y)` sin tocar el tile en sí. |
| `PRINT TILE (x, y) = t` | Dibuja un tile de color sobre la pantalla en `(x, y)` (overlay, no toca el mapa). |
| `WARP TO n, x, y` | Teletransporta al jugador a la pantalla `n`, posición en píxeles `(x*16, y*16)`. |

### 11.3 El jugador

| Se escribe | Efecto |
|---|---|
| `RECHARGE` | Pone la vida del jugador a `PLAYER_LIFE` (la constante de `config.h`; el intérprete generado la referencia por su nombre, así que el valor es el que tengas puesto en tu `config.h`). |
| `SHOW` | Fuerza un refresco de pantalla (`SPUpdateNow` / `_cpc_UpdateNow`). |
| `PAUSE n` | Espera `n` frames (bucle de `halt`). |

### 11.4 Sonido, texto, externos

| Se escribe | Efecto |
|---|---|
| `SOUND n` | Toca el efecto de sonido `n` (`peta_el_beaper`). |
| `TEXT "…"` | Dibuja una línea corta de texto en el marcador (en `TEXT_X`/`TEXT_Y`). La cadena se guarda **literalmente** en el bytecode. |
| `TEXT BOX "…"` | Muestra una caja de texto completa. La cadena se codifica en 5 bits dentro de `text.bin`. |
| `EXTERN n, m` | Llama a la `do_extern_action (n, m)` del motor. El segundo parámetro vale 0 por defecto. Esta es la salida hacia tu propio código C. |
| `PAUSE n` | ver arriba. |

### 11.5 Objetos

| Se escribe | Efecto |
|---|---|
| `GET ITEM SET $F` | Coge el objeto sobre el que está el jugador (ver [§14](#14-el-inventario-de-un-solo-hueco)). |

### 11.6 Decoraciones

```
DECOS
	3, 4, 5
	6, 7, 8
END
```

`DECOS` pinta una lista de tiles de una vez (es lo mismo que escribir varios
`SET TILE`, pero es un único opcode y por tanto más barato). Cada línea es
`x, y, tile`.

El bloque `DECOS` lo termina un `END`, y la cláusula que lo contiene necesita
**su propio** `END` afterwards:

```
IF TRUE
THEN
	DECOS
		3, 4, 5
	END
	SOUND 1
END
```

### 11.7 Control de flujo y estado de partida

| Se escribe | Efecto |
|---|---|
| `BREAK` | Para la ejecución de la sección entera. Las cláusulas restantes no se evalúan. |
| `WIN GAME` | Termina el script y pone `script_result = 1` → el jugador gana. |
| `GAME OVER` | Termina el script y pone `script_result = 2` → el jugador pierde. |
| `RERUN v` | Pone `PARAM` a `v` y reinicia la sección actual desde arriba. |
| `CALL nombre` | Ejecuta la subrutina `nombre` (ver [§12](#12-subrutinas)). |

`BREAK`, `WIN GAME`, `GAME OVER` y `RERUN` son *terminadores*: el compilador
no emite el marcador de cierre de la cláusula detrás de ellos, y avisa de
cualquier código que venga después.

### 11.8 Diálogos

| Se escribe | Efecto |
|---|---|
| `$F = DIALOG "a", "b", "c"` | Presenta un menú con hasta tres opciones y guarda la opción elegida (1, 2 o 3) en `$F`. |

`DIALOG` es una *expresión*; se puede usar directamente en una comparación,
pero el patrón habitual es guardarla y luego hacer `RERUN` con el resultado,
lo que convierte la sección en una máquina de estados (ver
[§17.3](#173-una-máquina-de-estados-con-diálogo)).

---

## 12. Subrutinas

```
SUB MIPROCEDIMIENTO
	IF TRUE
	THEN
		$5 = 1
	END
END
```

y, desde cualquier sitio:

```
CALL MIPROCEDIMIENTO
```

Reglas:

* La subrutina debe estar **definida antes de ser llamada** (el compilador
  resuelve la dirección cuando ve el `CALL`).
* El cuerpo de una subrutina es sintácticamente idéntico al de una sección:
  cláusulas con condiciones y acciones.
* `CALL` se implementa guardando el puntero de script, saltando a la
  subrutina, ejecutando el intérprete recursivamente y restaurando el
  puntero. Un `BREAK` dentro de una subrutina sale de la subrutina solamente.
* Las subrutinas viven en el mismo binario que las secciones; el compilador
  recuerda sus offsets.
* Los nombres no distinguen mayúsculas.

---

## 13. Texto y diálogos

### 13.1 Dos sistemas de texto distintos

| | `TEXT "…"` (legacy) | `TEXT BOX "…"` |
|---|---|---|
| Opcode | `$E3` | `$E6` |
| Almacenamiento | ASCII literal, en línea dentro del bytecode | 5 bits, en `text.bin` |
| Lo dibuja | `draw_line_of_text` (texto del marcador) | `run_text_box` → `textbox()` (una caja enmarcada) |
| Requiere | `TEXT_X` / `TEXT_Y` en `config.h` | `ENABLE_ENCODED_TEXT` en `config.h` |
|Ajuste de línea | no | sí, a `TEXT WIDTH` caracteres |

`TEXT` es legacy: dibuja una línea corta en el marcador, necesita que
`TEXT_X` / `TEXT_Y` estén definidos en `config.h` (papaya los deja
comentados para ahorrar código) y *no* ajusta la línea. `TEXT BOX` es la
forma moderna de mostrar texto.

### 13.2 La codificación de 5 bits

El codificador (`mt5bitEsc.bas`) empaqueta el texto en valores de 5 bits, con
un mecanismo de escape:

| Valor | Significado |
|---|---|
| `0` | ESC — el siguiente valor es un carácter "extendido" |
| `1 … 26` | `A` … `Z` |
| `27` | `,` |
| `28` | `.` |
| `29` | `?` |
| `30` | `!` |
| `31` | espacio |
| ESC `0` | fin de cadena |
| ESC `1 … 30` | ASCII 33 … 62 (`!` … `>`) |
| ESC `31` | salto de línea (lo mete el ajuste de línea) |

Consecuencias:

* **Las minúsculas se convierten en mayúsculas.** Escribe los textos en
  mayúsculas.
* Los caracteres acentuados de los alfabetos español/francés **no están
  soportados** y se descartan en silencio. `VIEJO` vale, `PERA` no.
* Los caracteres fuera del conjunto soportado (por ejemplo `@`, `[`, `\`, `]`,
  `_`, `{`, `|`, `}`) se descartan en silencio.
* El ajuste de línea parte la línea a los `TEXT WIDTH` caracteres (24 por
  defecto) e inserta un salto de línea. Parte por espacios, así que las
  palabras largas no se dividen.
* Los textos idénticos se guardan una sola vez en `text.bin` y se referencian
  por offset.

### 13.3 Diálogos

```
%RES = DIALOG "MAS PAPAYAS", "OTRO LUGAR"
```

* Hasta **tres** opciones. Las opciones que falten se codifican como `$FFFF` y
  el motor las salta al pintar el diálogo.
* Las cadenas de las opciones pasan por el mismo codificador de 5 bits, así que
  aplican las mismas restricciones de caracteres.
* **Las comas son separadores de opciones**: una opción no puede contener
  comas. Los espacios valen tal cual (se codifican con el valor 31 del
  codificador de 5 bits). Además, en las opciones de diálogo un `_` en el
  fuente se convierte en un espacio antes de codificarse, por si prefieres
  escribirlo así.
* El motor debe proporcionar una función `dialog (unsigned char *ptr)` que
  recibe las tres cadenas decodificadas (32 bytes cada una) y devuelve la
  opción elegida en `L` (1, 2 o 3).
* Requiere `ENABLE_DIALOG` en `config.h` (que implica `ENABLE_ENCODED_TEXT`).

---

## 14. El inventario de un solo hueco

MSC4 trae una pequeña comodidad para los juegos en los que el jugador sólo
puede llevar **un** objeto a la vez (una llave, un sujetador, un medallón…).
Es la combinación de las directivas `ITEM SLOT` / `ITEM EMPTY`, la acción
`GET ITEM SET` y la sección `SPECIAL TILE TOUCHED`.

La idea:

* Un flag (el *hueco de objeto*, p.ej. `ITEM SLOT = 0`) guarda el número de
  tile del objeto que lleva el jugador. `ITEM EMPTY` (0 por defecto) quiere
  decir "no lleva nada".
* Otro flag por objeto registra si ya se ha recogido.
* Los objetos se colocan en el mapa como tiles con comportamiento `128` (bit 7
  del comportamiento puesto).
* Cuando el jugador anda sobre uno de esos tiles, el motor dispara
  `SPECIAL TILE TOUCHED`, con `TN` = el número de tile y `TX`/`TY` = sus
  coordenadas.

En el script:

```
SPECIAL TILE TOUCHED
	IF TN = 18
	THEN
		GET ITEM SET %BRA
		SOUND 7
	END
END
```

`GET ITEM SET $F`:

* no hace nada si el hueco de objeto no está vacío;
* si no, pone el número de tile en el hueco de objeto;
* quita el tile de la pantalla (tile 0 en `TX`, `TY`);
* pone el flag `$F` a 1.

La parte del motor son tres líneas en `mainloop.h` que repintan el tile del
objeto en el marcador cada vez que el hueco cambia:

```c
#if defined ACTIVATE_SCRIPTING && defined ITEM_X
	if (flags [ITEM_FLAG] != flag_old) {
		draw_coloured_tile (ITEM_X, ITEM_Y, flags [ITEM_FLAG]);
		flag_old = flags [ITEM_FLAG];
	}
#endif
```

`ITEM_FLAG` en `config.h` debe apuntar al mismo flag que `ITEM SLOT` en el
script.

---

## 15. Integración con el motor

### 15.1 Los ficheros

```
juego/
  script/
    script.spt        <- el script
    script.spt.bin    <- bytecode generado
    text.bin          <- pool de textos generado
  dev/
    msc4i.h           <- la "interfaz" de MSC4 (fichero del framework, no tocar)
    msc4i.asm         <- el intérprete generado
    msc.txt           <- el log de depuración de la compilación (opcional)
    extern.h          <- TU código: los callbacks de EXTERN / TEXT BOX / DIALOG
```

`msc4i.h` es el puente entre el motor y el script, y pertenece al
**framework**: la copia de `src/dev/msc4i.h` (el esqueleto de un juego) es
byte a byte idéntica a la de `games/papaya/dev/`. Proporciona:

* las constantes de número de sección (`SC_ENTERING_GAME`, …);
* el punto de entrada `script (unsigned char n)`;
* el decodificador de textos de 5 bits y el pegamento de `textbox` / `dialog`;
* las directivas `BINARY` que incrustan `script.spt.bin` y `text.bin` en el
  juego.

Eso no significa que sea intocable: **Sir Ababol** (`games/ababol/dev/msc4i.h`)
lo adapta, porque su `decode_text` salta directamente a `_textbox` en vez de
pasar por `run_text_box`, y no usa `DIALOG` en absoluto. Pero si no tienes una
razón para tocarlo, deja el fichero tal cual.

### 15.2 El punto de entrada `script()`

```c
void __FASTCALL__ script (unsigned char a) {
	#asm
			ld  a, l
			ld  (_script_n), a          ; qué sección ejecutar

			ld  a, (_tpx)              ; refresca TN a partir del tile
			ld  c, a                   ;   del centro del jugador
			ld  a, (_tpy)
			call qtile_do
			ld  a, l
			ld  (_tqt), a

			ld  a, 0xff                ; PARAM = $FF al entrar
			ld  (_script_param), a

			call _script_do

			// gpx, gpy pueden haber cambiado, así que actualiza player.x/player.y
			call _shl_player_coords
	#endasm
}
```

`PARAM` vale `$FF` cuando una sección se entra normalmente, y lo que se le
haya pasado a `RERUN` cuando la sección se re-ejecuta. Ésa es toda la
funcionalidad de "reentrada": una sección puede mirar `PARAM` para saber
*por qué* se está ejecutando.

### 15.3 Los enganches del motor

| Evento del motor | Dónde | Llamada |
|---|---|---|
| Partida nueva | `mainloop.h` | `script (SC_ENTERING_GAME)` |
| Entrar en una pantalla | `engine.h` (init de pantalla) | `script (SC_ENTERING_ANY)` y luego `script (SC_ENTERING_SCREEN + (n_pant << 1))` |
| Tecla de acción pulsada | `mainloop.h` | `script (SC_PRESS_FIRE_AT_ANY)` y luego `script (SC_PRESS_FIRE_AT_SCREEN + (n_pant << 1))` |
| Hotspot recogido | `mainloop.h` | `script (SC_PLAYER_GOT_SOMETHING)` |
| Enemigo matado | `engine.h` | `script (SC_PLAYER_KILLS_ENEMY)` |
| Jugador sobre un tile de comportamiento 128 | `engine.h` (bucle de update) | `script (SC_SPECIAL_TILE_TOUCHED)` |
| Jugador toca un enemigo | `engine.h` | `script (SC_ENEMY_TOUCHED)` |

Las llamadas por pantalla están protegidas con `#ifndef NO_INDEXED_SCRIPTING`;
define eso en `config.h` si tu juego no las usa.

### 15.4 Interruptores de `config.h`

```c
#define ACTIVATE_SCRIPTING     // activa el motor de scripting
#define SCRIPTING_DOWN         // (opcional) DOWN es la tecla de acción
//#define SCRIPTING_KEY_M      // (opcional) M es la tecla de acción
//#define NO_INDEXED_SCRIPTING // (opcional) sin secciones por pantalla
#define ENABLE_ENCODED_TEXT     // (opcional) soporte de TEXT BOX
#define ENABLE_DIALOG           // (opcional) soporte de DIALOG
#define ITEM_FLAG 0             // el flag que se muestra en el marcador como "el objeto"
#define ITEM_X 28
#define ITEM_Y 1
```

Ojo: `MAX_FLAGS` **no** está en `config.h`: se define en `dev/churromain.c` (32 por
defecto), y es el tamaño del array `flags[]` que comparten el motor y el script.

Con `ACTIVATE_SCRIPTING` definido, el motor v3.2 termina la partida cuando
`script_result` pasa a 1 (victoria) o 2 (derrota).

### 15.5 El lado del motor de `EXTERN`

`EXTERN n, m` llama a `do_extern_action (n, m)`, que vive en **`extern.h`, un
fichero del juego, no del motor**. El motor sólo la llama: nunca la
implementa.

El esqueleto de un juego (`src/dev/extern.h`) tiene los cuerpos vacíos con
los `#ifdef` ya puestos:

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

Copias ese esqueleto a tu juego y lo rellenas. Un ejemplo completo es
`games/papaya/dev/extern.h`, que implementa las cuatro acciones de
`EXTERN` de **Papaya** (un bitmap `persistence[]` de 640 bytes que recuerda
qué papayas se han recogido en cada pantalla), más la `textbox()` (la caja con
su marco, el redibujado del fondo y la espera de una tecla) y la `dialog()`.

Aquí es donde enganchas el script a tu propio código C:

```c
void do_extern_action (unsigned char n, unsigned char m) {
	switch (n) {
		case 0: /* reset de la persistencia */ break;
		case 1: /* marcar el tile actual como persistente */ break;
		case 2: /* lee la persistencia, borra las papayas */ break;
		case 3: /* actualiza el marcador */ break;
	}
}
```

Fíjate en que `textbox()` y `dialog()` son también **tuyos**: el motor
decodifica el texto y te lo pasa, pero dibujar la caja y presentar el menú es
cosa tuya. Si activas `ENABLE_ENCODED_TEXT` y no escribes una `textbox()`, el
juego no enlazará.

### 15.6 `script_result`

| Valor | Significado | Lo lee el motor como |
|---|---|---|
| 0 | normal | — |
| 1 | `WIN GAME` | el jugador gana |
| 2 | `GAME OVER` | el jugador pierde |

### 15.7 Dos modos de direccionamiento

**Modo indexado** (por defecto). El índice de secciones que va al principio
del binario guarda el offset de cada sección; el intérprete busca ese offset
en una tabla. El índice tiene `16 + 2 * rooms` entradas.

**Modo tabla de saltos** (opción `jumptable`). El índice sólo guarda las 16
secciones "generales"; las secciones por pantalla se encuentran con una cadena
de `cp`/`ret` que genera el compilador. Esto cambia un poco de velocidad por
un índice mucho más pequeño en juegos con muchas pantallas y pocas
scriptadas. Papaya lo usa.

```asm
.script_jump_table
	ld  a, (_script_n)
	ld  hl, 0x00C8      ; dirección absoluta de "ENTERING SCREEN 1"
	cp  18
	ret z
	...
	ld  a, 0xff         ; no encontrado
	ret
```

---

## 16. Trucos y errores corregidos

### 16.1 Trucos que siguen ahí

Son *por diseño* (o al menos, así funciona el lenguaje). Son las cosas que te
van a morder primero.

1. **Una letra detrás de `$` no es un nombre.** `$F` *no* es "el flag F": el
   compilador ahora lo rechaza, pero el comportamiento histórico era evaluar
   `Val("F")`, que vale `0`. Usa dígitos (`$0`, `$15`) o alias
   (`%FLAG_NAME`).

2. **`$%NOMBRE` es una indirección, no un flag.** `ALIAS %G = 3` y luego
   `$%G = 1` pone a 1 el *flag 3*. Es el idioma "dame un puntero" previsto,
   pero es fácil meter la pata.

3. **`PLAYER TOUCHES` es lo mismo que `PLAYER AT`.** Ninguno de los dos
   significa "cualquier píxel del sprite toca el tile". En MK1 v3.2 el sprite
   del jugador es de 16×16 (exactamente un tile), así que en la práctica
   coinciden; la diferencia sólo se nota con sprites más grandes.

4. **`PLAYER IN_X` / `IN_Y` van en píxeles, no en tiles.**

5. **Las minúsculas se pierden en `TEXT BOX` y `DIALOG`.** Escribe en
   mayúsculas.

6. **Las opciones de un diálogo no pueden llevar comas** (son los
   separadores). Los espacios sí valen.

7. **`NOINDEXED` en el script es sólo documental.** Imprime un aviso; no
   cambia el binario. El tamaño del índice lo marca `ROOMS` (o la opción
   `jumptable`). Del lado del motor es `#define NO_INDEXED_SCRIPTING` en
   `config.h`.

8. **`TEXT` (el que no es caja) es legacy.** Dibuja una línea corta en el
   marcador y necesita `TEXT_X` / `TEXT_Y` en `config.h`; si no los defines,
   el juego no compila. `TEXT BOX` es la forma moderna.

9. **`BREAK` aborta la sección entera,** no sólo la cláusula actual.

10. **El código inalcanzable tras un terminador se compila igual** (con un
    aviso). `BREAK`, `WIN GAME`, `GAME OVER` y `RERUN` hacen `RET` en el
    intérprete, así que los bytes de más son peso muerto.

11. **El valor de `ROOMS` tiene que ser correcto** (salvo que compiles con
    `jumptable`, en cuyo caso el índice tiene siempre 16 entradas y `ROOMS`
    no afecta al binario). El índice se dimensiona con él; un valor
    equivocado produce un binario cuyo índice no coincide con lo que espera
    el motor. Pon las directivas al *principio* del script: el tamaño del
    índice se calcula mientras se lee el script, así que una directiva
    `ROOMS` posterior a un `CALL` daría offsets inconsistentes.

12. **`EN_N` lee la global `enit` del motor.** En MK1 v3.2 `enit` es el índice
    del enemigo en proceso, así que `EN_N` vale 0, 1, 2… — *no* es el tipo de
    enemigo (eso es `EN_T`). Si llevas el script a un motor que no tenga
    `enit`, el ensamblador generado no ensamblará.

### 16.2 Corregido en esta versión

El compilador de `/msc4` ha sido corregido. Los cambios se listan aquí
porque algunos cambian el *significado* de cosas que antes estaban rotas (y
silenciosamente) en tus scripts.

| # | Problema | Corrección |
|---|---|---|
| 1 | `PLAYER NOT FALLING` (con espacio) no producía **ningún código**: la condición se descartaba en silencio y la cláusula se ejecutaba como si no existiera. | Ahora se aceptan tanto `NOT_FALLING` como `NOT FALLING`. |
| 2 | `PLAYER FALLING` comprobaba `player.possee` y era cierto cuando valía *distinto de cero*; `PLAYER NOT FALLING` leía el offset 16 (el byte bajo del puntero `current_frame`). | `FALLING` es ahora `vy > 0 && !possee`, `NOT FALLING` es su negación exacta, usando el `vy` completo de 16 bits. |
| 3 | `PARAM = 1` escribía en `flags[$E9]` en lugar de en `script_param`: el handler de lvalue del intérprete se emitía bajo la guarda equivocada. | El handler se emite ahora siempre que `PARAM` se use como lvalue. |
| 4 | Un script que usaba `DIALOG` pero no `TEXT BOX` generaba un intérprete que no ensamblaba (faltaba el helper `read_addr`). | `read_addr` se emite siempre que pueda hacer falta. |
| 5 | `EN_N` leía `_en_it`, un símbolo que no existe en el motor. | Ahora lee `_enit` (la global C `enit`). |
| 6 | Los opcodes de condición `IF TILE AT` / `IF BEH AT` (`$30`/`$31`) eran código muerto: el codificador de expresiones los reescribía siempre. | Se han eliminado los opcodes muertos; `TILE AT` / `BEH AT` son expresiones, y nada más. |
| 7 | `IF TRUE` se codificaba como opcode `$F0` **sin handler**; funcionaba sólo porque los opcodes desconocidos caen por la rama de "no hacer nada". | `$F0` tiene ahora un handler de verdad. |
| 8 | Un comando o una condición desconocidos se **ignoraban en silencio**: el compilador no emitía nada y no decía nada. | Ahora se informa de cada comando y cada condición desconocida, con el número de línea. |
| 9 | `SET TILE (x, y), 7` (coma en vez de `=`) ponía silenciosamente el tile **0**. | Ahora el `=` es obligatorio; si falta, es un error de sintaxis. |
| 10 | Una referencia a flag inválida como `$F` se convertía silenciosamente en el flag 0. | Ahora es un error. |
| 11 | `END IF` se rechazaba con "Unexpected command" y abortaba el análisis de la sección. | `END IF` se acepta como sinónimo de `END`. |
| 12 | Un script con errores seguía produciendo un `.bin` y un intérprete. | El compilador ahora informa de todos los errores, se niega a escribir el `.bin`, se niega a escribir el intérprete y sale con un estado distinto de cero. |

Dos cosas más se han cambiado por claridad, sin alterar el comportamiento de
los scripts correctos:

* El comando `TEXT` (el que no es caja) sigue soportado (es legacy, pero
  funciona).
* `PLAYER GOT SOMETHING` es el nombre canónico de la sección (el
  `PLAYER GETS COIN` del borrador antiguo de `msc4.md` no existe).

### 16.3 Secciones planificadas pero no implementadas

El índice de secciones reserva tres huecos que el compilador todavía no
reconoce:

| Hueco | Sección planificada |
|---|---|
| 6 | `TIMER ZERO` — se dispara cuando el temporizador del juego llega a 0. |
| 7 | `TILE PUSHED` — se dispara cuando el jugador empuja un tile. |
| 9 | `TILE DESTROYED` — se dispara cuando se destruye un tile. |

Escribir hoy una de esas cabeceras hace que el compilador diga "Wrong section
at N" y que la sección se ignore.

---

## 17. Ejemplos

### 17.1 La caja de papaya

De `games/papaya/script/script.spt`. El juego añade una caja (un cajón)
que el jugador puede coger y dejar. La posición de la caja se guarda en tres
flags, y una subrutina hace el pintado de verdad, porque hay que hacerlo tanto
al coger la caja como al soltarla.

```
# Box
# =======

# The box is initialised for each "level" in the screen it first appears if
# %NEWLEVEL = 1. %BOXP, %BOXX, %BOXY, %BOXT control the location and the tile
# it overwrites.
# We use %BOXP = 99 to signify we have the box.

SUB PLACEBOX
	# Make sure the box is placed using the right tile
	IF TRUE
	THEN
		%BOXT = TILE AT (%BOXX, %BOXY)
		%AUX = 33
	END

	IF %BOXT > 0
	IF %BOXT < 7
	THEN
		%AUX = %BOXT
		INC %AUX, 33
	END

	IF TRUE
	THEN
		SET TILE (%BOXX, %BOXY) = %AUX
	END
END
```

Notas:

* Los bloques `SUB` tienen exactamente el mismo aspecto que las secciones.
  Deben aparecer *antes* de los `CALL` que los usan.
* `%BOXT = TILE AT (%BOXX, %BOXY)` lee el tile que hay ahora mismo bajo la
  caja, para poder restaurarlo.
* `IF %BOXT > 0` / `IF %BOXT < 7` implementan "0 < x < 7" (no hay `&&`).

La caja se coge y se suelta en la sección `PRESS FIRE AT ANY`:

```
PRESS_FIRE AT ANY
	# We don't have expressions so...
	IF TRUE
	THEN
		%AUX = TY
		INC %AUX, 1
		%TAUX = TILE AT (TX, %AUX)
	END

	# Get the box
	IF %BOX9 <> 99
	IF %TAUX > 32
	IF %TAUX < 41
	THEN
		%BOXP = 99
		SET TILE (TX, %AUX) = %BOXT
		SOUND 1
		%CARRYING = 33
		BREAK
	END

	# Place the box
	IF %BOXP = 99
	IF BEH AT (TX, %AUX) <> 0
	IF TY > 0
	THEN
		%BOXP = NPANT
		%BOXX = TX
		%BOXY = TY
		DEC PY, 20
		SOUND 1
		CALL PLACEBOX
		%CARRYING = 47
	END

END
```

Notas:

* `BREAK` evita que la cláusula de "soltar la caja" se ejecute en el mismo
  frame que la de "coger la caja".
* `NPANT`, `TX`, `TY`, `PY` son variables especiales usadas como valores
  corrientes.
* `CALL PLACEBOX` devuelve al medio de la cláusula.

### 17.2 Un script completo y autocontenido

```
# ------------------------------------------------------------------
# A tiny example: a door that opens when you press the action key
# while standing on the right tile, and a box you can pick up.
# ------------------------------------------------------------------

ROOMS = 8

# The flag that holds the item we are carrying.
ITEM SLOT = 0
ITEM EMPTY = 47
ALIAS %CARRYING = 0
ALIAS %DOOR_OPEN = 1
ALIAS %HAS_KEY = 2

# Text boxes are wrapped at 22 characters.
TEXT WIDTH = 22

# ------------------------------------------------------------------
# New game: reset everything.
# ------------------------------------------------------------------

ENTERING GAME
	IF TRUE
	THEN
		%CARRYING = EMPTY
		%DOOR_OPEN = 0
		%HAS_KEY = 0
	END
END

# ------------------------------------------------------------------
# Screen 3 has the key; screen 5 has the door.
# ------------------------------------------------------------------

ENTERING SCREEN 3
	IF %HAS_KEY = 0
	THEN
		# The key has behaviour 128, so it is a "special tile".
		SET TILE (7, 4) = 16
	END
END

ENTERING SCREEN 5
	IF %DOOR_OPEN = 0
	THEN
		SET TILE (10, 6) = 17
	END
END

# ------------------------------------------------------------------
# Picking the key up: it is a tile with behaviour 128, so the engine
# fires SPECIAL TILE TOUCHED when we walk over it.
# ------------------------------------------------------------------

SPECIAL TILE TOUCHED
	IF TN = 16
	THEN
		GET ITEM SET %HAS_KEY
		SOUND 7
	END
END

# ------------------------------------------------------------------
# Opening the door: press the action key while standing on it.
# ------------------------------------------------------------------

PRESS FIRE AT SCREEN 5
	IF %DOOR_OPEN = 0
	IF %HAS_KEY = 1
	IF PLAYER AT (10, 6)
	THEN
		%DOOR_OPEN = 1
		SET TILE (10, 6) = 0
		%CARRYING = EMPTY
		SOUND 2
		TEXT BOX "THE DOOR IS OPEN."
	END
END
```

### 17.3 Una máquina de estados con diálogo

La expresión `DIALOG` junto con `RERUN` permite que una sección se comporte
como una corrutina. `PARAM` le dice a la sección en qué "paso" está.

```
# %STATE = 0 : we have not talked to the old man yet
# %STATE = 1 : he asked us to get more fruit
# %STATE = 2 : we have talked twice, we are done

ENTERING GAME
	IF TRUE
	THEN
		%STATE = 0
	END
END

ENEMY TOUCHED
	IF EN_T = 1
	THEN
		# Did we already talk to him?
		IF %STATE = 0
		THEN
			%STATE = 1
			TEXT BOX "BRING ME MORE FRUIT AND WE WILL SPLIT IT."
			%ANSWER = DIALOG "OK", "NO WAY"
			RERUN %ANSWER
		END

		# PARAM is 1 or 2 depending on the option the player chose.
		IF %STATE = 1
		IF PARAM = 1
		THEN
			TEXT BOX "GOOD BOY. GO AND FETCH MORE."
			%STATE = 2
			BREAK
		END

		IF %STATE = 1
		IF PARAM = 2
		THEN
			TEXT BOX "THEN GO AWAY!"
			%STATE = 2
			BREAK
		END
	END
END
```

Cómo funciona:

* `%STATE` es un flag normal: sobrevive entre ejecuciones de la sección.
* `DIALOG` devuelve 1, 2 o 3.
* `RERUN %ANSWER` pone `PARAM` a ese valor y reinicia la sección desde arriba;
  la sección entonces encuentra la rama `IF PARAM = …` que corresponde.
* El `BREAK` al final de una rama para la sección, de modo que las otras
  cláusulas `IF PARAM = …` no se evalúan.

> Recuerda el punto 8 de §16.1: la forma soportada de pasar un parámetro a
> una sección es `RERUN`.

### 17.4 El idioma `&%ALIAS`

De `games/ababol`, donde el mismo código tiene que abrir cuatro puertas
distintas y recoger cuatro medallones distintos, pero el *flag* que hay que
poner sólo se conoce en tiempo de ejecución:

```
# Draw medallions
# Note how we use & to know the actual flag #

ENTERING SCREEN 3
	IF %MEDA1 = 0
	THEN
		SET TILE (8, 1) = 37
		%MX = 8
		%MY = 1
		%K = 37
		%G = &%MEDA1
	END
END
```

y más tarde, cuando el jugador usa el medallón en una cripta:

```
PRESS FIRE AT ANY
	IF %DGX <> 0
	IF PLAYER TOUCHES (%DGX, %GY)
	IF %CARRYING = %K
	THEN
		%CARRYING = EMPTY
		SET TILE (%GX, %GY) = 0
		$%G = 1          # set the flag whose number is in %G
		SOUND 2
	END
END
```

* `&%MEDA1` evalúa al *número* del flag ligado a `%MEDA1`.
* `%G` guarda ese número.
* `$%G = 1` pone a 1 el flag cuyo número está guardado en `%G`.

> El fuente original de **Sir Ababol** escribe `SET TILE (%GX, %GY), 0` con una
> coma en vez de un `=`. Eso ya no compila con la versión actual del
> compilador: el `=` es obligatorio, y su ausencia es un error de sintaxis.
> (El juego se compiló con una versión anterior del compilador, la v4.5.)

---

## 18. Apéndice — formato del binario

### 18.1 El índice de secciones

El índice es una tabla de offsets de 16 bits little endian, uno por hueco de
sección:

| Hueco | Sección |
|---|---|
| 0 | `ENTERING GAME` |
| 1 | `ENTERING ANY` |
| 2 | `PRESS FIRE AT ANY` |
| 3 | `PLAYER GOT SOMETHING` |
| 4 | `PLAYER KILLS ENEMY` |
| 5 | `SPECIAL TILE TOUCHED` |
| 6 | *(reservado: `TIMER ZERO`)* |
| 7 | *(reservado: `TILE PUSHED`)* |
| 8 | `ENEMY TOUCHED` |
| 9 | *(reservado: `TILE DESTROYED`)* |
| 10…15 | reservados |
| 16 + 2n | `ENTERING SCREEN n` |
| 17 + 2n | `PRESS FIRE AT SCREEN n` |

El índice tiene `16 + 2 * rooms` entradas (o sólo 16 en modo `jumptable`). Un
offset de `0` quiere decir "no existe esa sección": el intérprete aborta
enseguida.

Los offsets que se guardan en el índice son **absolutos** (incluyen el tamaño
del índice), porque el intérprete les suma `script_bytecode`.

### 18.2 El bytecode de una sección

Una sección es una secuencia de cláusulas, terminada con `$FF`:

```
clausula  clausula  ...  clausula  $FF
```

y una cláusula es:

```
<longitud> <condición>… $FF <acción>… [$FF]
```

* El byte de longitud le dice al intérprete dónde empieza la siguiente
  cláusula, así que una cláusula se puede saltar en O(1) cuando una condición
  falla.
* El `$FF` que sigue a las condiciones significa "THEN": vienen las acciones.
* El `$FF` que sigue a las acciones significa "fin de cláusula". Se omite si
  la cláusula terminó con un terminador (`BREAK`, `WIN GAME`, `GAME OVER`,
  `RERUN`).

### 18.3 El mapa de opcodes

**Condiciones**

| Opcode | Significado |
|---|---|
| `$01` | `A = B` |
| `$02` | `A < B` |
| `$03` | `A >= B` |
| `$04` | `A <> B` |
| `$05` | `NPANT = N` (sólo con `FASTNPANT`) |
| `$21` | `PLAYER IN_X (a, b)` (píxeles) |
| `$22` | `PLAYER IN_Y (a, b)` (píxeles) |
| `$23` | `PLAYER AT (x, y)` (tile del centro) |
| `$24` | `PLAYER FALLING` (`vy > 0 && !possee`) |
| `$25` | `PLAYER NOT_FALLING` (`vy <= 0 \|\| possee`) |
| `$26` | `PLAYER STILL` |
| `$F0` | `TRUE` |

**Acciones**

| Opcode | Significado |
|---|---|
| `$00` | `FLAGS[a] = v` |
| `$01` | `FLAGS[a] += v` |
| `$02` | `FLAGS[a] -= v` |
| `$20` | `SET TILE (x, y) = t` |
| `$21` | `SET BEH (x, y) = b` |
| `$22` | lista de `DECOS` |
| `$30` | `GET ITEM SET $f` |
| `$50` | `PRINT TILE (x, y) = t` |
| `$6D` | `WARP TO n, x, y` |
| `$E0` | `SOUND n` |
| `$E1` | `SHOW` |
| `$E2` | `RECHARGE` |
| `$E3` | `TEXT "…"` (en línea) |
| `$E4` | `EXTERN n, m` |
| `$E5` | `PAUSE n` |
| `$E6` | `TEXT BOX "…"` (offset dentro de `text.bin`) |
| `$F0` | `WIN GAME` |
| `$F1` | `GAME OVER` |
| `$F2` | `BREAK` |
| `$F3` | `RERUN v` |
| `$F4` | `CALL address` |

### 18.4 El intérprete

El intérprete generado es una pequeña máquina de estados:

```
_script_do      ; buscar la sección (índice o tabla de saltos), fijar el puntero
script_loop     ; leer la longitud de la cláusula, calcular dónde va la siguiente
script_clausule ; leer y evaluar las condiciones
script_actions  ; leer y ejecutar las acciones
```

Leer un valor (`read_vbyte`) entiende el marcador de indirección `$FF` y
resuelve las variables especiales; leer un *destino* (`read_i_v`) resuelve las
variables especiales escribibles a sus direcciones en el motor.

Como el compilador sólo emite los handlers de los opcodes que tu script usa,
el intérprete es tan pequeño como tu script permita. Si usas un opcode nuevo,
regenera el intérprete: el juego no funcionará con uno viejo.

### 18.5 Reconstruir el compilador

El compilador está escrito en FreeBASIC. Para reconstruirlo:

```
fbc msc4.bas mtparser.bas cmdlineparser.bas mt5bitEsc.bas
```

Los cuatro ficheros hay que compilarlos juntos (los `.bi` son las
declaraciones). Sirve cualquier versión reciente de FreeBASIC; el compilador
se desarrolló con la 1.08.1.

Tras reconstruirlo, recuerda copiar el nuevo `msc4.exe` a la carpeta `utils`
de los juegos que lo usen, y regenerar los intérpretes (`.asm`) y el bytecode
(`.bin`) de esos juegos.
