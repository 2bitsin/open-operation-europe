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
time is in its `configure.py`; here every `configure.py` only declares its C
sources' flags and include roots (the old port's `-std=c99 -Wall -Wextra ...`,
make the module's GoogleTest suite. Python `*.test/` suites run under pytest in the
same `./buildutil test`. `assets.embed/` in `sources/opeu/opeu/` is compiled
into the executable as `opeu::resources`. The layout reference is

The suites `./buildutil test` runs: build identity (platform), the VGALIB
and SYSLIB drawing checks and the file service (services), the embedded
asset table (opeu), the fixtures (integration), and two pytest cases in
integration: `open.test/` (the opening's skip, title key and plain runs)
and `audio.test/` (the smoke script's music: song starts, key-ons, audible
and unclipped WAV). Every test runs from a clean clone with only conan and
the embedded assets.

`buildutil.toml` sets the project name, `cxx_standard = 23`, the `contracts`
option and `[run] default = "opeu"`. The `contracts` option is one of
`stop`, `halt`, `complain` or `ignore`, and `sources/opeu/contract.hpp`
binds it.

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
and copied inside, so no Linux output lands in the tree.

```
python3 tools/ci/lane.py buildutil --buildutil-src ~/path/to/buildutil
```

The lane is `tools/ci/buildutil_lane.py`. It installs the packages above and
gcc 14 as the default, installs buildutil and a current cmake into a venv,
and then runs `./buildutil build` and `./buildutil test`. At v1.0 it passed
on arm64 with gcc 14.2. An x86_64 Linux machine is the same source but has
not been run.

## Emscripten (the browser build)

```
./buildutil build --compiler emscripten --release
python3 tools/web/serve.py --dir _install/opeu
```

buildutil offers the `emscripten` compiler only on Linux hosts. The SDK is
`$EMSDK/upstream/emscripten/em++`, else `em++` on `PATH`. `EMSDK` can be set
in `.env`. The output is `_install/opeu/opeu.html`, `opeu.js` and
`opeu.wasm`. The data files are inside the wasm, so there is no `.data`
preload file. The page source is `web/shell.html`. `web/pre.js` sets up the
IndexedDB mount for saved games and the export and import helpers.
`tools/web/serve.py` serves the build with `.wasm` as `application/wasm` and
`Cache-Control: no-store`. Open `http://localhost:8765/opeu.html`. Add
`?autostart` to skip the click-to-start overlay in automated checks.

The build needs no threads and no SharedArrayBuffer, so it needs no
cross-origin isolation headers.
