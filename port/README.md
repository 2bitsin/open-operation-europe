# Operation Europe — native port (SDL3)

The game program MAIN of Operation Europe, built natively for macOS (and
portable to Linux, Windows and Emscripten) from the **recovered C** of
`src/main` as the generated `game/` tree presents it (`make game`), with
hand-written native C for everything the original did in
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
    --intro         run the opening (OPEN.EXE) before the game; default in a window
    --no-intro      skip it; default with --headless (see "OPEN.EXE" below)

Environment: `OPEU_TRACE=link,svc,mem` (or `all`) logs calls to `port.log`
(`OPEU_LOG` overrides the path); `OPEU_BACKTRACE=1` prints the native call
stack on a crash and when a headless run stops (macOS/Linux).

Game tree: the build compiles `game/` of this checkout, generated from
`src/main` by `make game` (run it first; CMake stops if `game/` is
missing). To compile another generated tree pass `-DOPEU_GAME=/path/to/game`.
`-DOPEU_SRC` (`make port PORT_SRC=...`) is no longer used and only warns.

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

    # web_env.py is only needed when emcc is not configured (see below)
    python3 port/tools/web_env.py emcmake cmake -S port -B port/build/web -DCMAKE_BUILD_TYPE=Release
    python3 port/tools/web_env.py cmake --build port/build/web -j8
    python3 port/tools/serve_web.py  # or: python3 -m http.server 8765 --directory port/build/web
    open http://localhost:8765/opeu.html

Output in `port/build/web`: `opeu.html` (from `platform/web/shell.html`),
`opeu.js`, `opeu.wasm` (about 2.7 MB) and `opeu.data` (about 2.3 MB: the
original data files of `binaries/`, or of `-DOPEU_WEB_DATA=DIR`; the DOS
programs `*.EXE`/`*.COM`, the batch files and the eXoDOS marker are left
out). The native build is unchanged by any of this (everything web-only is
under `if(EMSCRIPTEN)` / `#ifdef __EMSCRIPTEN__`).

`port/tools/web_env.py` (it runs the command given to it with the variables
below set) is needed with Homebrew's emscripten:
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
- No call-signature glue any more. src/main/core/aiops5.c declares six AI
  routines of aiops2/3/4 as `void` while they return `uint16_t`; native
  ABIs ignore the unread value, WebAssembly does not (wasm-ld makes such a
  call a trap stub). `game/core/aiops5.c` calls them through their owners'
  prototypes instead (the generator's "declarations" in
  `tools/game-renames.json`), which replaced `core/wasm_abi.c`.
- `CMakeLists.txt`: the `if(EMSCRIPTEN)` block (SDL3 port, link flags,
  data packaging, `/data` as the default data directory; the host test
  programs are not built for the web).
