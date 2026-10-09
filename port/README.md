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
`find_package(SDL3)`, otherwise fetched and built statically). For the
browser build see "Web build (Emscripten)" below. The game data is
read from `binaries/` (the original game directory); pass `--data DIR` to
use another copy. Options of `port/build/release/opeu`:

    --data DIR      original game directory
    --scale N       initial window scale (default 2)
    --headless      no window, virtual time (1 ms per poll), scripted input
    --ms N          headless: stop after N ms of virtual time
    --input FILE    scripted input log (format below)
    --png FILE      write the final frame as PNG when stopping
    --shots DIR MS  headless: also write DIR/<ms>.png every MS ms of virtual time
    --wav FILE      write the sound output (49716 Hz 16-bit stereo WAV)
    --opl-log FILE  log every OPL2 register write: frame (at 49716 Hz), register, value
    --mute          window mode: do not open an audio device
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

In a window, `--input` plays the same script against host time (ms since
start), e.g. to listen to the music unattended. Headless, every input poll
advances virtual time by 1 ms; at `--ms` the
frame is written to `--png` and the program exits 0. Scripts in
`port/tests/`: `smoke-input.txt` (mouse: title, New Game, scenario list,
France briefing; `make smoke-play`) and `smoke-turn.txt` (Enter every 1.5 s
through set-up and briefing into turn processing; `make smoke-play
SMOKE_INPUT=port/tests/smoke-turn.txt SMOKE_MS=120000`, needs a reference
with both fills recovered).

## Web build (Emscripten)

The same sources build for the browser with Emscripten (tested with
emscripten 6.0.10 from Homebrew; SDL3 is emscripten's own port, pinned by
that release to SDL 3.4.2 and checked by SHA-512 when emcc fetches it):

    . port/tools/web_env.sh          # only if emcc is not configured (see below)
    emcmake cmake -S port -B port/build/web -DCMAKE_BUILD_TYPE=Release
    cmake --build port/build/web -j8
    python3 port/tools/serve_web.py  # or: python3 -m http.server 8765 --directory port/build/web
    open http://localhost:8765/opeu.html

Output in `port/build/web`: `opeu.html` (from `platform/web/shell.html`),
`opeu.js`, `opeu.wasm` (about 2.7 MB) and `opeu.data` (about 2.3 MB: the
original data files of `binaries/`, or of `-DOPEU_WEB_DATA=DIR`; the DOS
programs `*.EXE`/`*.COM`, the batch files and the eXoDOS marker are left
out). The native build is unchanged by any of this (everything web-only is
under `if(EMSCRIPTEN)` / `#ifdef __EMSCRIPTEN__`).

`port/tools/web_env.sh` (source it) is needed with Homebrew's emscripten:
that package has no `.emscripten` config and its launcher picks the first
`python3` in `PATH`, which on macOS can be the system 3.9 (emscripten 6
needs >= 3.10). The script sets `EMSDK_PYTHON`, writes
`port/build/emscripten.config` (LLVM and Binaryen of the install) as
`EM_CONFIG`, and puts the emscripten cache in `port/build/emcache`, so the
first build compiles libc and SDL3 into the checkout (about a minute)
instead of into the Homebrew prefix. Variables already set are kept.

**Page.** The canvas is 640x480 scaled by CSS to the largest 4:3 box of the
browser window (nearest-neighbour, letterboxed by SDL); the game draws its
own pointer and the host cursor is hidden over it; right clicks go to the
game. A "Click to start" overlay holds `main()` until the first click
(gives the canvas keyboard focus and is the user gesture browsers want
before audio). `opeu.html?autostart` skips it (automated checks). When the
program ends (scenario finish) the overlay says so; reload to play again.
No external resources are loaded.

