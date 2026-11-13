# Ramiro el Vampiro 1 & 2 for MTE MK1 v3.2

Ports of *No tiene un respiro* (Ramiro 1) and *Devuelve el zafiro*
(Ramiro 2), The Mojon Twins 2016 (MK2 v0.90b, msc3), to MK1 v3.2 with msc4.
Ramiro moves exactly like in Ramiro 4 (MK1v4 `games_zx/ramiro4`).

```
addons/ramiro.h          the add-ons, reusable by any game (inline asm)
games/ramiro1/           No tiene un respiro
games/ramiro2/           Devuelve el zafiro
tools/                   one-off importers and the movement test
```

Each game folder is a normal MK1 v3.2 project. ZX 48K is the main target.
CPC is mode 1 with placeholder graphics and WYZ music.

## Building

| | Windows | Linux |
|---|---|---|
| ZX (.tap) | `dev\comp.bat zx` | `dev/comp.sh` |
| CPC (.cdt + .sna) | `dev\comp.bat cpc` | `dev/comp_cpc.sh` (.sna only) |

The Linux scripts use the Python tools in `utils/py`. Set `Z88DK` to the
z88dk 1.10 directory. Besides the ports used for Camp Escape, there are
two new tools:

* `cpcgfx.py`: the mkts_om CPC modes MK1 uses.
* `cpctools.py`: `defb2bin` replaces pasmo for the sprite LUT, and `bin2sna`
  writes the snapshot.

Making a .cdt needs the Windows loader tools.

Map, enemies and tile behaviours live in `apor/ramiroN.apor`. Edit that
file with Aportador; `comp.sh` exports it on every build.

## addons/ramiro.h

To use it, include it from `custom.h`. Then call `ramiro_before_vsync ()`
from `before_vsync ()`, and make `do_extern_action` and `textbox` jump to
`ramiro_extern` and `ramiro_textbox` in `extern.h`. See ramiro1's files.
The settings (`RAMIRO_*`, physics, flags) go in `config.h`.

What it provides:

* **Movement.** `player_custom_move ()` is Ramiro 4's `move ()` (its
  `config.h` resolved): `BOTH_KEYS_JUMP`, `RAMIRO_HOP`, `RAMIRO_HOVER`,
  variable jump, `SHORT_PLAYER` collision, `BETTER_VERTICAL_CONNECTIONS`
  `attr`, `EVIL_TILE_SIMPLE`, a conditional evil zone with slow kill, and
  coins.
  * `tools/movetest.py` checks it frame by frame against a Python model of
    Ramiro 4's code. It ran thousands of random-input frames with no
    mismatch.
* **Animation.** Ramiro 4's custom animation (`PLAYER_CUSTOM_FRAME`).
* **Screen connections.** The upward connection at y = -8 runs in
  `ramiro_before_vsync`, after the enemies, as in Ramiro 4. The other three
  are the engine's (`ENABLE_CUSTOM_FLICK`).
* **Type 6 enemies.** Ramiro 4 bats. They move only while
  `RAMIRO_T6_FLAG` is set. Optionally, `RAMIRO_MAKE_T6_FLAG` turns empty
  slots into bats (Ramiro 2).
* **Damage.** Per-type damage (`RAMIRO_LINEAR_HIT`, `RAMIRO_FLYING_HIT`) with
  Ramiro 4's 8-pixel-wide enemy collision.
* **Fire zone.** msc3's `SET_FIRE_ZONE`, via EXTERN 10-14.
* **Other EXTERNs.** REDRAW (16). REENTER needs no EXTERN: the scripts do
  `OPANT = 99`.
* **Sound.** BeepFX effects (Ramiro 4's set) on ZX, through EXTERN 9, n.
  On CPC they map to WYZ effects.
* **HUD.** The sunlight countdown.
* **Text box.** Sir Ababol / Papaya's text box, ported as is: it grows
  line by line, types the text character by character, beeps, waits for a
  key and repaints. Additions for Ramiro:
  * The frame uses the Ramiro font's frame glyphs.
  * A leading `"1".."9"` names the speaker, shown in a tab on top of the box
    (2016 colours, set in `config.h`). The names are in `extern.h`.
  * `"0..."` is a room name, centered in the HUD.
  * The screen is updated before the box is drawn, so boxes shown on
    entering a screen have the new screen behind them.
  * No runtime character translation: the font has `, . ? !` and space in
    slots 91–95, where msc4's decoder puts them, and `¡ ¿ Ñ` in the `< > =`
    slots, so the script writes them as `< > =`.

## Changes to the engine

There are two changes, both in `dev/engine.h` and both only active with
`PLAYER_CUSTOM_MOVE`:

1. **The move hook.** `move ()` calls `player_custom_move ()` instead of the
   built-in physics and collision.
2. **`shl_player_coords ()`.** It now rewrites `player.x` / `player.y` only if
   `gpx` / `gpy` changed. msc4 calls it after every script run. Without
   this, every press of DOWN (the action key, which is also hover) dropped
   the sub-pixel position, and Ramiro moved differently from Ramiro 4.

## Data conversion (tools/)

* **`ramiro_import.py`** imports the 2016 data:
  * Rows 2 and 3 of `work.png` are swapped, so maps fit RLE53.
  * Maps are remapped. The 9 (R1) and 7 (R2) one-off tiles that don't fit
    are drawn with `DECOS` in the script (`map/decos.spt` lists them).
  * Enemy types are converted. For bats, both ends of the path are set to
    the start position.
  * Behaviours are converted, and the `.apor` is written.
* **`ramiro_texts.py`** converts `texts.txt` to `TEXT BOX` strings.
* **`cpc_placeholders.py`** makes the CPC mode 1 placeholder graphics from
  the ZX ones.

## Things that differ from 2016 (on purpose)

* **Physics.** Ramiro 4's, as requested.
* **Text box.** Sir Ababol / Papaya's, with the 2016 speaker tab.
* **Trap flag (R1).** Screen 29's trap used an undefined alias, which msc3
  read as flag 0. Visiting screens 6 to 8 reset flag 0, so the trap re-armed.
  It has its own flag now (18).
* **Intro flag (R2).** The intro used an undefined alias as well; it now has
  flag 4.
* **Key 1 controls.** In 2016, key 1 (W A S D) jumped with N/M and W did
  nothing. Now W is up/jump and M is fire.

## Not done / to check

* **CPC is not tested.** It compiles, and the binary fits below
  `WYZ_SONG_BUFFER`, but no CPC emulator was available. The graphics are
  placeholders. The music is Ramiro 4's CPC WYZ songs, with its instruments
  and effects.
* **Spanish only.**