- `platform/web/shell.html`, `platform/web/pre.js`: the page and the file
  system set-up described above. `tools/web_env.py`, `tools/serve_web.py`:
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

    game/<area>/*.c          recovered game (generated from src/main), each with its services
    port/main.c              crt0's role: data image, devices, then the recovered main()
    port/core/               data arena, DGROUP stack model, routine link table, diagnostics,
                             event pump, platform glue of the services
    port/include/            port.h (layer contract), glue.h, services.h
    port/services/           native assembly-library and DOS/BIOS/driver services
    port/platform/           platform contract (platform.h) and SDL3 backend
    port/third_party/ymfm/   the OPL2 sound-chip emulator (BSD-3-Clause, vendored)
    port/gen/                generated: data placement, module data, symbols
    port/open/               the opening (OPEN.EXE): link callbacks, image, process, start-up, tests
    port/tools/              the generators

The host branch of every recovered module keeps the original 16-bit data
layout and reports each call that leaves the module to a service callback
(a numbered service, or in the battle modules the callee's original
address). `tools/make_game_tree.py` resolves those callbacks in `game/`
itself: every module ends with a native-build section (`#ifndef
OPEU_HOST_EVENTS`) holding its service tables `svc_<module>`, whose cases
call the recovered routine the original called (its host entry, with that
module's own table) or the native replacement of an assembly/DOS/BIOS
service, and the `PORT_ROUTINE` handlers of its routines. The tables come
from `tools/game-services.json` (mined from the former `port/link`, see
argument conversion. The validators build the same files with
`-DOPEU_HOST_EVENTS` (tools/host_tree/cc), which leaves the host branch
exactly as in `src/`.

### Recovered code and platform glue

The former `port/link` (342 files, 10,208 lines; 8,849 in per-module
files) is gone. What could not become a direct call inside a module stays
in port/, all of it platform glue:

- `include/glue.h`, `core/glue.c` (about 630 lines): the DGROUP stack
  helpers (scratch and frame enter/leave) and the frame-call helpers of the
  variadic `request_format`/`cursor_print`, one per module family because
  each family lays the words out differently in its stack frame (they are
  kept apart so the frames stay byte-identical); the battle reorder and
  call-by-name helpers over the link table; the INT 33h/66h register-block
  adapters; the prompt and file helpers (scenario load, picture reader,
  closing message); the bounded random helper that syncs the generator word
  at DS:c6f4; `port_install_hooks` (installs `svc_ai_unrecovered`). Shared
  by many modules or tied to the DGROUP model, so not part of any one
  module.
- `core/hosts.c` (about 100 lines): the host structs that textout, uiprim,
  dialogs and numdlg take instead of a bare callback (their callback plus
  the segment, stack and buffer addresses of the platform's DS layout).
- `core/routines.c` (about 110 lines): `PORT_ROUTINE` handlers of the 16
  native services named by address (VGALIB, SYSLIB, `getch`); the
  handlers of recovered routines are generated into their modules.
- `core/link.c` + `gen/link_table.c` (generated by `tools/gen_link_table.py`
  from `port/gen/symbols.json` and the `PORT_ROUTINE` handlers of `game/`
  and `port/`): the routine link table, see "Provenance".

## Provenance

- **Code.** All 332 `src/main` modules are compiled into
  `libopeu_recovered` as their generated `game/<area>/<m>.c` counterparts
  (same code, names from `tools/game-renames.json`, one header per module,
  plus each module's generated services); the port adds the platform glue,
  the native services and the start-up. `tools/port_lint.py
  --compile-commands=...` maps each compiled `game/<area>/<m>.c` to
  `src/main/<area>/<m>.c` (a game/ file without one is rejected), requires
  90% of them, and rejects any port/ file that defines a recovered routine
  (reference or game/ name) or a generated `svc_` table.
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
  their service callback. The link table (`<build>/gen/link_table.c`,
  generated by `port/tools/gen_link_table.py` from the symbol names and the
  `PORT_ROUTINE(name)` handlers in `game/` and `port/core/routines.c`;
  renamed routines are listed under their reference names through
  `game/compat/aliases.json`) maps such an identifier to the native C
  handler that calls the recovered routine. It is a sorted table lookup of
  names; it executes nothing but C functions.

## OPEN.EXE (the opening)

KOEI.COM ran FMDRV, then OPEN.EXE (the opening and title), then MAIN. The
port does the same in one process (`port/main.c`: `port_fm_init()`, then
`open_intro_run()`, then MAIN's entry). The opening plays first, in the same
window and with the same FM driver instance. Any key or mouse button skips
it, as in the original, and then MAIN starts. `--no-intro` skips the
opening. Headless runs skip it unless `--intro` is given, so the smoke
scripts' PNG and WAV output is byte-identical to a build without the
opening. Checked 2026-10-09 against a build of HEAD: `smoke-input.txt` for
15 s and `smoke-turn.txt` for 120 s, PNG and WAV.

**Compiled from `src/open`.**
- All 12 recovered C modules are compiled unchanged into the library
  `opeu_open_recovered` (one delimited block in `port/CMakeLists.txt`):
  `core/openmain.c outil.c otext.c oemsmap.c`, `io/osound.c` and
  `data/odems.c odopen.c odpal.c odsound.c odtext.c odutil.c ofmusic.c`.
- OPEN and MAIN were separate programs and reuse names with different host
  signatures. A forced include, `port/open/open_names.h`, renames the 189
  symbols the src/open modules define (`#define opeu_x open_opeu_x`).
  `port/open/tools/gen_open_names.py` generates it from `nm` of each module
  (`--check` verifies it is current).
- What the modules only reference resolves to MAIN's recovered modules:
  `opeu_string_length`, `opeu_copy_bytes` and `opeu_ascii_upper`, MAIN's
  STRINGS/MEMORY objects, which the original linked into OPEN.EXE
  unchanged. Their host signatures match.
- `ctest -R open_provenance` (`port/open/tests/check_open_provenance.py`)
  checks that all 12 modules are in the build, that no port file defines
  any of their 111 routines, and that the generated files are current.
  `tools/port_lint.py` passes.

**Link callbacks.** `port/open/link/<module>.c` do argument conversion only;
each file's header has its event table.
- `openmain.c`: 45 events.
- `outil.c`: 18 events.
- `otext.c`: 7 events and the host struct.
- `oemsmap.c`: 9 EMS calls.
- `osound.c`: the driver open, int86/int86x to INT 66h, and segread.

**Process memory.** `port/open/open_memory.c` builds OPEN's image in the
same data arena, with load segment 0 like MAIN's.
- DGROUP is at 093A and holds the objects of the seven data modules. The
  OFMUSIC song bank is at 0710:0002.
- `port/open/open_image.c` places the 78 objects. It is generated by
  `port/open/tools/gen_open_image.py` from the layout in
  `tests/validate_open_dgroup_data.py` and native `sizeof`s (`--check`
  verifies it).
- SS = DS, with SP = 3040h. The modeled stack (`open_sp`) serves OTEXT's
  conversion buffer and the EMS entries.
- OPEN has its own DOS allocator: first fit from DS + 305h, with an MCB
  paragraph before each block. OPEN allocates 0x501 + 0x6d5 + 6 x 0x1000
  paragraphs, more than MAIN's heap model holds.
- When OPEN returns, the arena is rebuilt for MAIN (`port_memory_init()`),
  as DOS loaded MAIN fresh.

What carries over to MAIN is what carried over in DOS: VRAM, the DAC and
attribute palette (MAIN draws its keyboard/mouse prompt in the palette OPEN
faded to), the resident FM driver's state, the BIOS keyboard buffer and the
mouse driver.

**Assembly library: reused because byte-identical.** OPEN's KOEI library was
compared with MAIN's instruction by instruction (labels normalized). These
ranges are identical, so OPEN's calls use MAIN's native service:

| OPEN | MAIN | What |
|---|---|---|
| OVGALIB 0130..0157 | 23AE | |
| OVGALIB 01AC..021B | 23D5 | set_palette |
| OVGALIB 021C..077E | 2444 | clip, pixel, draw/restore |
| OVGALIB 0913..09B7 | 2AAC | clear, box |
| OVGALIB 0CF3..1015 | 2B50 | |
| OVGALIB 1015..107A | 2ED8 | |
| OVGALIB 111A..11C7 | 2F3D | |
| OVGALIB 1264..1310 | 2FEA | |
| OVGADEC 13FA..1808 | 3096 | picture decoder |
| OCOMPOS 1BB9..1C2F | 301F | screen_copy |
| OSYSLIB 1310, 132A, 1337, 1352, 13D1 | 402E, 4052, 405F, 4048, 4066 | |
| OFILEIO | FILEIO | every routine OPEN calls |
| EMSDRV | EMSDRV | MAIN's object, linked unchanged (`port/services/ems.c`, no EMM) |

**Hand-written native routines.** These are OPEN-only assembly, in
`port/services/open_compos.c` and `open_syslib.c`. None is a game routine,
and the recovered C has none of them.
- OCOMPOS 1B56 bitmap_save: MAIN's 2ED8 plus a PUSH/POP pair. It writes
  through ES:DI, which wraps at 16 bits inside the destination segment, so
  it is written on segment:offset.
- OCOMPOS 1C2F far_copy (REP MOVSW), 1C74 buf_or, 1C94 buf_and_mask and
  1E2F dissolve: OPEN/END only.
- The dissolve reads the screen bytes it keeps without selecting a read
  plane. 1B56 and the picture decoder leave GC index 4 (the read map) at 3,
  so where the mask is clear, planes 0-2 receive plane 3's bits. The port
  models this with a tracked `read_map` rather than assuming it.
- OSYSLIB 133E sfx_play (INT 66h F001h), 134B sfx_busy (F000h) and 13E0
  sound_open: AH=0 with the bank, which is MAIN's 4074 without its AH=10h
  effect-bank call.
- OVGALIB 09B7: an empty far routine.

**Platform edits.** Each is documented in the file that makes it, with the
reason.
- Vertical retrace (`port/open/link/outil.c`, event 5). OVGALIB 01AC
  (= MAIN 23D5) polls 3DAh for the next retrace before it writes the DAC.
  MAIN's native `opeu_set_palette` leaves the wait out, because MAIN times
  nothing by it. OPEN's fades are paced by it: one set_palette per step and
  no other delay. The link waits for the next frame boundary of mode 12h's
  59.94 Hz on the host clock before the call. The boundaries are absolute,
  so there is no drift. Without the wait every fade took 0 ms, and the
  opening was already 1.5 s ahead when the music started.
- `dos_exists` (openmain event 31, the shutdown's wait for SCESEL.DAT) and
  the open-error spin (outil event 13) pump host events on every turn, so a
  window close or `--ms` ends them.
- Far sources of the two blitters (`source_bytes()` in `openmain.c`): LODS
  through DS:SI wraps, so a rectangle that would run past offset FFFFh gets
  a wrapped copy. OPEN's own rectangles never do.
- The recovered allocation-failure paths loop forever after the message
  (`for(;;)`), as in the original. With the port's allocator they are not
  reached.
- Diagnostic `OPEU_OPEN_DUMP=file`: writes OPEN's 64 KB from the bank
  segment at its exit, for the music replay below.

**Music.** The sequence of driver calls:
- Song 1 of OPEN's bank starts (AX=0101h, then 0401h) at tick 2173. That is
  7.46 s after OPEN's start; in DOSBox-X it is 7.52 s after OPEN's mode
  switch.
- The title issues 0400h.
- The exit path issues AH=1 AL=5, then AH=2 AL=20 (the fade).

Song 5 lies past the bank's three-song table. Its five track offsets (F407,
1388, 5002, 0003, 0000) point into OPEN's heap, DGROUP's stack area and the
bank.
- Until the exit (73 ticks, 0.25 s, under the fade) the port plays what its
  memory holds there. The heap part (picture buffers) has the original's
  content. The stack part is the port's modeled stack, not the original's C
  stack frames.
- After the exit, the port's arena has MAIN's far data at those addresses,
  where DOS had MAIN's code. The port's log shows only the fade's level
  writes and no key-on after it. MAIN opens its own bank only after its
  keyboard/mouse prompt.

`ctest -R open_music` (`port/open/tests/open_music_diff.py`) runs the opening
headless with a key at the title (140 s). It logs every INT 66h call with
its frame and every OPL write. It then replays the same calls at the same
PIT ticks on the original FMDRV.COM under `tests/validate_fmdrv.py`'s
Unicorn harness. The replay uses OPEN's bank, OPEN's memory from the song-5
call on (`OPEU_OPEN_DUMP`) and MAIN's arena after the exit. Result
2026-10-09:
- 40,722 calls returned identical AX/BX.
- 17,200 of 17,200 OPL writes are identical (tick, register, value) over
  43,695 ticks.
- The install writes are identical.

`port/open/tests/compare_open_dro.py` compares the port's log with
DOSBox-X's DRO of the original. Both are aligned at the first note, and
writes that repeat a register's value are dropped from both. Over 120 s of
song 1, 53 of 57 registers receive identical value sequences within 10 ms.
A0-A3 (the frequency low bytes) differ only in vibrato intermediate steps.
The cause is the phase of the driver's 62.1 Hz effect clock: in DOS, FMDRV
had been resident for a while before the song; in the port it is installed
at time 0.

**Frames.** `port/open/tests/dosbox_open_capture.py` records the original
under DOSBox-X: KOEI -> FMDRV -> OPEN, with every later .EXE replaced in a
scratch copy by an exit-100 stub. Settings: S3, 386, `cycles=fixed 20000`,
OPL2, and palette expansion `(x<<2)|(x>>4)`, the same as the port's.
`port/open/tests/compare_open_frames.py` matches each still scene of the
first loop (held >= 300 ms) to the native `--shots` frames (every 25 ms)
and compares pixels. Results 2026-10-09:
- All 32 non-black scenes of the first loop have a pixel-identical native
  frame. These are the KOEI logo, KOEI PRESENTS, A KOU SHIBUSAWA
  PRODUCTION, WORLD WAR SERIES, Panther, officer, trench, tanks, officer
  pointing, the six Panther variants, the small tank, the forest map, flak,
  artillery, the red gunner, the title, and the title with "Hit any
  button.". Every black or near-black scene has one too.
- The skip run (a key during the Panther picture) matches 8 of 8 scenes.
  The opening exits 79.21 s after its start, when song 1 ends (DOSBox-X:
  about 79.2 s).
- Native fades take 15 steps of one frame each (2256-2488 ms, against
  2252-2486), and holds match (2000 ms against 2002).
- The port runs ahead of DOSBox-X wherever pictures are drawn. The
  original's waits are relative to the end of drawing, and its absolute
  timeline waits never fire (TIMELINE_START(20000) is below every target).
  The port draws instantly.
- By the title, the lead is 12.8 s at 20000 cycles and 10.4 s at 100000
  cycles. About 3 s of it scales with the emulated CPU speed and about 10 s
  does not; at the first picture load that part is 706 ms of 1018 ms. A
  plausible cause is DOSBox-X's per-I/O time on the decoder's VGA port
  writes, but this is not established.
- Animation frames between still scenes are not compared.
- A full 150 s run with `--shots ... 25` writes about 5.4 GB of PNGs.

`ctest -R open_run` (`port/open/tests/check_open_run.py`) runs three
headless cases:
- the key-skip run: the opening exits at 78-80.5 s, then MAIN's prompt
  appears on a black screen;
- a key at the title: the opening exits within 2 s, and the prompt frame is
  the same;
- a plain headless run: the opening does not run.

The web build runs the opening under Node with the same output. `node
opeu.js --headless --intro --ms 145000 --input /host/.../key.txt --shots
/host/... 1000 --wav /host/...` wrote 145 PNGs, the final PNG and the WAV,
all byte-identical to the native build's.

## Status

### Screens reached (headless runs, PNG captures)

Input prompt, New Game / Load Game, the scenario list, the France briefing
(picture and text), the players menu, side, map type, battle view and
difficulty choices, the staff screens (Army Commander and Chief of
Operations assignment), then the
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
- `core/glue.c`, `core/hosts.c`, `core/routines.c`: the platform glue of
  the services (see "Recovered code and platform glue").
- `main.c`: crt0's role (data image, devices, then `opeu_application_entry`).
- `platform/`: SDL3 window/present, events, time, PNG writer; for the
  web build also the page shell and file-system set-up (`platform/web/`).

### Edited recovered routines

None. No file under `src/` is changed; `game/` is generated from it and
not edited by hand. Differences from the host branches are the generator's:
the native-build sections (argument conversion only) and the six aiops5
declarations replaced by their owners' prototypes. Native replacements live
in `port/services`.

### Boundary pointers installed by the port

- `opeu_ai_unrecovered` (aiops1.c:143): the AI modules' boundary for six
  callees, all now recovered. `svc_ai_unrecovered` (native-build section
  of `game/core/aiops1.c`, installed by `port_install_hooks`) maps the ids
  to them
  (0 `opeu_release_side_units`, 1 `opeu_release_side_elements`,
  2 `opeu_list_due_records`, 3 `opeu_objective_level`,
  4 `opeu_objective_eligible`, 5 `opeu_ai_05a8`), following
  `tests/validate_ai_ops.py`'s oracle bindings. Argument conversion only.

### Recovered routines that are missing (blockers)

- None for the turn loop: `opeu_path_fill` (19DF:006E) and
  `opeu_move_reachable` (32E6:00D8) are recovered in `src/`, and the
  services that reach them call them directly (the miner resolved the old
  `OPEU_HAVE_PATH_FILL`/`OPEU_HAVE_MOVE_REACHABLE` switches of `port/link`
  to "present").

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
