# MSC4 — Mojon Script Compiler 4
## Specification of the scripting language and of its integration with the engine

*Applies to:* `msc4.bas` / `msc4.exe` — version string `msc v4.6.20260729`.
*Reference implementation:* `tests/msc4/`.
*Reference game:* `games_old/papaya` (MK1 v3.2 engine, CPC + ZX Spectrum).
*Spanish version:* [msc4.md](./msc4.md).

> This document describes **MSC4**, the "Mojon Script Compiler 4", as it is actually
> implemented in `tests/msc4/msc4.bas`. It is *not* a description of the older
> "msc" language documented in `docs/manual.md` (chapter 7) nor of the abandoned
> `tests/msc4/LANGSPECS.md` design notes. Where the implementation and the design
> notes disagree, **the implementation wins**; every such case is flagged in
> [§16 Gotchas, and the bugs that were fixed](#16-gotchas-and-the-bugs-that-were-fixed).

---

## Table of contents

1. [What MSC4 is for](#1-what-msc4-is-for)
2. [The build pipeline](#2-the-build-pipeline)
3. [Command line interface](#3-command-line-interface)
4. [Lexical structure](#4-lexical-structure)
5. [Directives](#5-directives)
6. [Sections](#6-sections)
7. [Clauses](#7-clauses)
8. [Values, expressions and indirections](#8-values-expressions-and-indirections)
9. [Special variables](#9-special-variables)
10. [Conditions](#10-conditions)
11. [Actions](#11-actions)
12. [Subroutines](#12-subroutines)
13. [Text and dialogs](#13-text-and-dialogs)
14. [The one-slot inventory aid](#14-the-one-slot-inventory-aid)
15. [Engine integration](#15-engine-integration)
16. [Gotchas, and the bugs that were fixed](#16-gotchas-and-the-bugs-that-were-fixed)
17. [Examples](#17-examples)
18. [Appendix — binary format](#18-appendix--binary-format)

---

## 1. What MSC4 is for

MSC4 is a small, deliberately dumb scripting language that lets you add behaviour
to an MK1 game without writing C or assembly. It is a *reaction* language: the
engine fires an **event** (the player entered a screen, the player pressed the
action key, the player touched a special tile…), and the script reacts to it by
testing a few conditions and performing a few actions.

The mental model is:

```
event  ──►  section  ──►  clause 1  ──►  clause 2  ──►  clause 3  ──►  ...
                        (IF…THEN…)   (IF…THEN…)   (IF…THEN…)
```

* A **section** is bound to one kind of event. There is one section per kind of
  event, plus one section per screen for the two "indexed" events
  (*entering a screen*, *pressing action on a screen*).
* A **section** contains an ordered list of **clauses**.
* A **clause** is a list of **conditions** followed by a list of **actions**.
  The actions run **only if every condition in the clause is true**.
* The clauses of a section are evaluated **in order**. A clause whose conditions
  fail is skipped. A clause may stop the whole section with `BREAK`.

What MSC4 is *not*: it is not a general-purpose language. There are no loops, no
arithmetic beyond `+`/`-` on 8-bit values, no user-defined functions other than
`SUB`/`CALL`, no arrays. Everything that is not expressible is delegated to the
engine through the `EXTERN` action.

State is kept in **flags**: a flat array of 8-bit values (`flags[MAX_FLAGS]`) that
lives in the engine and is shared with the script.

---

## 2. The build pipeline

`msc4.exe` reads one or more `.spt` files and produces:

| Output | Name | Contents |
|---|---|---|
| Script bytecode | `<input>.bin` (e.g. `script.spt.bin`) | The section index followed by the compiled bytecode. Embedded into the game with `BINARY "script.spt.bin"`. |
| Text pool | `text.bin` (option `text=`) | The 5-bit encoded texts used by `TEXT BOX` and `DIALOG`. Embedded with `BINARY "text.bin"`. |
| Interpreter | the file given by `interpreter=` (default `msci.asm`) | A **Z80 assembly** file implementing the virtual machine, specialised to the opcodes your script actually uses. |

The interpreter is *generated*, not fixed: the compiler keeps a table of which
condition opcodes and which action opcodes were used, and emits machine code
**only for those**. A script that never uses `DIALOG` will not contain a single
byte of dialog code. This is the main reason MSC4 scripts are cheap.

The `.spt.bin` layout is:

```
+---------------------------+  offset 0
|  section index            |  2 bytes per entry, little endian
+---------------------------+  offset 2*indexSize
|  bytecode                 |
+---------------------------+
```

See [§18](#18-appendix--binary-format) for the details.

### Typical `comp.bat` recipe

From `games_old/papaya/dev/comp.bat`:

```bat
cd ..\script
..\utils\msc4.exe in=script.spt v=3 target=%om% interpreter=msc4i.asm text=text.bin jumptable debug >..\dev\msc.txt
copy script.spt.bin ..\dev > nul
move msc4i.asm ..\dev > nul
rem ... and then add msc4i.asm to the zcc command line
zcc +zx -m -vn -unsigned -zorg=24200 -lsplib2 -o %game%.bin msc4i.asm churromain.c -DSPECCY > nul
```

Note that the *same* generated `msc4i.asm` is used for both targets; the only
target-dependent bit is the `SHOW` action, which calls `SPUpdateNow` on the
Spectrum and `_cpc_UpdateNow` on the CPC.

---

## 3. Command line interface

```
msc4.exe in=f1.spt[,f2.spt,...] v=3|4|5 target=cpc|zx rooms=N [text=text.bin]
         [interpreter=msci.asm] [noindexed] [fastnpant] [jumptable]
         [debug] [textdebug]
```

| Option | Mandatory | Meaning |
|---|---|---|
| `in=` | yes | Comma-separated list of input `.spt` files. One `.bin` is produced per input file. |
| `v=` | yes | Base MK1 version of the target game (3, 4 or 5). Currently only used for bookkeeping. |
| `target=` | yes | `cpc` or `zx`. Selects the `SHOW` implementation. |
| `rooms=N` | no | Number of screens in the map. **Required** unless the script contains `ROOMS = N` or the `jumptable` option is used. |
| `text=` | no | Name of the text pool file. Default `text.bin`. |
| `interpreter=` | no | Name of the generated interpreter file. Default `msci.asm`. |
| `noindexed` | no | Your game will not use the per-screen sections. |
| `fastnpant` | no | Emit a dedicated opcode for `IF NPANT = N` (faster, slightly bigger). |
| `jumptable` | no | Use a code-based jump table instead of the index for the per-screen sections. |
| `debug` | no | Verbose trace of the compilation (token dump, bytecode dump). |
| `textdebug` | no | Dump the 5-bit encoding of every text. |

`debug` is *extremely* useful while writing a script: it prints the tokens the
compiler sees and the bytecode it produces for every single line. The same
information is written to `msc.txt` in the papaya build.

---

## 4. Lexical structure

* **Case insensitive** everywhere: section headers, keywords, alias names.
  (Flag *numbers* are digits, so they are trivially case-insensitive.)
* **Comments** start with `#` and run to the end of the line. A `#` inside a
  quoted string does *not* start a comment.
* **Whitespace** is not significant, except that keywords must be separated from
  their arguments.
* The comparison operators `=`, `<`, `>`, `!` may be written without surrounding
  spaces: `IF $A=3` and `IF $A = 3` are equivalent.
* `,`, `(`, `)` and `[`, `]` are *token separators* in the source, not part of a
  token. So `SET TILE (5,6) = 3` and `SET TILE 5 6 = 3` are the same thing.
* `_` is a token separator **in section headers only** (see below). Inside
  expressions and commands it is an ordinary character (this is why dialog
  options may contain spaces but not commas — see [§13](#13-text-and-dialogs)).
* Quoted strings: `"..."`. The quotes are removed by the tokenizer; everything
  between them (spaces, commas, punctuation) is kept verbatim.

### 4.1 The `ON` prefix

If a line starts with `ON ` (case-insensitive), the three characters are
stripped before parsing. This exists so that a section may be written
`ON TILE PUSHED` instead of `TILE PUSHED`. (Sections such as `TILE PUSHED` are
reserved but *not implemented* in this version of the compiler — see
[§16](#16-gotchas-and-the-bugs-that-were-fixed).)

### 4.2 Section headers are matched loosely

Section headers are matched token by token, and `_` is a separator at that
level. Therefore all of these are the same section:

```
PRESS FIRE AT ANY
PRESS_FIRE AT ANY
press_fire at any
```

The same holds for `PLAYER_KILLS_ENEMY` / `PLAYER KILLS ENEMY`,
`PLAYER_GOT_SOMETHING` / `PLAYER GOT SOMETHING`, `SPECIAL_TILE_TOUCHED` /
`SPECIAL TILE TOUCHED`, `ENEMY_TOUCHED` / `ENEMY TOUCHED`.

---

## 5. Directives

Directives are written at the top level of the script (outside any section) and
take effect from that point on. They are usually placed at the very beginning.

| Directive | Syntax | Effect |
|---|---|---|
| `ROOMS` | `ROOMS = N` | Number of screens in the map. Needed to size the section index. Can also be given on the command line (`rooms=N`); the script value wins. |
| `ALIAS` | `ALIAS %NAME = N` | Binds the name `%NAME` to flag number `N`. See [§8.3](#83-aliases). |
| `ITEM SLOT` | `ITEM SLOT = N` | Which flag holds the "carried item" (see [§14](#14-the-one-slot-inventory-aid)). The number is *burned into* the interpreter. |
| `ITEM EMPTY` | `ITEM EMPTY = N` | Which tile value means "no item" (default `0`). The word `EMPTY` in the script is replaced by this number. |
| `TEXT WIDTH` | `TEXT WIDTH = N` | Word-wrap width (in characters) used by the 5-bit text encoder. Default `24`. |
| `NOINDEXED` | `NOINDEXED` | Purely documentary: tells the compiler (and you) that the game does not use the per-screen sections. See [§16](#16-gotchas-and-the-bugs-that-were-fixed). |
| `FASTNPANT` | `FASTNPANT` | Equivalent to the `fastnpant` command-line switch. |

`SUB` is *not* a directive in the usual sense, but it is a top-level construct;
see [§12](#12-subroutines).

### 5.1 The `EMPTY` keyword

Anywhere a value is expected you may write `EMPTY`; the compiler substitutes the
value of the `ITEM EMPTY` directive (default `0`). This makes the intent
explicit:

```
%CARRYING = EMPTY      # "I am carrying nothing"
```

---

## 6. Sections

A section begins with its header on a line of its own and ends with a line
containing only `END`. The body of the section is a sequence of clauses.

### 6.1 The list of sections

| Header | Index | Fires when |
|---|---|---|
| `ENTERING GAME` | 0 | Once, right before the first frame of a new game. |
| `ENTERING ANY` | 1 | Every time the player enters a screen (including the first one). |
| `ENTERING SCREEN n` | 16 + 2n | Every time the player enters screen `n`. Runs **after** `ENTERING ANY`. |
| `PRESS FIRE AT ANY` | 2 | Every time the action key is pressed. |
| `PRESS FIRE AT SCREEN n` | 17 + 2n | Every time the action key is pressed while on screen `n`. Runs **after** `PRESS FIRE AT ANY`. |
| `PLAYER GOT SOMETHING` | 3 | The player picks up a hotspot (an object / a coin, depending on the game). |
| `PLAYER KILLS ENEMY` | 4 | An enemy dies. |
| `SPECIAL TILE TOUCHED` | 5 | The player stands on a tile whose behaviour has bit 7 set (behaviour `128 + n`). |
| `ENEMY TOUCHED` | 8 | The player collides with an enemy. |

Indices 6, 7 and 9 are reserved in the index layout (`TIMER ZERO`, `TILE PUSHED`,
`TILE DESTROYED`) but are **not** recognised by this version of the compiler:
they are *planned* features (see [§16.3](#163-sections-that-are-planned-but-not-implemented)).

`PLAYER GOT SOMETHING` is the canonical name of section 3. (The
`PLAYER GETS COIN` mentioned in the old `tests/msc4/msc4.md` draft does not
exist; section 3 fires when the player picks up a hotspot, which in most games
is an object or a coin.)

A screen may be listed several times in a header, in which case the *same*
bytecode is used for all of them:

```
ENTERING SCREEN 0, 2, 20
PRESS FIRE AT SCREEN 2, 7, 12, 17
```

### 6.2 Order of execution

For a given event the engine calls the sections in this order:

```
ENTERING ANY          →   ENTERING SCREEN n
PRESS FIRE AT ANY     →   PRESS FIRE AT SCREEN n
```

This is a documented and *load-bearing* fact: if you want a per-screen
behaviour to override a global one, put the global one in `ENTERING ANY` and
guard the per-screen one with a flag (see the `PLACEBOX` sub in
[§17.1](#171-the-papaya-box)).

### 6.3 Which key is "FIRE"?

The action key is configurable in the engine's `config.h`:

```c
#define ACTIVATE_SCRIPTING     // enables the scripting engine
#define SCRIPTING_DOWN         // use DOWN as the action key
//#define SCRIPTING_KEY_M      // use M as the action key
```

If neither is defined, `FIRE` is used.

---

## 7. Clauses

The skeleton of a section:

```
ENTERING SCREEN 3
	IF <condition>
	IF <condition>
	THEN
		<action>
		<action>
	END
	IF <condition>
	THEN
		<action>
	END
END
```

Rules of the shape:

* An `IF` line starts a clause. **Several `IF` lines in a row accumulate
  conditions** (logical AND).
* `THEN` must be on a line of its own and switches from "collecting conditions"
  to "collecting actions".
* `END` on a line of its own **closes the current clause** and returns to
  "looking for the next `IF`".
* The final `END` of the section (when no clause is open) closes the section.

Evaluation:

* The conditions of a clause are evaluated in order; as soon as one is false the
  clause is skipped entirely.
* If all conditions hold, the actions are executed in order.
* The next clause is then evaluated. A clause is **not** an `else`: several
  clauses of the same section may all fire.
* `BREAK` (see [§11](#11-actions)) aborts the whole section: no further clause
  is evaluated.

### 7.1 Terminators

`BREAK`, `WIN GAME`, `GAME OVER` and `RERUN` end the clause immediately: the
compiler does not emit the closing `0xFF` marker after them (they `RET` from
the interpreter) and it prints a warning if you put more code after them:

```
Warning! unreachable code @ 13
```

---

## 8. Values, expressions and indirections

### 8.1 The three kinds of value

| Syntax | Meaning |
|---|---|
| `123` | A literal number, `0 … 254`. |
| `$n` | The **content** of flag `n`. |
| `%NAME` | Syntactic sugar for `$n` where `n` is the flag bound to `%NAME`. |

The flag index must be a number: `$0`, `$15`, `$254`. A *letter* after the `$`
is not a name (`$F` used to silently mean flag 0; now it is an error). Use an
alias if you want a name.

`FLAG n` is an alternative spelling of `$n`, and it composes:

```
FLAG n        == $n
FLAG FLAG n   == $$n
```

(`$` is also spelled `FLAG`, so `FLAG FLAG FLAG 3` is `$$$3`.) Note that `$` is
*not* allowed in front of an alias: write `FLAG %NAME`, not `$%NAME`… except
that `$%NAME` *is* accepted and means "the flag whose number is the content of
the flag bound to `%NAME`" (see [§8.3](#83-aliases)).

### 8.2 Infinite indirection

`$` chains are unbounded: `$` followed by a flag index means "the content of
that flag"; the content of a flag is itself a byte, and if that byte is `255`
(`$FF`) it means "another indirection".

The interpreter's `read_vbyte` implements this:

```asm
.read_vbyte
	call read_byte
	cp  0xff
	jr  z, read_vbyte_rec      ; 0xFF = "this is a flag, not a literal"
	ret
.read_vbyte_rec
	call read_vbyte             ; read the flag index
	...                         ; resolve special variables, else flags [A]
```

So the encoding of a value is:

| Byte sequence | Meaning |
|---|---|
| `n` | the literal `n` (0…254) |
| `FF n` | `flags[n]` |
| `FF FF n` | `flags[flags[n]]` |
| … | and so on |

### 8.3 Aliases

An alias gives a *name* to a flag number:

```
ALIAS %CARRYING = 0
```

After this directive, `%CARRYING` may be used anywhere a value is expected, and
it means `$0`.

* The name is case insensitive (`%carrying` = `%CARRYING`).
* The name may contain letters, digits and underscores.
* `&%NAME` yields the **number** of the flag, not its content. This is the
  "give me a pointer" operator, and it is what makes the
  `SET FLAG`/`GET ITEM` tricks of [§14](#14-the-one-slot-inventory-aid) and
  the `ababol` gate/medallion code possible:

  ```
  %G = &%MEDA1        # %G := the flag *number* of %MEDA1
  $%G = 1             # set that flag to 1
  ```

* Aliases may also be introduced *implicitly*: using an unknown `%NAME` in the
  script allocates the first free flag slot for it and prints the assignment at
  the end of the compilation:

  ```
  %conseguido -> $0
  %total_papayas -> $1
  ```

  The order of allocation is the order of first use, so **do not rely on it**:
  always declare the aliases you care about explicitly with `ALIAS`.

### 8.4 Expressions

Three things can be used where a value is expected, in addition to the plain
values above:

```
TILE AT (x, y)     the tile number at the given screen coordinates
BEH AT (x, y)      the behaviour of the tile at the given screen coordinates
DIALOG "a", "b", "c"   the option selected by the player (1, 2 or 3)
```

They are written exactly like an assignment to a pseudo-variable:

```
%BOXT = TILE AT (%BOXX, %BOXY)
IF BEH AT (TX, %AUX) <> 0
%RES = DIALOG "YES", "NO", "MAYBE"
```

The coordinates may themselves be values (flags, literals, special variables).

`DIALOG` is the odd one out: it is not a *value* in the mathematical sense, it
is a call to the engine that pops up a menu and returns the chosen option. It
is nevertheless encoded as a value, so it can be stored in a flag and used
later, or fed straight to `RERUN`.

---

## 9. Special variables

Besides the ordinary flags (`$0 … $254`), the compiler exposes a set of
**special variables** that are backed by engine state. They are encoded as
pseudo-flag numbers in the range `$E6 … `$FE`.

| Name | Pseudo-flag | Meaning | Read | Write |
|---|---|---|---|---|
| `NPANT` (or `N_PANT`) | `$FE` | The screen the player is on. | ✔ | ✔ |
| `PX` | `$FD` | Player X, in **pixels**. | ✔ | ✔ |
| `PY` | `$FC` | Player Y, in **pixels**. | ✔ | ✔ |
| `KILLED` | `$FB` | Number of enemies killed so far. | ✔ | ✔ |
| `OBJS` | `$FA` | Number of objects (hotspots) collected. | ✔ | ✔ |
| `LIFE` | `$F9` | Player's life. | ✔ | ✔ |
| `TX` | `$F8` | Tile X of the player's centre. | ✔ | ✘ |
| `TY` | `$F7` | Tile Y of the player's centre. | ✔ | ✘ |
| `TN` | `$F6` | The tile the player's centre is on. | ✔ | ✘ |
| `HOTSPOT` | `$F5` | The type of the hotspot the player just touched. | ✔ | ✘ |
| `HIDE_HOTSPOTS` | `$F0` | `scenery_info.hide_hotspots`. | ✔ | ✔ |
| `DONT_MAKE_FANTIES` | `$EF` | `scenery_info.dont_make_fanties`. | ✔ | ✔ |
| `EN_T` | `$EE` | The **type** of the enemy being touched (1, 2, 3…). | ✔ | ✔ |
| `EN_N` | `$ED` | The **id** of the enemy being touched. Read from the engine's `enit` global, which holds the index of the enemy being processed (0, 1, 2… in the default configuration). | ✔ | ✘ |
| `EN_X` | `$EC` | Enemy X (pixels). | ✔ | ✔ |
| `EN_Y` | `$EB` | Enemy Y (pixels). | ✔ | ✔ |
| `OPANT` (or `O_PANT`) | `$EA` | The screen the player was on before entering the current one. | ✔ | ✔ |
| `PARAM` | `$E9` | The parameter passed to the current run of the section. | ✔ | ✔* |
| `TILE AT (x,y)` | `$E8` | Expression, see [§8.4](#84-expressions). | ✔ | ✘ |
| `BEH AT (x,y)` | `$E7` | Expression. | ✔ | ✘ |
| `DIALOG …` | `$E6` | Expression. | ✔ | ✘ |

\* writing to `PARAM` is **broken** in this version of the compiler; see
[§16](#16-gotchas-and-the-bugs-that-were-fixed).

Notes:

* `TX`, `TY` and `TN` describe the *centre* of the player sprite
  (`(gpx + 8) >> 4`, `(gpy + 8) >> 4` and the tile at that position). They are
  refreshed by the engine every frame, so they are valid in every section.
* `OPANT` is `0xFF` while the game is starting, which is why scripts test
  `OPANT < 60` before trusting it (see `games_old/ababol`).
* `EN_N` reads the engine symbol `_en_it`, which **does not exist** in the
  stock MK1 v3.2 engine. If you use `EN_N` you must define it yourself, or the
  generated assembler will not assemble. `EN_T` is safe.

---

## 10. Conditions

A condition is one `IF` line. The general form is

```
IF <value> <operator> <value>
```

with the operators `=`, `<`, `>`, `<=`, `>=`, `<>` and `!=` (`!=` and `<>` are
the same thing).

### 10.1 Comparison conditions

| Written | Meaning |
|---|---|
| `IF $A = $B` | equal |
| `IF $A <> $B` | different (also `!=`) |
| `IF $A < $B` | less than |
| `IF $A > $B` | greater than |
| `IF $A <= $B` | less than or equal |
| `IF $A >= $B` | greater than or equal |

Both operands may be literals, flags, aliases or expressions. Comparisons are
**unsigned 8-bit**.

### 10.2 `IF TRUE`

`IF TRUE` is the idiomatic way to write a clause with no conditions. It is
encoded as opcode `$F0`, which the interpreter now implements as an explicit
"always true" condition.

### 10.3 Player conditions

| Written | Meaning |
|---|---|
| `IF PLAYER AT (x, y)` | The **centre** of the player sprite is on tile `(x, y)` (tile coordinates). |
| `IF PLAYER TOUCHES (x, y)` | **Identical** to `PLAYER AT` in MSC4 (see the note below). |
| `IF PLAYER IN_X a, b` | The player's **pixel** X is between `a` and `b` (inclusive). |
| `IF PLAYER IN_Y a, b` | The player's **pixel** Y is between `a` and `b` (inclusive). |
| `IF PLAYER FALLING` | The player is falling: `vy > 0` **and** `player.possee` is 0 (i.e. not standing on anything). |
| `IF PLAYER NOT_FALLING` | The negation of the above: `vy <= 0` **or** `player.possee != 0`. May also be written `PLAYER NOT FALLING`. |
| `IF PLAYER STILL` | `vx == 0 && vy == 0`. |

> **`TOUCHES` is not `TOUCHES`.** In MSC3, `PLAYER TOUCHES` meant "any part of
> the sprite overlaps the tile". In MSC4 both `TOUCHES` and `AT` compile to the
> same opcode, which compares the player's *centre tile* against `(x, y)`. Use
> `TX`/`TY` (or `PX`/`PY`) if you need finer control.
>
> In MK1 v3.2 the player sprite is 16×16 (exactly one tile), so the centre
> tile is also the only tile the player can be on, and the two forms coincide
> in practice. The difference only shows up with bigger player sprites.

> **`IN_X` / `IN_Y` take pixels, not tiles.** The interpreter compares them
> against `gpx`/`gpy`. To test tiles, use `TX`/`TY`:
>
> ```
> IF TX = 5
> IF TY = 6
> ```
>
> **`FALLING` and `NOT FALLING` are exact negations of each other**, and both
> look at the whole 16-bit `player.vy` plus the `player.possee` flag. Before
> the fix described in [§16.2](#162-fixed-in-this-build) they read the wrong
> fields.

### 10.4 `IF NPANT = N`

`IF NPANT = N` is the normal way to ask "am I on screen N?". With the
`FASTNPANT` directive (or the `fastnpant` command-line switch) it is compiled
to a dedicated opcode that compares against the engine's `n_pant` directly
instead of going through the generic `read_vbyte` machinery.

Only the `=` form is optimised; `IF NPANT <> 3` uses the generic path.

---

## 11. Actions

Actions are the statements inside a clause. The left-hand side of an assignment
must be a *flag* (or a writable special variable).

### 11.1 Flags

| Written | Effect |
|---|---|
| `$A = B` | `flags[A] = B` |
| `SET $A = B` | same thing |
| `INC $A, B` | `flags[A] += B` |
| `DEC $A, B` | `flags[A] -= B` |
| `ADD FLAGS x, y` | `flags[x] += flags[y]` (deprecated form, still supported) |
| `SUB FLAGS x, y` | `flags[x] -= flags[y]` (deprecated form, still supported) |

`$A` may be an alias (`%A = 3`) or a writable special variable (`PY = 32`).
`B` may be a literal, a flag, an alias, a special variable or an expression.

Arithmetic is 8-bit and **wraps around**: `DEC $A, 1` on a flag holding 0
gives 255.

### 11.2 The screen

| Written | Effect |
|---|---|
| `SET TILE (x, y) = t` | Puts tile `t` at the given screen coordinates. The behaviour of the tile is taken from the `comportamiento_tiles` table. |
| `SET BEH (x, y) = b` | Sets the *behaviour* of the tile at `(x, y)` without changing the tile itself. |
| `PRINT TILE (x, y) = t` | Draws a coloured tile over the screen at `(x, y)` (overlay, does not touch the map). |
| `WARP TO n, x, y` | Teleports the player to screen `n`, pixel position `(x*16, y*16)`. |

### 11.3 The player

| Written | Effect |
|---|---|
| `RECHARGE` | Sets the player's life to `PLAYER_LIFE` (a constant defined by the generated interpreter, default 99). |
| `SHOW` | Forces a screen refresh (`SPUpdateNow` / `_cpc_UpdateNow`). |
| `PAUSE n` | Busy-waits `n` frames (`halt` loop). |

### 11.4 Sound, text, externals

| Written | Effect |
|---|---|
| `SOUND n` | Plays sound effect `n` (`peta_el_beeper`). |
| `TEXT "…"` | Draws a short line of text in the HUD (at `TEXT_X`/`TEXT_Y`). The string is stored **verbatim** in the bytecode. |
| `TEXT BOX "…"` | Shows a full text box. The string is 5-bit encoded into `text.bin`. |
| `EXTERN n, m` | Calls the engine's `do_extern_action (n, m)`. The second parameter defaults to 0. This is the escape hatch to custom C code. |
| `PAUSE n` | see above. |

### 11.5 Items

| Written | Effect |
|---|---|
| `GET ITEM SET $F` | Picks up the item the player is standing on (see [§14](#14-the-one-slot-inventory-aid)). |

### 11.6 Decorations

```
DECOS
	3, 4, 5
	6, 7, 8
END
```

`DECOS` paints a list of tiles in one go (it is the same as writing several
`SET TILE` commands, but it is a single opcode and therefore cheaper). Each
line is `x, y, tile`.

The `DECOS` block is terminated by an `END`, and the clause that contains it
needs **its own** `END` afterwards:

```
IF TRUE
THEN
	DECOS
		3, 4, 5
	END
	SOUND 1
END
```

### 11.7 Flow control and game state

| Written | Effect |
|---|---|
| `BREAK` | Stops the execution of the whole section. The remaining clauses are not evaluated. |
| `WIN GAME` | Ends the script and sets `script_result = 1` → the player wins. |
| `GAME OVER` | Ends the script and sets `script_result = 2` → the player loses. |
| `RERUN v` | Sets `PARAM` to `v` and restarts the current section from the top. |
| `CALL name` | Runs the subroutine `name` (see [§12](#12-subroutines)). |

`BREAK`, `WIN GAME`, `GAME OVER` and `RERUN` are *terminators*: the compiler
does not emit the clause's closing marker after them, and warns about any code
that follows.

### 11.8 Dialogs

| Written | Effect |
|---|---|
| `$F = DIALOG "a", "b", "c"` | Presents a menu with up to three options and stores the selected option (1, 2 or 3) in `$F`. |

`DIALOG` is an *expression*; it can be used directly in a comparison, but the
usual pattern is to store it and then `RERUN` with the result, which turns the
section into a state machine (see [§17.3](#173-a-dialog-driven-state-machine)).

---

## 12. Subroutines

```
SUB MYPROCEDURE
	IF TRUE
	THEN
		$5 = 1
	END
END
```

and, from anywhere:

```
CALL MYPROCEDURE
```

Rules:

* The subroutine must be **defined before it is called** (the compiler resolves
  the address when it sees the `CALL`).
* A subroutine body is syntactically identical to a section body: clauses with
  conditions and actions.
* `CALL` is implemented by pushing the script pointer, jumping to the
  subroutine, running the interpreter recursively, and restoring the pointer.
  A `BREAK` inside a subroutine returns from the subroutine only.
* Subroutines live in the same binary as the sections; the compiler remembers
  their offsets.
* Names are case insensitive.

---

## 13. Text and dialogs

### 13.1 Two different text systems

| | `TEXT "…"` (legacy) | `TEXT BOX "…"` |
|---|---|---|
| Opcode | `$E3` | `$E6` |
| Storage | Raw ASCII, inline in the bytecode | 5-bit encoded, in `text.bin` |
| Rendered by | `draw_line_of_text` (HUD text) | `run_text_box` → `textbox()` (a framed box) |
| Requires | `TEXT_X` / `TEXT_Y` in `config.h` | `ENABLE_ENCODED_TEXT` in `config.h` |
| Word wrap | no | yes, at `TEXT WIDTH` characters |

`TEXT` is legacy: it draws a short line in the HUD, needs `TEXT_X` / `TEXT_Y`
to be defined in `config.h` (papaya leaves them commented out to save code),
and it is *not* word-wrapped. `TEXT BOX` is the modern way to show text.

### 13.2 The 5-bit encoding

The encoder (`mt5bitEsc.bas`) packs the text into 5-bit values, with an escape
mechanism:

| Value | Meaning |
|---|---|
| `0` | ESC — the next value is an "extended" character |
| `1 … 26` | `A` … `Z` |
| `27` | `,` |
| `28` | `.` |
| `29` | `?` |
| `30` | `!` |
| `31` | space |
| ESC `0` | end of string |
| ESC `1 … 30` | ASCII 33 … 62 (`!` … `>`) |
| ESC `31` | newline (inserted by the word wrapper) |

Consequences:

* **Lower case is folded to upper case.** Write your texts in upper case.
* The accented characters of the Spanish/French alphabets are **not
  supported** and are silently dropped. `VIEJO` is fine, `PERA` is not.
* Characters outside the supported set (for example `@`, `[`, `\`, `]`, `_`,
  `{`, `|`, `}`) are silently dropped.
* The word wrapper breaks the line at `TEXT WIDTH` characters (default 24) and
  inserts a newline. It breaks at spaces, so long words are not split.
* Identical texts are stored only once in `text.bin` and referenced by offset.

### 13.3 Dialogs

```
%RES = DIALOG "MAS PAPAYAS", "OTRO LUGAR"
```

* Up to **three** options. Missing options are encoded as `$FFFF` and are
  skipped by the engine's dialog renderer.
* The option strings go through the same 5-bit encoder, so the same character
  restrictions apply.
* **Commas are option separators**: an option cannot contain a comma. Spaces
  are fine (they are internally encoded as `_` and decoded back).
* The engine must provide a `dialog (unsigned char *ptr)` function that
  receives the three decoded option strings (32 bytes each) and returns the
  selected option in `L` (1, 2 or 3).
* Requires `ENABLE_DIALOG` in `config.h` (which implies `ENABLE_ENCODED_TEXT`).

---

## 14. The one-slot inventory aid

MSC4 provides a small facility for games where the player can carry **one**
item at a time (a key, a bra, a medallion…). It is the combination of the
`ITEM SLOT` / `ITEM EMPTY` directives, the `GET ITEM SET` action, and the
`SPECIAL TILE TOUCHED` section.

The idea:

* A flag (the *item slot*, e.g. `ITEM SLOT = 0`) holds the tile number of the
  item the player is carrying. `ITEM EMPTY` (default 0) means "carrying
  nothing".
* A second flag per item records whether it has been picked up.
* Items are placed on the map as tiles with behaviour `128` (bit 7 of the
  behaviour set).
* When the player walks onto such a tile, the engine fires
  `SPECIAL TILE TOUCHED`, with `TN` = the tile number and `TX`/`TY` = its
  coordinates.

In the script:

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

* does nothing if the item slot is not empty;
* otherwise sets the item slot to `TN`;
* removes the tile from the screen (tile 0 at `TX`, `TY`);
* sets the flag `$F` to 1.

The engine side is a three-liner in `mainloop.h` that redraws the item tile in
the HUD whenever the slot changes:

```c
#if defined ACTIVATE_SCRIPTING && defined ITEM_X
	if (flags [ITEM_FLAG] != flag_old) {
		draw_coloured_tile (ITEM_X, ITEM_Y, flags [ITEM_FLAG]);
		flag_old = flags [ITEM_FLAG];
	}
#endif
```

`ITEM_FLAG` in `config.h` must point at the same flag as `ITEM SLOT` in the
script.

---

## 15. Engine integration

### 15.1 The files

```
game/
  script/
    script.spt        <- the script
    script.spt.bin    <- generated bytecode
    text.bin          <- generated text pool
  dev/
    msc4i.h           <- the MSC4 "interface" header (hand-written, per game)
    msc4i.asm         <- the generated interpreter
    msc.txt           <- the debug log of the compilation (optional)
```

`msc4i.h` is the bridge between the engine and the script. The version used by
papaya is the reference; it provides:

* the section-number constants (`SC_ENTERING_GAME`, …);
* the `script (unsigned char n)` entry point;
* the 5-bit text decoder and the `textbox` / `dialog` glue;
* the `BINARY` directives that embed `script.spt.bin` and `text.bin` into the
  game.

### 15.2 The `script()` entry point

```c
void __FASTCALL__ script (unsigned char a) {
	#asm
			ld  a, l
			ld  (_script_n), a          ; which section to run

			ld  a, (_tpx)              ; refresh TN from the player's
			ld  c, a                   ;   centre tile
			ld  a, (_tpy)
			call qtile_do
			ld  a, l
			ld  (_tqt), a

			ld  a, 0xff                ; PARAM = $FF on entry
			ld  (_script_param), a

			call _script_do

			// gpx, gpy may have changed, so update player.x/player.y
			call _shl_player_coords
	#endasm
}
```

`PARAM` is `$FF` when a section is entered normally, and whatever `RERUN` was
given when the section is re-entered. That is the whole "re-entrancy" feature:
a section can look at `PARAM` to know *why* it is running.

### 15.3 The engine hooks

| Engine event | Where | Call |
|---|---|---|
| New game | `mainloop.h` | `script (SC_ENTERING_GAME)` |
| Entering a screen | `engine.h` (screen init) | `script (SC_ENTERING_ANY)` then `script (SC_ENTERING_SCREEN + (n_pant << 1))` |
| Action key pressed | `mainloop.h` | `script (SC_PRESS_FIRE_AT_ANY)` then `script (SC_PRESS_FIRE_AT_SCREEN + (n_pant << 1))` |
| Hotspot collected | `mainloop.h` | `script (SC_PLAYER_GOT_SOMETHING)` |
| Enemy killed | `engine.h` | `script (SC_PLAYER_KILLS_ENEMY)` |
| Player on a behaviour-128 tile | `engine.h` (update loop) | `script (SC_SPECIAL_TILE_TOUCHED)` |
| Player touches an enemy | `engine.h` | `script (SC_ENEMY_TOUCHED)` |

The per-screen calls are guarded by `#ifndef NO_INDEXED_SCRIPTING`; define that
in `config.h` if your game does not use them.

### 15.4 `config.h` switches

```c
#define ACTIVATE_SCRIPTING     // enable the scripting engine
#define SCRIPTING_DOWN         // (optional) DOWN is the action key
//#define SCRIPTING_KEY_M      // (optional) M is the action key
//#define NO_INDEXED_SCRIPTING // (optional) no per-screen sections
#define ENABLE_ENCODED_TEXT     // (optional) support for TEXT BOX
#define ENABLE_DIALOG           // (optional) support for DIALOG
#define MAX_FLAGS 32            // number of flags available to the script
#define ITEM_FLAG 0             // the flag shown in the HUD as "the item"
#define ITEM_X 28
#define ITEM_Y 1
```

With `ACTIVATE_SCRIPTING` defined, the v3.2 engine ends the game when
`script_result` becomes 1 (win) or 2 (game over). (`WIN_ON_SCRIPTING` is an
MK1 v4 / MSC3 feature and does not exist in the v3.2 engine.)

### 15.5 The engine side of `EXTERN`

`EXTERN n, m` calls `do_extern_action (n, m)`, which the game must implement
in `extern.h`. This is where you hook the script to your own C code:

```c
void do_extern_action (unsigned char n, unsigned char m) {
	switch (n) {
		case 0: /* reset persistence */ break;
		case 1: /* mark the current tile as persistent */ break;
		case 2: /* read persistence, clear the papayas */ break;
		case 3: /* update the HUD */ break;
	}
}
```

### 15.6 `script_result`

| Value | Meaning | Read by the engine as |
|---|---|---|
| 0 | normal | — |
| 1 | `WIN GAME` | the player wins |
| 2 | `GAME OVER` | the player loses |

### 15.7 Two addressing modes

**Indexed mode** (default). The section index at the head of the binary holds
the offset of every section; the interpreter looks the offset up in a table.
The index is `16 + 2 * rooms` entries long.

**Jump-table mode** (`jumptable` option). The index only holds the 16
"general" sections; the per-screen sections are found by a chain of
`cp`/`ret` instructions generated by the compiler. This trades a bit of speed
for a much smaller index in games with many screens and few scripted ones.
Papaya uses it.

```asm
.script_jump_table
	ld  a, (_script_n)
	ld  hl, 0x00C8      ; absolute address of "ENTERING SCREEN 1"
	cp  18
	ret z
	...
	ld  a, 0xff         ; not found
	ret
```

---

## 16. Gotchas, and the bugs that were fixed

### 16.1 Gotchas that are still there

These are *by design* (or at least, they are how the language works). They are
the things that will bite you first.

1. **A letter after `$` is not a name.** `$F` is *not* "flag F": the compiler
   rejects it now, but the historical behaviour was to evaluate `Val("F")`,
   which is `0`. Use digits (`$0`, `$15`) or aliases (`%FLAG_NAME`).

2. **`$%NAME` is an indirection, not a flag.** `ALIAS %G = 3` then `$%G = 1`
   sets *flag 3*. This is the intended "give me a pointer" idiom, but it is
   easy to trip over.

3. **`PLAYER TOUCHES` is the same as `PLAYER AT`.** Neither of them means
   "any pixel of the sprite touches the tile". In MK1 v3.2 the player sprite is
   16×16 (exactly one tile), so the two coincide in practice; the difference
   only shows up with bigger player sprites.

4. **`PLAYER IN_X` / `IN_Y` work in pixels, not tiles.**

5. **Lower case is lost in `TEXT BOX` and `DIALOG`.** Write in upper case.

6. **Dialog options cannot contain commas** (they are the separators). Spaces
   are fine.

7. **`NOINDEXED` in the script is documentary only.** It prints a note; it does
   not change the binary. The index size is governed by `ROOMS` (or by the
   `jumptable` option). The engine side is `#define NO_INDEXED_SCRIPTING` in
   `config.h`.

8. **`TEXT` (the non-box one) is legacy.** It draws a short line in the HUD
   and needs `TEXT_X` / `TEXT_Y` in `config.h`; if you do not define them the
   game will not build. `TEXT BOX` is the modern way.

9. **The whole section is aborted by `BREAK`,** not just the current clause.

10. **Unreachable code after a terminator is still compiled** (with a warning).
    `BREAK`, `WIN GAME`, `GAME OVER` and `RERUN` return from the interpreter,
    so the extra bytes are dead weight.

11. **The `ROOMS` value must be right.** The index is sized from it; a wrong
    value produces a binary whose index does not match the engine's
    expectations. Put the directives at the *top* of the script: the index
    size is computed while the script is being read, so a `ROOMS` directive
    that appears after a `CALL` would produce inconsistent offsets.

12. **`EN_N` reads the engine's `enit` global.** In MK1 v3.2 `enit` is the
    index of the enemy being processed, so `EN_N` is 0, 1, 2… — it is *not*
    the enemy type (that is `EN_T`). If you port the script to an engine that
    does not have `enit`, the generated assembler will not assemble.

### 16.2 Fixed in this build

The compiler in `tests/msc4` has been fixed. The changes are listed here
because some of them change the *meaning* of things that used to be broken
(silently) in your scripts.

| # | Problem | Fix |
|---|---|---|
| 1 | `PLAYER NOT FALLING` (with a space) produced **no code at all**: the condition was silently dropped and the clause ran as if it did not exist. | Both `NOT_FALLING` and `NOT FALLING` are now accepted. |
| 2 | `PLAYER FALLING` tested `player.possee` and was true when it was *non-zero*; `PLAYER NOT_FALLING` read offset 16 (the low byte of the `current_frame` pointer). | `FALLING` is now `vy > 0 && !possee`, `NOT FALLING` is its exact negation, using the full 16-bit `vy`. |
| 3 | `PARAM = 1` wrote `flags[$E9]` instead of `script_param`: the interpreter's lvalue handler was emitted under the wrong guard. | The handler is now emitted whenever `PARAM` is used as an lvalue. |
| 4 | A script using `DIALOG` but not `TEXT BOX` generated an interpreter that did not assemble (the `read_addr` helper was missing). | `read_addr` is emitted whenever it may be needed. |
| 5 | `EN_N` read `_en_it`, a symbol that does not exist in the engine. | It now reads `_enit` (the C global `enit`). |
| 6 | The `IF TILE AT` / `IF BEH AT` condition opcodes (`$30`/`$31`) were dead code: the expression encoder always rewrote them. | The dead opcodes were removed; `TILE AT` / `BEH AT` are expressions, and nothing else. |
| 7 | `IF TRUE` was encoded as opcode `$F0` with **no handler**; it worked only because unknown opcodes fall through. | `$F0` now has a real handler. |
| 8 | An unknown command or condition was **silently ignored**: the compiler emitted nothing and said nothing. | Every unknown command and condition is now reported, with the line number. |
| 9 | `SET TILE (x, y), 7` (comma instead of `=`) silently set tile **0**. | The `=` is now required; a missing one is a syntax error. |
| 10 | A bad flag reference such as `$F` silently became flag 0. | It is now an error. |
| 11 | `END IF` was rejected with "Unexpected command" and aborted the parsing of the section. | `END IF` is accepted as a synonym of `END`. |
| 12 | A script with errors still produced a `.bin` and an interpreter. | The compiler now reports every error, refuses to write the `.bin`, refuses to write the interpreter, and exits with a non-zero status. |

Two more things were changed for the sake of clarity, without altering the
behaviour of correct scripts:

* The `TEXT` (non-box) command is still supported (it is legacy, but it works).
* `PLAYER GOT SOMETHING` is the canonical name of the section (the
  `PLAYER GETS COIN` mentioned in the old `msc4.md` draft does not exist).

### 16.3 Sections that are planned but not implemented

The section index reserves three slots that the compiler does not recognise
yet:

| Slot | Planned section |
|---|---|
| 6 | `TIMER ZERO` — fires when the game timer reaches 0. |
| 7 | `TILE PUSHED` — fires when the player pushes a tile. |
| 9 | `TILE DESTROYED` — fires when a tile is destroyed. |

Writing one of these headers today makes the compiler say "Wrong section at N"
and the section is ignored.

---

## 17. Examples

### 17.1 The papaya box

From `games_old/papaya/script/script.spt`. The game adds a "box" (a crate) that
the player can pick up and put down. The box's position is stored in three
flags, and a subroutine does the actual tile painting, because it has to be
done both when the box is picked up and when it is dropped.

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

Notes:

* `SUB` blocks look exactly like sections. They must appear *before* the
  `CALL`s that use them.
* `%BOXT = TILE AT (%BOXX, %BOXY)` reads the tile that is currently under the
  box, so the box can restore it.
* `IF %BOXT > 0` / `IF %BOXT < 7` implement "0 < x < 7" (there is no `&&`).

The box is picked up and dropped in the `PRESS FIRE AT ANY` section:

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

Notes:

* `BREAK` prevents the "place the box" clause from running in the same frame
  as the "get the box" one.
* `NPANT`, `TX`, `TY`, `PY` are special variables used as ordinary values.
* `CALL PLACEBOX` returns to the middle of the clause.

### 17.2 A complete, self-contained script

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

### 17.3 A dialog-driven state machine

The `DIALOG` expression plus `RERUN` allows a section to behave like a
coroutine. `PARAM` tells the section which "step" it is on.

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

How it works:

* `%STATE` is a normal flag: it survives between runs of the section.
* `DIALOG` returns 1, 2 or 3.
* `RERUN %ANSWER` sets `PARAM` to that value and restarts the section from the
  top; the section then finds the `IF PARAM = …` branch that matches.
* `BREAK` at the end of a branch stops the section so that the other
  `IF PARAM = …` clauses are not evaluated.

> Remember pitfall #8: writing to `PARAM` directly does not work; `RERUN` is
> the supported way.

### 17.4 Using the `&%ALIAS` idiom

From `games_old/ababol`, where the same code has to open five different gates
and pick up four different medallions, but the *flag* to set is only known at
runtime:

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

and later, when the player uses the medallion on a crypt:

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

* `&%MEDA1` evaluates to the *number* of the flag bound to `%MEDA1`.
* `%G` stores that number.
* `$%G = 1` sets the flag whose number is stored in `%G`.

(The original `ababol` source writes `SET TILE (%GX, %GY), 0` with a comma
instead of an `=`. That happens to work: the compiler reads the *fifth* token
as the tile number, finds nothing there, and uses 0. It is not a good idea to
rely on that.)

---

## 18. Appendix — binary format

### 18.1 The section index

The index is a table of 16-bit little-endian offsets, one per section slot:

| Slot | Section |
|---|---|
| 0 | `ENTERING GAME` |
| 1 | `ENTERING ANY` |
| 2 | `PRESS FIRE AT ANY` |
| 3 | `PLAYER GOT SOMETHING` |
| 4 | `PLAYER KILLS ENEMY` |
| 5 | `SPECIAL TILE TOUCHED` |
| 6 | *(reserved: `TIMER ZERO`)* |
| 7 | *(reserved: `TILE PUSHED`)* |
| 8 | `ENEMY TOUCHED` |
| 9 | *(reserved: `TILE DESTROYED`)* |
| 10…15 | reserved |
| 16 + 2n | `ENTERING SCREEN n` |
| 17 + 2n | `PRESS FIRE AT SCREEN n` |

The index has `16 + 2 * rooms` entries (or just 16 in `jumptable` mode). An
offset of `0` means "no such section": the interpreter aborts immediately.

The offsets stored in the index are **absolute** (they include the size of the
index itself), because the interpreter adds `script_bytecode` to them.

### 18.2 The bytecode of a section

A section is a sequence of clauses, terminated by `$FF`:

```
clausule  clausule  ...  clausule  $FF
```

and a clause is:

```
<length> <condition>… $FF <action>… [$FF]
```

* The length byte tells the interpreter where the next clause starts, so a
  clause can be skipped in O(1) when a condition fails.
* `$FF` after the conditions means "THEN": the actions follow.
* `$FF` after the actions means "END of clause". It is omitted if the clause
  ended with a terminator (`BREAK`, `WIN GAME`, `GAME OVER`, `RERUN`).

### 18.3 The opcode map

**Conditions**

| Opcode | Meaning |
|---|---|
| `$01` | `A = B` |
| `$02` | `A < B` |
| `$03` | `A >= B` |
| `$04` | `A <> B` |
| `$05` | `NPANT = N` (only with `FASTNPANT`) |
| `$21` | `PLAYER IN_X (a, b)` (pixels) |
| `$22` | `PLAYER IN_Y (a, b)` (pixels) |
| `$23` | `PLAYER AT (x, y)` (centre tile) |
| `$24` | `PLAYER FALLING` (`vy > 0 && !possee`) |
| `$25` | `PLAYER NOT_FALLING` (`vy <= 0 \|\| possee`) |
| `$26` | `PLAYER STILL` |
| `$F0` | `TRUE` |

**Actions**

| Opcode | Meaning |
|---|---|
| `$00` | `FLAGS[a] = v` |
| `$01` | `FLAGS[a] += v` |
| `$02` | `FLAGS[a] -= v` |
| `$20` | `SET TILE (x, y) = t` |
| `$21` | `SET BEH (x, y) = b` |
| `$22` | `DECOS` list |
| `$30` | `GET ITEM SET $f` |
| `$50` | `PRINT TILE (x, y) = t` |
| `$6D` | `WARP TO n, x, y` |
| `$E0` | `SOUND n` |
| `$E1` | `SHOW` |
| `$E2` | `RECHARGE` |
| `$E3` | `TEXT "…"` (inline) |
| `$E4` | `EXTERN n, m` |
| `$E5` | `PAUSE n` |
| `$E6` | `TEXT BOX "…"` (offset into `text.bin`) |
| `$F0` | `WIN GAME` |
| `$F1` | `GAME OVER` |
| `$F2` | `BREAK` |
| `$F3` | `RERUN v` |
| `$F4` | `CALL address` |

### 18.4 The interpreter

The generated interpreter is a small state machine:

```
_script_do      ; find the section (index or jump table), set the pointer
script_loop     ; read the clause length, compute where the next clause is
script_clausule ; read and evaluate conditions
script_actions  ; read and execute actions
```

Reading a value (`read_vbyte`) understands the `$FF` indirection marker and
resolves the special variables; reading a *destination* (`read_i_v`) resolves
the writable special variables to their engine addresses.

Because the compiler only emits the handlers for the opcodes your script uses,
the interpreter is as small as your script allows. If you use a new opcode,
regenerate the interpreter — the game will not work with a stale one.

### 18.5 Rebuilding the compiler

The compiler is written in FreeBASIC. To rebuild it:

```
fbc msc4.bas mtparser.bas cmdlineparser.bas mt5bitEsc.bas
```

All four files must be compiled together (the `.bi` files are the
declarations). Any recent FreeBASIC release works; the compiler was developed
with 1.08.1.

After rebuilding, remember to copy the new `msc4.exe` to the `utils` folder of
the games that use it, and to regenerate the interpreters (`.asm`) and the
bytecode (`.bin`) of those games.