**Waits.** The game never returns to a main loop: it polls the keyboard,
mouse and FM driver in loops (all through `port_pump`). The build links
with `-sASYNCIFY` and the SDL3 backend gives control back to the browser
with `emscripten_sleep()` at two places only: after each presented frame
(at most about 60 per second) and in `plat_sleep_ms()` (the pump's idle
sleep, every 64 idle polls). SDL's own automatic `emscripten_sleep()` calls
are disabled (`SDL_HINT_EMSCRIPTEN_ASYNCIFY=0`) so these are the only
yields. `ASYNCIFY_STACK_SIZE` is 1 MiB (the saved native stack of the
deepest wait); `ASYNCIFY_IGNORE_INDIRECT` is not used because every path
from game code to the pump goes through function-pointer service callbacks,
so the whole module is instrumented. `-DOPEU_WEB_JSPI=ON` uses JSPI instead
(smaller and faster, `opeu.wasm` about 1.8 MB; Chrome/Edge and Node only,
not Safari). No threads and no SharedArrayBuffer, so no COOP/COEP headers.

**Files and saves.** `opeu.data` mounts the originals at `/gamedata`; the
program uses `/data` (the `--data` default of the web build). In the
browser `/data` is an IDBFS mount with `autoPersist`: before `main()`
(`platform/web/pre.js`) it is loaded from IndexedDB (database `/data`),
originals it does not have yet are copied in from `/gamedata`, and every
file the game writes is stored back to IndexedDB when it is closed. Saved
games therefore survive reloads; clearing the site data resets the game
directory to the originals.

**Headless under Node.** The same `opeu.js` runs in Node (`-sENVIRONMENT=web,node`),
with `/data` in memory and the host file system mounted at `/host`; run it
from the build directory (the packaged data is found relative to the
current directory):

    cd port/build/web
    node opeu.js --headless --ms 120000 --input /host$PWD/../../tests/smoke-turn.txt --png /host/tmp/web.png

Verified 2026-10-09: the Node runs of `smoke-input.txt` (15 s) and
`smoke-turn.txt` (120 s and 600 s of virtual time) write PNGs byte-identical
to the native build's; in headless Chromium (CDP, swiftshader) the page
loads, the click starts the game, keyboard (Enter through set-up into turn
processing) and mouse (mouse selection, New Game, scenario, briefing) work
with the CSS-scaled canvas, and a file written to `/data` survives a page
reload.

