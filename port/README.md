# Operation Europe — native port (SDL3)

The game program MAIN of Operation Europe, built natively for macOS (and
portable to Linux, Windows and Emscripten) from the **recovered C** of
`src/main`, with hand-written native C for everything the original did in
assembly or through DOS/BIOS/driver interrupts. Nothing is emulated: there is
no CPU interpreter, no 8086 emulator, no interrupt/trap layer, no DOS
runtime and no translated machine code. `python3 tools/port_lint.py
--compile-commands=<build>/compile_commands.json` checks this.

## Build and run

    make play                 # configure + build port/build/release, open a window
    make smoke-play           # headless run with a scripted input log, writes a PNG
    make port-lint            # emulation lint + provenance check of the build

Requirements: CMake >= 3.16, a C99 compiler, Python 3, SDL3 (found with
`find_package(SDL3)`, otherwise fetched and built statically). The game data is
read from `binaries/` (the original game directory); pass `--data DIR` to
use another copy. Options of `port/build/release/opeu`:

    --data DIR      original game directory
    --scale N       initial window scale (default 2)
    --headless      no window, virtual time (1 ms per poll), scripted input
    --ms N          headless: stop after N ms of virtual time
    --input FILE    scripted input log (format below)
    --png FILE      write the final frame as PNG when stopping
    --shots DIR MS  headless: also write DIR/<ms>.png every MS ms of virtual time
    --dump-arena F  write the initial data arena (diagnostic) and exit

Environment: `OPEU_TRACE=link,svc,mem` (or `all`) logs calls to `port.log`
(`OPEU_LOG` overrides the path); `OPEU_BACKTRACE=1` prints the native call
stack on a crash and when a headless run stops (macOS/Linux).

Reference tree: the build compiles `src/` of this checkout. To compile
another recovered tree (for example a checkout with newer recoveries), pass
`-DOPEU_SRC=/path/to/src` to CMake or `PORT_SRC=/path/to/src` to make.
CMake reports which optional host branches the tree has
(`OPEU_HAVE_PATH_FILL`, `OPEU_HAVE_MOVE_REACHABLE`, see "Missing" below).

### Scripted input (`--input`)

One event per line, `#` starts a comment; `<ms>` is virtual time in ms.

    <ms> key <name>          press a key (enter esc space tab bs up down left right
                             home end pgup pgdn ins del f1..f10 a..z 0..9 kp0..kp9)
    <ms> key <scan>:<ascii>  BIOS codes in hex, e.g. 1c:0d
    <ms> text <characters>   one key per character (US layout)
    <ms> move <x> <y>        pointer to screen pixel (0..639, 0..479; game band from y=40)
    <ms> click <x> <y> [left|right]   press at <ms>, release 100 ms later
    <ms> press|release <x> <y> [left|right]

Headless, every input poll advances virtual time by 1 ms; at `--ms` the
frame is written to `--png` and the program exits 0. Scripts in
`port/tests/`: `smoke-input.txt` (mouse: title, New Game, scenario list,
France briefing; `make smoke-play`) and `smoke-turn.txt` (Enter every 1.5 s
through set-up and briefing into turn processing; `make smoke-play
SMOKE_INPUT=port/tests/smoke-turn.txt SMOKE_MS=120000`, needs a reference
with both fills recovered).

## How it is put together

    src/main/**/*.c          recovered game (host branches), compiled unchanged
    port/main.c              crt0's role: data image, devices, then the recovered main()
    port/core/               data arena, DGROUP stack model, routine link table, diagnostics, event pump
    port/services/           native assembly-library and DOS/BIOS/driver services
    port/link/<area>/M.c     the service callback of recovered module M
    port/link/routines/      handlers for routines reached by identifier
    port/platform/           platform contract (platform.h) and SDL3 backend
    port/gen/                generated: prototypes, data placement, index
    port/tools/              the generators

