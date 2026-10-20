# Building

Operation Europe, native: KOEI's 1994 DOS game recovered into C++ source and
running on SDL3 (macOS, Linux, the browser). This page is everything needed
to build it, give it the game's data, run it and test it.

The tree is the build script. `./buildutil` at the root is buildutil's own
launcher: on first use it creates the project's Python environment in
`_pyvenv/`, installs buildutil (vendored in `.buildutil/`, so no package
index is needed for it) and its pinned tools, and reads `buildutil.toml`.
From then on every command is `./buildutil <verb>`.

## Prerequisites

- git, and Python 3.11 or newer.
- A C++23 compiler: Apple clang (Xcode or its command line tools) on macOS,
  gcc 14 on Linux. Ubuntu 24.04's default g++ 13 stops at `#include
  <print>`, so install `g++-14` and make it the default `gcc`/`g++`.
- CMake 3.30 or newer and Ninja. buildutil writes `compiler.cppstd=26` into
  the generated gcc profile, and CMake 3.28 (Ubuntu 24.04) has no GNU flag
  for C++26, so the first conan source build fails with it. `pip install
  cmake` into `_pyvenv/` is enough.
- Network access to conancenter on the first build (see Dependencies).
- To play: your own copy of the 1994 DOS release of *Operation Europe: Path
  to Victory 1938-45*. The repository holds no game files.

## Getting started

```
git clone <this repository> opeu && cd opeu
./buildutil install                          # the project venv and the vendored buildutil
./buildutil build                            # conan fetches SDL3 and the rest; cmake and ninja build
./buildutil run -- --data ~/games/OPEUROPE   # the directory of your DOS copy
```

A first build on a laptop takes about two minutes: about 15 s for the
Python environment, 15 s for conan (packages from conancenter, gtest built
from source), 5 s to configure and the rest to compile.

## The game's data

The game reads the 26 data files of the 1994 English DOS release (their
names, sizes and fingerprints are in `sources/opeu/assets/data-files.hpp`).
Point it at the directory of your copy, or put the files where it looks.
It takes them from the first of these that applies:

1. `--data DIR`: that directory and no other.
2. Data compiled into the executable, when the tree it was built from held
   the files in `sources/opeu/opeu/assets.embed/` (this repository does not).
3. The first directory that holds all 26, searched in this order:
   `$OPEU_DATA_DIR`; the executable's own directory (`_install/opeu/` after
   `./buildutil run`); the per-user data directory, SDL's preference path
   plus `data/` (`~/Library/Application Support/opeu/Operation Europe/data/`
   on macOS, `~/.local/share/opeu/Operation Europe/data/` on Linux); the
   current directory.

A directory counts only when every one of the 26 files is there (in any
letter case) with the release's size and fingerprint; anything else in it
(the DOS programs, `SAVEDATA`) is ignored, so the installed game directory
works as it is. A file that is missing or not the release's stops the game
with a report: which directories it tried and, file by file, what the
closest one lacks, also shown in a message box when the game has a window;
the exit code is 3.

```
./buildutil run -- --check-data ~/games/OPEUROPE   # that report for one directory, without playing (exit 0 or 3)
./buildutil run -- --check-data                    # the same for the search above
```

The pictures, tiles, portraits and glyphs are decoded from the data files in
memory at start-up (a few milliseconds). Saved games go to the per-user
directory (`--saves DIR` chooses another). A remastered asset pack, if you
have one, is layered over the classic images with `--remaster DIR` or
`$OPEU_REMASTER`.

## Commands

```
./buildutil build                         # configure if needed, then build (release)
./buildutil build --debug                 # or --relwithdebinfo; each build type has its own _build/ tree
./buildutil test                          # build, then every ctest entry (GoogleTest and pytest)
./buildutil test opeu-platform            # one module's suites
./buildutil test --option contracts=halt  # the C++ suites in another contract mode
./buildutil run -- --data DIR             # build, install and start the game
./buildutil run -- --data DIR --headless --ms 15000 --png out.png
./buildutil run -- --data DIR --saves /tmp/opeu-saves   # another save directory
./buildutil run -- --help                 # the game's own options
./buildutil clean                         # drop _build/; --nuke also drops _pyvenv/ and _conanhome/
./buildutil --help                        # every verb and option
```

Build output goes to `_build/<arch>-<os>-<compiler>-<buildtype>/` and
installs to `_install/`. The conan cache is `_conanhome/`, the rendered cmake
machinery `_bdudata/` and the conan profiles `_profiles/`. Every root entry
that starts with `_` is ignored by git (`/_*`). So are the files buildutil
regenerates on every configure: `CMakeUserPresets.json`, `.clangd` and the
`.vscode` task files.

Long first builds may need `--watchdog-budget SECONDS`, because the watchdog
fails any command after three minutes by default. The first Linux build
compiles SDL's whole dependency graph from source.

## Layout

Every directory under `sources/opeu/` with a `CMakeLists.txt` is a module:
`opeu` (the application and its entry), `game` (the game logic), `intro`
(the opening), `glue`, `services` (the DOS and BIOS services and the FM
sound driver rewritten as native code), `platform` (SDL3: video, input,
timer, audio), `assets` (a decoder per data format, the asset pack),
`render` (SDL_Renderer), `ymfm` (the OPL2 emulation, BSD-3-Clause),
`probes` (diagnostic tools) and `integration` (end-to-end tests). A
module's `CMakeLists.txt` holds `Init_submodule()` and
`Link_dependencies(...)`; its `configure.py` declares its sources' flags.
Every source is C++23. `*.test.cpp` files and `*.test/` directories are the
module's GoogleTest suite, or a pytest suite for Python. A directory named
`*.embed/` is compiled into the executable when the tree holds it.

`buildutil.toml` sets the project name, `cxx_standard = 23`, the `contracts`
option and `[run] default = "opeu"`. The `contracts` option is one of
`stop`, `halt`, `complain` or `ignore`, and `sources/opeu/contract.hpp`
binds it.

## Tests

`./buildutil test` runs every ctest entry (the GoogleTest suites and each
module's pytest suites, among them the functional suite that plays scripted
games headless and compares frames, sound and saves with goldens), then
every `tools/*/pytest.ini` suite.

Without game data the tests that need it skip, each with a message that
starts with `no game data:`; every other test runs. With your copy in
`OPEU_DATA_DIR` those run too:

```
OPEU_DATA_DIR=~/games/OPEUROPE ./buildutil test
```

A few still skip with it: those that compare with a golden made from the
game's data, which this repository does not carry. A directory
`OPEU_DATA_DIR` names must hold all 26 files; an incomplete one fails the
tests. `python3 tools/testing/skips.py --log LOG --game-data` checks a test
log: every skip must come from the game-data helpers
(`sources/opeu/game-data.test.hpp`, `tools/testing/game_data.py`).

The headless options make a run reproducible: on virtual time, from a
scripted input log, it writes PNG frames, a WAV of the sound and a log of
every OPL register write, and the same script always gives the same bytes:

```
./buildutil run -- --data DIR --headless --mute --ms 15000 \
  --input sources/opeu/integration/fixtures/smoke-input.txt \
  --png _local/smoke.png --wav _local/smoke.wav --opl-log _local/smoke.opl
```

The benchmark gates (`sources/opeu/game/game.bench/`,
`sources/opeu/services/services.bench/`) and the run-time gate
(`sources/opeu/integration/perf.test/`) compare timings with
`sources/opeu/integration/fixtures/perf/baseline.json`, scaled by how busy
the machine is at that moment; on a machine the baseline does not list they
apply the closest machine's numbers at a wide budget.
`OPEU_PERF_BUDGET=<percent>` sets the budget. To re-baseline after a
deliberate change, record this machine's entry from a clean Release build:
`_pyvenv/bin/python sources/opeu/integration/perf.test/perf.py baseline _build/<profile>`.

## Warnings

A finished module compiles under the warnings gate: its `configure.py`
declares every source with

```
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
```

(`bc.declare(source, options=WARNINGS + ...)`), so the ordinary `./buildutil
build` is the warnings build and any new warning stops it. glue, platform,
intro, opeu and render have it; turning it on for another module is the same list
in that module's `configure.py` and the fixes it asks for. The conversions
the gate asks for are explicit casts that keep the value the implicit
conversion gave. Proven with Apple clang; gcc reports some conversions
clang does not, so the first Linux build of a newly gated module may need
a few more casts.

## Dependencies

`sources/CMakeLists.txt` names each dependency once with `Require()`. The
same line feeds `find_package` and the conan graph:

| Package | Used for |
|---|---|
| SDL3 (3.4 series) | window, input, timer and audio (`sources/opeu/platform/`); conan's `sdl` natively, the Emscripten SDK's own port in the browser |
| stb | `stb_image`, the asset pack's PNG reader; `stb_image_write`, the headless `--png` and `--shots` output (one header and one call; libpng would add zlib, setjmp error handling and a write-struct lifecycle). Rows are written with filter type 0, so the PNG readers need no unfiltering |
| nlohmann_json | the asset pack's manifest and archive directory (`sources/opeu/assets/`); header-only |
| GTest | the C++ test suites |
| benchmark | benchmarks (built only when benchmarks are) |

With no remote configured, conan resolves every package from conancenter
and builds what has no binary for your compiler (gtest, and on Linux SDL3
with its options). A private conan 2 remote can serve the binaries instead:
put `CONAN_REMOTE_URL`, `CONAN_REMOTE_NAME`, `CONAN_REMOTE_USER` and
`CONAN_REMOTE_PASS` in a `.env` at the root (gitignored; a real environment
variable wins over the file), and buildutil uses it and uploads what it
built.

## Linux

`buildutil.toml` `[conan] options_linux` turns off SDL's PulseAudio and sndio
backends. PulseAudio 17 is a large meson source build. ALSA reaches PipeWire
and PulseAudio desktops through their ALSA plugins, and sndio is for the
BSDs. X11, Wayland, OpenGL/EGL, udev and D-Bus stay on.

conan's `sdl` recipe reaches the host through `*/system` recipes. With the
default `tools.system.package_manager:mode=check`, it refuses to continue
when a package is missing. On Ubuntu 24.04, install these packages:

```
build-essential g++-14 gcc-14 cmake ninja-build git python3 python3-venv python3-pip pkg-config
libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxau-dev libxaw7-dev
libxcomposite-dev libxcursor-dev libxdamage-dev libxdmcp-dev libxext-dev libxfixes-dev libxi-dev
libxinerama-dev libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxrandr-dev libxrender-dev
libxres-dev libxss-dev libxt-dev libxtst-dev libxv-dev libxxf86vm-dev libxcb-glx0-dev
libxcb-render0-dev libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev libxcb-image0-dev
libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev
libxcb-xinerama0-dev libxcb-dri3-dev uuid-dev libxcb-cursor-dev libxcb-dri2-0-dev
libxcb-present-dev libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev libxcb-util-dev
libgl-dev libegl-dev libudev-dev xkb-data
```

Alternatively, pass `-c tools.system.package_manager:mode=install` with sudo
rights and let conan install them. `tools/github/ci.py` installs the same
list on the CI runners.

gcc gives about 25 warnings that clang does not, all in recovered code and in
ymfm. None of them is an error.

## The browser build

```
./buildutil build --compiler emscripten --release --no-tests
python3 tools/web/serve.py --dir _install/opeu
```

buildutil offers the `emscripten` compiler only on Linux hosts. The SDK is
`$EMSDK/upstream/emscripten/em++`, else `em++` on `PATH` (`EMSDK` can be set
in `.env`); the CI builds with emsdk 6.0.10. The output is
`_install/opeu/opeu.html`, `opeu.js` and `opeu.wasm`. The test suites are
native-only, hence `--no-tests`. `tools/web/serve.py` serves the build with
`.wasm` as `application/wasm`; open `http://localhost:8765/opeu.html` (add
`?autostart` to skip the click-to-start overlay). The build needs no
threads and no SharedArrayBuffer, so no cross-origin isolation headers.