Sound: SDL3's Emscripten audio backend (a ScriptProcessorNode on the
page's AudioContext, resampled by SDL from 49716 Hz to the context rate)
calls the same audio callback as the native build, on the main thread,
whenever the browser runs its event loop (the game yields at every frame
and idle sleep; calling into the module while it is suspended by ASYNCIFY
is allowed because the callback itself never sleeps). The click on the
start overlay is the user gesture browsers want before audio starts.
Checked 2026-10-09 in headless Chromium with `port/tools/web_audio_check.mjs`
(it measures the page's audio buffers over CDP): the AudioContext runs at
48 kHz and, after the input prompt, the title song plays (RMS about 0.17
of full scale, no clipping). Under Node (no Web Audio) the build clocks the
driver from the event pump as headless; `--wav` there writes a WAV
byte-identical to the native build's.

Known limitations: a long computation that does not
poll input keeps the tab busy until it polls again (as the original kept
the PC busy); the first visit stores the original files (about 2.3 MB) in
IndexedDB; Safari needs the ASYNCIFY build (the default).

### Web platform edits (and why)

- `platform/sdl3/platform_sdl3.c` (`#ifdef __EMSCRIPTEN__` only):
  `emscripten_sleep(0)` after `SDL_RenderPresent` (the browser shows a
  frame and delivers input only when the program yields);
  `plat_sleep_ms()` calls `emscripten_sleep(ms)` (idle yield);
  `SDL_HINT_EMSCRIPTEN_ASYNCIFY=0` (no other, unpaced yields);
  `SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT=#canvas` (keys go to the focused
  canvas, not the whole page); no `SDL_SetRenderVSync` (the browser paces
  frames; with no emscripten main loop the call only warns); letterbox
  instead of integer-scale presentation (the CSS box is rarely an integer
  multiple of 640x480).
- `core/wasm_abi.c` + `CMakeLists.txt` (web only): aiops5.c declares six AI
  routines of aiops3/aiops4 (`opeu_ai_08f8 093c 0a5a 0c3c 0d82 1064`) as
  `void` while they return `uint16_t`. Native ABIs ignore the unread
  return value; WebAssembly checks call types, wasm-ld replaces such calls
  with a trap stub and Binaryen then rejects the module. aiops5.c is
  compiled with each name mapped to a `void` forwarder that calls the
  recovered routine and drops the value. `src/` is unchanged.
- `CMakeLists.txt`: the `if(EMSCRIPTEN)` block (SDL3 port, link flags,
  data packaging, `/data` as the default data directory; the host test
  programs are not built for the web).
- `platform/web/shell.html`, `platform/web/pre.js`: the page and the file
  system set-up described above. `tools/web_env.sh`, `tools/serve_web.py`:
  toolchain environment and a local server (`.wasm` as `application/wasm`,
  `Cache-Control: no-store`).

## Sound

The music and sound effects are those of the original: MAIN's song bank
(19 songs) and effect bank (19 effects) at 3D91:0000/7BBA in the data
arena, played by the KOEI FM driver on an emulated YM3812 (OPL2).

- **Driver** (`services/fmdriver.c`, `fmdriver.h`): FMDRV.COM 3.30
  rewritten by hand as readable C: named voice records (six melodic voices
  on OPL channels 0-5, the rhythm voice on 6-8), the song/effect sequencer
  with one function per track command, repeat/loop, tempo, fade-out,
  queued songs, the end-of-song callback, software vibrato and the five
  instrument LFO shapes, rhythm mode, the countdowns and the INT 66h /
  far-call API. It writes registers through `write_register(reg, value)`
  and reads the caller's banks through `peek(seg, off)`. The original
  binary is never loaded or executed. Two cases where the original would
  fault (a divide overflow at extreme LFO settings, a note table index
  past its end for transposes over 160 semitones) are defined in C and
  documented at the spot.
- **Clock** (`services/fm.c`): the original programmed the PIT to 1000h
  counts (1193182/4096 = 291.304 Hz, every 16th tick chained to the BIOS
  clock). The tick is derived sample-accurately from the audio clock: each
  output frame (49716 Hz, the OPL2's own rate) adds 1193182 to a phase,
  each 4096 * 49716 is one tick, run before the frame where it falls due.
  In a window the SDL audio callback renders frames and so clocks the
  driver (music and the game's countdown waits keep running while the
  game computes, as with the PIT interrupt); game-side driver calls take
  the audio stream lock. If the device stops pulling samples for 200 ms the
  event pump clocks the driver from host time instead. Headless (and with
  `--mute` or no audio device) the pump clocks it from virtual/host time,
  so headless runs are reproducible: the same script gives byte-identical
  WAV, OPL log and PNG (also from the web build under Node).
- **Chip**: [ymfm](https://github.com/aaronsgiles/ymfm) by Aaron Giles,
  BSD-3-Clause (`third_party/ymfm/LICENSE`), pinned to commit
  `81aec25ccbb98f4873a255f7551ac4dadac59b4a` (2026-07-27; ymfm has no
  release tags). Vendored files: `ymfm.h`, `ymfm_opl.{h,cpp}`,
  `ymfm_fm.{h,ipp}`, `ymfm_adpcm.{h,cpp}`, `ymfm_pcm.{h,cpp}` (the OPL
  family source needs the ADPCM/PCM units for its Y8950/OPL4 classes).
  One local change: in `ymfm_opl.cpp` (Y8950 register switch) `case 0x0f:`
  is spelled `case 15:`, because `tools/port_lint.py` reads `case 0x0f:`
  as an opcode switch; same value, comment at the line. Chosen over
  Nuked-OPL3 (LGPL-2.1) and DOSBox's DBOPL (GPL) for the licence. This is
  emulation of the sound chip (a hardware service behind the platform
  layer), not of the CPU. Output gain x3 (`opl_ymfm.cpp`): the game's songs
  peak around 23000 of 32767.
- **Output**: SDL3 audio stream, 16-bit stereo at 49716 Hz (SDL resamples
  to the device rate).

How to hear it: `make play` (window with sound), or write it to a file:

    port/build/release/opeu --headless --ms 60000 --input port/tests/smoke-turn.txt --wav music.wav

Verification without ears:

- `ctest -R fmdriver_registers` (`port/tests/fmdriver_diff.py`): the register
  stream of the native driver against the original FMDRV.COM, which runs
  under `tests/validate_fmdrv.py`'s Unicorn harness (its machine, session
  and synthetic banks reused; the original is the fixture, the port never
  runs it). Every step (install, each INT 66h / far call, each PIT tick,
  each installer run) must produce the same OPL writes in the same order
  and the same AX/BX/DX/DS. Sessions: the validator's (songs, effects,
  fade, queue, callback, effect-voice and rhythm commands, uninstall, no
  chip), all 19 MAIN songs (8000 ticks each with repeat, then MAIN's fade
  20), all 19 MAIN effects, OPEN's three songs (`port/tests/dump_open_bank.c`
  compiles `src/open/data/ofmusic.c`). Needs a Python with `unicorn`
  (the repository's `.venv`; `-DOPEU_TEST_PYTHON=...`); skipped otherwise.
  Result 2026-10-09: 205,166 of 205,166 steps identical (79,279 OPL
  writes, 204,960 PIT ticks, 194 calls); per-session numbers in
  `<build>/fmdriver-diff.json`.
- `ctest -R fm_headless_audio` (`port/tests/check_audio.py`): a headless run of
  `smoke-input.txt` with `--wav`/`--opl-log`/`OPEU_TRACE=fm`: the game
  starts a song, key-ons follow each song start within one second, the WAV
  is silent before and audible after (>= 60% of 100 ms windows above
  -50 dBFS) and does not clip.

## How it is put together

    src/main/**/*.c          recovered game (host branches), compiled unchanged
    port/main.c              crt0's role: data image, devices, then the recovered main()
    port/core/               data arena, DGROUP stack model, routine link table, diagnostics, event pump
    port/services/           native assembly-library and DOS/BIOS/driver services
    port/link/<area>/M.c     the service callback of recovered module M
    port/link/routines/      handlers for routines reached by identifier
    port/platform/           platform contract (platform.h) and SDL3 backend
    port/third_party/ymfm/   the OPL2 sound-chip emulator (BSD-3-Clause, vendored)
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
- `services/fmdriver.c`: the KOEI FM driver FMDRV.COM ("OPL-II Sound
  Driver 3.30", `src/fmdrv/driver.asm`) rewritten by hand as native C (see
  "Sound" below). Hardware service: the original is a DOS TSR, not part of
  the game program.
- `services/fm.c`: the PC side of that driver: INT 66h entry for the
  recovered SOUND module and the SYSLIB helpers, the PIT clock (derived
  from the audio sample clock), bank memory reads from the arena, the
  audio callback, `--wav`/`--opl-log`.
- `services/opl_ymfm.cpp`, `services/opl.h`: the YM3812 chip (ymfm) behind
  `opl_write(reg, value)` / `opl_render()`.
- `services/input.c`, `core/pump.c`: BIOS keyboard (INT 16h) and the
  mouse driver (INT 33h) from SDL events, keeping the polling semantics;
  pointer drawing; the headless script player.
- `services/clock.c`: the BIOS tick count and `getch` from SDL time.
- `core/memory.c`, `core/link.c`: the data arena, the DGROUP stack model,
  the routine link table lookup, diagnostics.
- `main.c`: crt0's role (data image, devices, then `opeu_application_entry`).
- `platform/`: SDL3 window/present, events, time, PNG writer; for the
  web build also the page shell and file-system set-up (`platform/web/`)
  and `core/wasm_abi.c` (see "Web platform edits").

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
- Sound: OPEN.EXE (the opening, with its own song bank) and END.EXE are not
  part of the port, so their music is only exercised by the driver
  comparison (OPEN's three songs), not heard in the game.