The host branch of every recovered module keeps the original 16-bit data
layout and reports each call that leaves the module to a service callback
(a numbered service, or in the battle modules the callee's original
address). `port/link` implements those callbacks: each service calls the
recovered routine the original called (its host entry, with that module's
own callback) or the native replacement of an assembly/DOS/BIOS service.
Link files contain no game logic, only argument conversion.

## Provenance

- **Code.** All 332 `src/main` modules are compiled from source into
  `libopeu_recovered`; the port adds the link callbacks, the native services
  and the start-up. No port file defines a function named like a recovered
  routine (lint-checked).
- **Initial data image.** `port/core/memory.c` builds the 640 KiB data arena
  (FAR_DATA segments 349B..4582, DGROUP at 4712, DOS blocks from 5712; plain
  data, nothing executes from it) from:
  1. the objects of the 36 recovered data modules (`src/main/data/*.c`,
     compiled natively as part of the build); `port/gen/data_image.c` only
     lists where each object lives (segment:offset) and its size —
     `port/tools/gen_ds_image.py` takes the addresses from
     `port/gen/symbols.json` and the sizes by compiling a probe that includes
     each data module and prints `sizeof` of its objects;
  2. the initialized `_DATA` of a few code modules and of the FILEIO/VGALIB
     assembly modules (string literals and small tables the host branches
     read through `ds[]`), generated from their source text by
     `port/tools/gen_module_data.py` into `port/gen/module_data.c`.
  The only DGROUP bytes not provided are the Microsoft C runtime's own data
  (copyright string, runtime error messages, `_C_FILE_INFO`), which the game
  code never reads. No byte comes from the original executable or from
  DOS object files.