The page asks for the game's data the first time: it shows the report and a
drop zone (or a file picker) for the 26 files of your copy, stores them in
the browser (IndexedDB, mounted at `/data`) and reloads; the next visit
finds them there. "Forget game data" under the saved games removes them.
Saved games live in IndexedDB too (`/saves`), with export and import
buttons.

`sources/CMakeLists.txt` requires SDL3 from the SDK's own port on
Emscripten. `cmake/browser.cmake` (a no-op elsewhere) prepares that port and
gives the application its link options: ASYNCIFY, memory and stack sizes,
the file system with IDBFS and NODEFS, `--pre-js web/pre.js` and
`--shell-file web/shell.html`. Under node, `/saves` and `/data` are in
memory and the host file system is at `/host`, so a headless run reads and
writes host files:

```
node _install/opeu/opeu.js --data /host/$HOME/games/OPEUROPE --headless --ms 15000 \
  --input /host/$PWD/sources/opeu/integration/fixtures/smoke-input.txt --png /host/tmp/web.png
```

## Continuous integration

`.github/workflows/build.yml` runs on every push to `main`, on tags `v*`,
on pull requests and by hand. Each job is one call of
`python3 tools/github/ci.py <job>`, which prints each step as a group of the
log and on a failure names the step and what to look at; the buildutil logs
are the job's artifact.

| Job | Runner | What it does |
|---|---|---|
| `linux` | ubuntu-24.04 | the packages of "Linux" above with gcc 14 as the default, `./buildutil build --release`, `opeu --dump-arena` against its hash, `./buildutil test` (only `no game data` skips allowed, the benchmark gates at `OPEU_PERF_BUDGET=100`) |
| `macos` | macos-15 | the same with the runner's Xcode |
| `web` | ubuntu-24.04 | the pinned emsdk, `./buildutil build --compiler emscripten --release --no-tests` |

Conan resolves every package from conancenter; `_conanhome/`, `_pyvenv/`
and the emsdk are cached, keyed on the files that decide them. A job runs
the same way on a machine like the runner (`python3 tools/github/ci.py
linux` installs packages with sudo, so use a throwaway machine or
container).
