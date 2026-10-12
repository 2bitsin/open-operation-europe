# Building

The tree is the build script. `./buildutil` at the root is buildutil's own
launcher. On first use it creates the project venv in `_pyvenv/`, installs
buildutil 0.100 (and pytest; the project declares no further Python
dependencies, which would go in `buildutil.toml` `[venv] extra_deps`), and
reads `buildutil.toml`. From then on every command
is `./buildutil <verb>`.

## Prerequisites

- A C++23 compiler. The tree builds with Apple clang on macOS and with gcc 14
  on Linux. Ubuntu 24.04's default g++ 13 stops at `#include <print>`, so
  install `g++-14` and make it the default `gcc`/`g++`.
- Python 3.11 or newer.
- CMake 3.30 or newer and Ninja. buildutil writes `compiler.cppstd=26` into
  the generated gcc profile, and CMake 3.28 (Ubuntu 24.04) has no GNU flag
  for C++26, so the first conan source build fails with it. `pip install
  cmake` in the environment buildutil runs from is enough.
- A conan 2 remote, or conancenter (see below).

## Commands

```
./buildutil build                         # configure if needed, then build (release)
./buildutil build --debug                 # or --relwithdebinfo; each build type has its own _build/ tree
./buildutil test                          # build, then every ctest entry (GoogleTest and pytest)
./buildutil test opeu-platform            # one module's suites
./buildutil test --option contracts=halt  # the C++ suites in another contract mode
./buildutil run                           # build, install and start the game
./buildutil run -- --headless --ms 15000 --png out.png
./buildutil run -- --saves /tmp/opeu-saves   # another save directory
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

## Modules

Every directory under `sources/opeu/` with a `CMakeLists.txt` is a module.
That file holds `Init_submodule()` and `Link_dependencies(...)`, with a
comment for each dependency saying why. Anything a module generates at build
time is in its `configure.py`; here every `configure.py` only declares its
sources' flags and include roots (the old port's `-Wall -Wextra ...`,
buildutil compiles it as C++23 (`cxx_standard` below); the recovered code is
make the module's GoogleTest suite. Python `*.test/` suites run under pytest in the
same `./buildutil test`. `assets.embed/` in `sources/opeu/opeu/` is compiled
into the executable as `opeu::resources`. The layout reference is

The suites `./buildutil test` runs: build identity (platform), the VGALIB
and SYSLIB drawing checks and the file service (services), the embedded
asset table (opeu), the fixtures (integration), and two pytest cases in
integration: `open.test/` (the opening's skip, title key and plain runs)
and `audio.test/` (the smoke script's music: song starts, key-ons, audible
and unclipped WAV). After ctest, `./buildutil test` runs every
`tools/*/pytest.ini` suite: `tools/census/` is the legacy census gate (no
idiom count above `tools/census/baseline.json`, finished modules at zero;
`python3 tools/census/census.py` prints the table, and
a clean clone with only conan and the embedded assets.

`buildutil.toml` sets the project name, `cxx_standard = 23`, the `contracts`
option and `[run] default = "opeu"`. The `contracts` option is one of
`stop`, `halt`, `complain` or `ignore`, and `sources/opeu/contract.hpp`
binds it.

## Warnings

A finished module compiles under the warnings gate: its `configure.py`
declares every source with

```
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
```

(`bc.declare(source, options=WARNINGS + ...)`), so the ordinary `./buildutil
build` is the warnings build and any new warning stops it. glue, platform,
intro and opeu have it; turning it on for another module is the same list
in that module's `configure.py` and the fixes it asks for. The conversions
the gate asks for are explicit casts that keep the value the implicit
conversion gave. Proven with Apple clang; gcc reports some conversions
clang does not, so the first Linux build of a newly gated module may need
a few more casts.

## clang-tidy

`.clang-tidy` at the root enables `modernize-*`, `cppcoreguidelines-*` and
`readability-*` (with the owning-memory, bounds, C-array and loop-convert
checks among them) less the house exceptions it lists with their reasons
(short coordinate names, the game's numbers, clang-format's brace and
table layout, lower-case literal suffixes, unions, do-while). The checks
are noisy on legacy code; they apply to finished modules. A legacy
directory may relax checks in its own `.clang-tidy`
(`InheritParentConfig: true`) with the reason: `glue/` and `intro/` turn
off the checks that name the 16-bit memory model (globals, C arrays,
pointer arithmetic, unchecked indexing, reinterpret casts, the
printf-shaped trace, `goto`) until M2 deletes it, and `port.hpp` carries
the same exceptions in a `NOLINTBEGIN` block for the modules that include
it. Every other remaining finding has a `NOLINT` with its reason.

Run it over a module from the compile database of a build (LLVM's
clang-tidy; on macOS `brew install llvm`):

```
./buildutil build
/opt/homebrew/opt/llvm/bin/run-clang-tidy -quiet \
    -p _build/armv8-macos-apple-clang-release \
    -header-filter='sources/opeu/platform/' 'sources/opeu/platform/'
```

The positional argument is a regular expression over the source paths,
so `'sources/opeu/(glue|platform|intro|opeu)/'` runs the finished modules
together; `-fix` applies the automatic fixes. glue, platform, intro and
opeu report nothing.

## Dependencies and the conan remote

`sources/CMakeLists.txt` names each dependency once with `Require()`. The
same line feeds `find_package` and the conan graph:

| Package | Used for |
|---|---|
| SDL3 (3.4 series) | window, input, timer and audio (`sources/opeu/platform/`); conan's `sdl` natively, the Emscripten SDK's own port in the browser |
| stb | `stb_image_write`, the headless `--png` and `--shots` output (one header and one call; libpng would add zlib, setjmp error handling and a write-struct lifecycle). Rows are written with filter type 0, as the port's own writer did, so the pixels are the same as v1.0's and the PNG readers need no unfiltering |
| GTest | the C++ test suites |
| benchmark | benchmarks (built only when benchmarks are) |

The remote is read from a gitignored `.env` at the root, which buildutil
loads on every run (a real environment variable wins over the file):

```
CONAN_REMOTE_URL=https://.../artifactory/api/conan/<repo>
CONAN_REMOTE_NAME=<name>
CONAN_REMOTE_USER=<user>
CONAN_REMOTE_PASS=<token>
```

GitLab CI's `CI_ARTIFACTORY_{HREF,NAME,USER,PASS}` are accepted as fallback
spellings, and this checkout's `.env` uses them. With credentials, buildutil
uploads the binaries it built after each build, so later builds on other
machines download them instead of compiling SDL's graph again. Without a
remote, conancenter is used and nothing is uploaded.

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
rights and let conan install them. The list is the one in
`tools/ci/buildutil_lane.py`.

gcc gives about 25 warnings that clang does not, all in recovered code and in
ymfm. None of them is an error.

### Container lanes

`tools/ci/lane.py` runs a lane in a Docker `ubuntu:24.04` container. It uses
`arm64v8/ubuntu:24.04` on an arm64 host. The checkout is mounted read-only
and copied inside, so no Linux output lands in the tree. What a lane hands
back goes to `--out` (default `_local/lane-out/<lane>/`).

```
python3 tools/ci/lane.py buildutil --buildutil-src ~/path/to/buildutil
python3 tools/ci/lane.py web --buildutil-src ~/path/to/buildutil
```

`buildutil` is `tools/ci/buildutil_lane.py`. It installs the packages above
and gcc 14 as the default, installs buildutil and a current cmake into a
venv, and then runs `./buildutil build` and `./buildutil test`. At v1.0 it
passed on arm64 with gcc 14.2, and it passes again since the modules were
wiring"): every executable links with GNU ld and the 14 tests pass. An
x86_64 Linux machine is the same source but has not been run.

`web` is `tools/ci/web_lane.py`, the Emscripten lane below. It clones emsdk
inside the container and installs the pinned SDK (6.0.10, whose SDL3 port is
3.4.2), installs buildutil and cmake into a venv, runs `./buildutil build
--compiler emscripten --release --no-tests`, fails if a `.data` preload file
was installed, and runs the smoke script headless under the SDK's node from
the installed `opeu.js`. It writes `opeu.{html,js,wasm}`, `sizes.txt`,
`build.log`, the smoke's `web-smoke.{png,wav,opl}` and their SHA-1 sums in
`web-smoke.sha1` to the out directory. Compare those with a native run of the
same script:

```
./buildutil run -- --saves _local/smoke/saves --headless --ms 15000 \
  --input sources/opeu/integration/fixtures/smoke-input.txt \
  --png _local/smoke/native.png --wav _local/smoke/native.wav --opl-log _local/smoke/native.opl
```

On 2026-10-09 (arm64, emsdk 6.0.10) the three files were byte-identical to
the macOS native build's.

## Emscripten (the browser build)

```
./buildutil build --compiler emscripten --release --no-tests
python3 tools/web/serve.py --dir _install/opeu
```

buildutil offers the `emscripten` compiler only on Linux hosts. The SDK is
`$EMSDK/upstream/emscripten/em++`, else `em++` on `PATH`. `EMSDK` can be set
in `.env`. The output is `_install/opeu/opeu.html`, `opeu.js` and
`opeu.wasm` (7.9 KB, 206 KB and 4.9 MB with emsdk 6.0.10). The data files are
inside the wasm, so there is no `.data` preload file. The test suites are
native-only, hence `--no-tests`.

Nothing in `buildutil.toml` selects the lane. `sources/CMakeLists.txt`
requires SDL3 from the SDK's own port on Emscripten (`Require(SDL3 ...
SYSTEM PLATFORM Emscripten)`). `cmake/browser.cmake` (a no-op outside
Emscripten) prepares that port, puts `cmake/modules/` with `FindSDL3.cmake`
on the module path, and gives the application its link options: ASYNCIFY,
memory and stack sizes, the file system with IDBFS and NODEFS,
`--pre-js web/pre.js` and `--shell-file web/shell.html`. It follows
scooby-engine's `cmake/browser.cmake`. A `configure.py` can link libraries
and set source flags but not an executable's link options, which is why this
§4.9).

The page source is `web/shell.html`. `web/pre.js` mounts IDBFS at `/saves`,
the save directory on Emscripten, and provides the export and import helpers.
Under node, `/saves` is MEMFS and the host file system is at `/host`, so a
headless run reads and writes host files:

```
node _install/opeu/opeu.js --headless --ms 15000 --input /host/$PWD/sources/opeu/integration/fixtures/smoke-input.txt --png /host/tmp/web.png
```

`tools/web/serve.py` serves the build with `.wasm` as `application/wasm` and
`Cache-Control: no-store`. Open `http://localhost:8765/opeu.html`. Add
`?autostart` to skip the click-to-start overlay in automated checks.

The build needs no threads and no SharedArrayBuffer, so it needs no
cross-origin isolation headers.