- **Addresses.** `port/gen/symbols.json` maps every routine/data name of
  `src/main/dos/layout.json` to its original segment:offset. It was produced
  by resolving the placement map with the public offsets of the DOS objects
  compiled from `src/` (the gate's fragment linker); it contains names and
  addresses only.
- **Why there is a routine link table.** The recovered host branches
  themselves identify some callees by their original address: the 53 battle
  and display modules (and stockord) pass the callee's linear address as the
  service id, and about 30 modules read far code pointers from data tables
  (segment:offset words in the recovered data) and hand the unknown ones to
  their service callback. `port/gen/link_table.c` (generated by
  `port/tools/gen_link_table.py` from the symbol names and the
  `PORT_ROUTINE(name)` handlers in `port/link`) maps such an identifier to
  the native C handler that calls the recovered routine. It is a sorted
  table lookup of names; it executes nothing but C functions.

## Status

### Screens reached (headless runs, PNG captures)

With this checkout's `src/` (which lacks the two fills listed below):
input prompt, New Game / Load Game, the scenario list, the France briefing
(picture and text), the players menu, side, map type, battle view and
difficulty choices, the staff screens (Army Commander and Chief of
Operations assignment). With Enter accepted throughout, the briefing
closes and the first computer unit order stops at the AI path fill with
the "not linkable" message.

With a reference that has both fills recovered (`-DOPEU_SRC` pointing at the
main checkout's staged `src/`, 2026-10-09): everything above, then the
operational map with the army panel, hour-by-hour turn processing (units
moving, "executing orders" reports, weather changes, the date advancing
past the first day), supply transfers with the Supplies panel and unit
reports, and the isometric tactical battle screen (both commanders,
"firing" messages, battle turns 1..10). A 40-minute (virtual) Enter-spam run
ended without a fault.

Not yet exercised: giving orders by hand on the map (only accepted
defaults), saving and loading, the scenario end and the exit to END.EXE,
and an interactive windowed session (the SDL3 window path builds and is the
same code minus the scripted pump).

### Hand-written native routines (and why)

All of these replace assembly or DOS/BIOS/driver services; none is a game
routine.

- `services/vgalib.c`: the VGALIB assembly library (palette, planar blits,
  glyphs, boxes, lines, fills, bitmap save/restore, picture decoders, text
  mode switch), over four 64 KiB VGA planes kept as data;
  `vgalib_render` converts planes + DAC to the indexed frame SDL presents.
- `services/syslib_gfx.c`: the drawing half of SYSLIB (VRAM save/restore,
  draw, glyph colour, sprite compose, `koei_3b77/3bc5/3f26`).
- `services/syslib.c`: the rest of SYSLIB: `opeu_isqrt`, the break and
  critical-error handler install (no-op), the FM driver wait/busy helpers,
  the turn-input poll, the sound-driver open.
- `services/fileio.c`: FILEIO (DOS open/close/read/write/seek/create/exists,
  memory blocks, disk reset) through stdio on the data directory and the
  arena's DOS block allocator.
- `services/ems.c`: the EMS driver (INT 67h) as plain memory.
- `services/fm.c`: the FM driver (INT 66h): countdown and request words
  answered as the driver does (AH=3 and AH=13h return AL=0); silent, with
  `port_fm_requests` as the hook for an OPL backend.
- `services/input.c`, `core/pump.c`: BIOS keyboard (INT 16h) and the
  mouse driver (INT 33h) from SDL events, keeping the polling semantics;
  pointer drawing; the headless script player.
- `services/clock.c`: the BIOS tick count and `getch` from SDL time.
- `core/memory.c`, `core/link.c`: the data arena, the DGROUP stack model,
  the routine link table lookup, diagnostics.
- `main.c`: crt0's role (data image, devices, then `opeu_application_entry`).
- `platform/`: SDL3 window/present, events, time, PNG writer.

### Edited recovered routines

None. No file under `src/` is changed or copied; every recovered module is
compiled as is. Platform differences live in `port/link` (argument
conversion) and `port/services`.

### Boundary pointers installed by the port

- `opeu_ai_unrecovered` (aiops1.c:143): the AI modules' boundary for six
  callees, all now recovered. `link/core/aiops.c` maps the ids to them
  (0 `opeu_release_side_units`, 1 `opeu_release_side_elements`,
  2 `opeu_list_due_records`, 3 `opeu_objective_level`,
  4 `opeu_objective_eligible`, 5 `opeu_ai_05a8`), following
  `tests/validate_ai_ops.py`'s oracle bindings. Argument conversion only.

### Recovered routines that are missing (blockers)

- `opeu_path_fill` (19DF:006E) and `opeu_move_reachable` (32E6:00D8) have no
  host branch in this checkout's `src/`. With such a reference the computer
  player's route search (`opeu_path_fill_hook`) and the movement
  reachability check (moveunit/movetgt service 5) stop the game with a
  "not linkable" message; the port does not substitute its own fill. Both
  are recovered in the main checkout (staged); a build against that tree
  links them automatically (`OPEU_HAVE_PATH_FILL`,
  `OPEU_HAVE_MOVE_REACHABLE`).

### Known issues (reported by the link work, not yet fixed)

- RNG: btladv, btlretr, btlrcalc, btlturn and btlrfire call
  `opeu_rand_bounded` without syncing the generator word at DS:c6f4, so
  battle randomness can diverge from the original sequence.
- Fixed scratch buffers of some host branches overlap other data: the dpl*
  modules use DS:eb00..eb87 (battle path costs), xferunit/xferstep use
  DS:ee00..ee84 (inside the modeled stack window), the dialogs scratch sits
  at DS:ebe0 (`PORT_DS_END`).
- Modeled stack reserves are estimates: `PORT_FRAME` (0x100) is less than
  some map chains need (~0x150); new-game set-up reserves 0x900 (needs at
  least 0x64c); the scenario finish needs at least 0x210.
- evangel service 12 stops (its table address is not passed by the host
  branch); vwregt drops a mode word; chief's choose_value arguments disagree
  with the callee; requests.c:110 reads an uninitialised threshold; the
  load-scenario short-file case is only traced; settings.c:837 may recurse
  into new-game set-up.
- control1: one link report suspected an action-table entry 0x5a off
  (1389:0186 vs 1389:012c). Both are real routine entries
  (`opeu_turn_control_one`, `opeu_choose_turn_delay`) and the turn runs
  without a fault; not confirmed as a bug.
- Sound is silent (no OPL emulation yet).
