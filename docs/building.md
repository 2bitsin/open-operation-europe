# Building

The tree is the build script. `./buildutil` at the root is buildutil's own
launcher. On first use it creates the project venv in `_pyvenv/`, installs
buildutil 0.100 (and pytest; the project declares no further Python
dependencies, which would go in `buildutil.toml` `[venv] extra_deps`), and
reads `buildutil.toml`. From then on every command
is `./buildutil <verb>`. `.buildutil/` is the vendored copy of buildutil
0.100.0 that `./buildutil install` wrote; it is tracked so a clone builds
without network access to the buildutil package, and `./buildutil install`
refreshes it when the launcher is upgraded.

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
its `link` row counts the routine link table and its `callback` row the
event-typed parameters and `Call(` sites, the two that M2 takes to zero;
its `fixed_ptr` row counts every fixed-width integer pointer
(`u?intN_t*`, `uintptr_t*`, `const` included; matches, not lines), the
user's metric, which only falls; `int86`, `regs`, `far_pointer` and
`reg_field` count the DOS interrupt calls, their register blocks, the far
pointer fields, the register field accesses (`.ax`, `->dl`) and the C
memory and string routines (`c_mem`: `memset`, `memcpy`, `strcmp`, ...)
the C stdio file calls (`c_stdio`: `fopen`, `fread`, ...) and the
includes of C headers (`c_lib_header`: `<cstring>`, `<cstdlib>`,
`<string.h>`, `<stdlib.h>`; `c_header`: any `<name.h>`), and the
remnant rows (`koei_names`, `variadic`, `printf`, `printf_family`,
`c_runtime`, `seg_off`, `void_cast`, `c_comment`, `preproc_if`,
`reinterpret_cast`, `c_int_types`, `legacy_prefix`, `placeholder`),
comments included (`c_comment` counts `/*` openers only);
`python3 tools/census/census.py` prints the table of `HEAD`, counted from a
`git archive` export, never the working tree, and
starts from the baseline committed at `HEAD`, not the working file, and
warns about any other row the working file changes, so concurrent
re-baselines cannot revert each other). Every test runs from
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
intro, opeu and render have it; turning it on for another module is the same list
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
together; `-fix` applies the automatic fixes. Since
glue, platform, intro and opeu report 126, 23, 153 and 5 identifier
findings: all legacy names that the rename tool (`tools/rename/`, #17)
renames. They report nothing else.

## Benchmarks and profiling

The rule: no change may make the game notably slower. Three gates in
`./buildutil test` hold it, against the numbers in
`sources/opeu/integration/fixtures/perf/baseline.json`:

- `sources/opeu/integration/perf.test/` (in `opeu-integration-pytest`):
  every run of the functional suite (the six one-hour demos, the
  battle-viewed demo, the three smokes, the save and load runs; a Debug
  build times only the four runs that stay under 3 s there). Headless play
  runs in virtual time as fast as the CPU allows, so a run's cost is the CPU
  time (user + system) of its `opeu` process: the median of 3 plays, played
  3 more times and judged on all 6 when over its limit, plus the suite's
  total. The wall time is recorded beside it. It takes about 12 s.
- `sources/opeu/services/services.bench/benches.test.py` and
  `sources/opeu/game/game.bench/benches.test.py` (in
  `opeu-services-pytest` and `opeu-game-pytest`): every google-benchmark
  case of `opeu-services-benches` (the canvas: blits, glyphs, columns,
  fills, lines, sprite compose, picture decode, render and present) and
  `opeu-game-benches` (the battle calculation and pathfinding: the path
  flood in its three modes, the walk down the flooded costs, the move
  costs, element ratings, the element at every cell, the exit marking).
  The gate takes the fastest of 5 repetitions per benchmark; the ones over
  their limit are measured again, 5 repetitions at a time, for up to 20 s.
  Noise only ever slows a benchmark down, so the fastest repetition is its
  true cost and a real regression fails every attempt. The gates configure
  the tested build tree with `BUILD_BENCHMARKING=ON` and build the
  benchmark executables themselves; buildutil passes the switch on every
  configure, so the next `./buildutil build` turns it off again.

The budget is 10 % over the baseline of the same machine
(`OPEU_PERF_BUDGET=<percent>` overrides it). A shared machine is the hard
the efficiency cores, the same binary plays up to a third slower (measured
on the 8-core M2 at load averages of 14 to 43). So every limit is
multiplied by how slow the machine is right then:

- for the runs, by the median CPU time of calibration processes (a fixed
  pure-Python workload of about 0.5 s, `CALIBRATION_WORKLOAD` in
  `perf.py`) that run in the same pool as the plays, one after every three,
  against the median recorded beside the baseline. They meet the same load
  as the plays: across sessions the ratio of v1.0.1's suite time to the
  calibration held within 5 to 8 % while the suite time itself moved by
  12 to 21 %. A calibration benchmark inside a benchmark executable did not
  track the plays at all;
- for the benchmarks, by how much slower than at the baseline the same
  executable runs its `Calibration` benchmark (`services.bench/calibration.hpp`:
  arithmetic over a 256 KiB working set, code no change to the game
  touches), never by less than 1 on the baseline's machine.

With that, the three gates passed all six times they were run together
(five Release, one Debug) at load averages of 14 to 43 (with six extra processes sweeping 64 MiB each).

The baseline is per machine (CPU model, core count, OS family,
architecture, Python version, since the calibration is Python) and per
build type (release, debug; RelWithDebInfo is judged as release). On a
machine it does not list, the numbers of the closest listed one (the same
architecture first, then the OS family, the CPU model, the core count)
apply, scaled the same way, and the budget is 50 % (again
`OPEU_PERF_BUDGET`): the calibration cannot capture every difference
between two machines, so there the gate catches only gross regressions
until the machine has its own entry. Collection reads only the baseline
file, so the suites collect on any machine; a build type no machine lists
fails the tests with the command that records it.

The baseline also holds `reference.v1.0.1`: the same runs played by a
Release build of v1.0.1 (a524a29), the last snapshot before the refactor,
measured the same way, so the refactor is measured against the original C
as well as against the last baseline. The first baseline (2e57d779) found
HEAD 29 % over v1.0.1 for the whole suite, the demos 21 to 60 % over: the
profile of demo-kursk put the cell block copy of the scrolling map view
(48 short `Canvas::Move` calls per 24x24 block, each through a non-inlined
guard and a general `memmove`) at the top. 23249d74 copies those blocks
with inline fixed-size moves; at its baseline (e22cd832, v1.0.1 re-recorded
in the same session) the suite is 6 % under v1.0.1, the demos 8 to 21 %
under, demo-france-battles 3 % over, and the short menu runs 1 to 16 %
over (10 to 20 ms each; per-call blit and glyph cost, to be measured with
`xctrace` once Xcode is installed). The Debug block copy rose from 11.2 to
14.8 us because the inline move is not inlined there; the baseline records
it.

### Running

```
./buildutil bench opeu-services          # the canvas benchmarks (google-benchmark's console table)
./buildutil bench opeu-game              # the battle and pathfinding benchmarks
./buildutil test opeu-integration        # the run gate (with the functional suite)
_pyvenv/bin/python sources/opeu/integration/perf.test/perf.py measure _build/<profile>   # all numbers, nothing written
```

A failure names the run or benchmark, its time, the limit, the baseline
and the machine's slowdown. Profile it (below) before anything else.

### Re-baselining

After a deliberate change (a run added to the suite, a benchmark added, a
speed-up worth keeping, a new machine), measure clean exports, never the
shared working tree, under `_local/perf/`, Release and Debug, with the
project venv's Python (the one `./buildutil test` runs the gate with):

```
mkdir -p _local/perf/head _local/perf/v1.0.1
git archive HEAD | tar -x -C _local/perf/head && git rev-parse HEAD > _local/perf/head/.perf-rev
git archive a524a29 | tar -x -C _local/perf/v1.0.1 && echo a524a29 > _local/perf/v1.0.1/.perf-rev
(cd _local/perf/head && ./buildutil build --release && ./buildutil build --debug)
(cd _local/perf/v1.0.1 && ./buildutil build --release)
_pyvenv/bin/python sources/opeu/integration/perf.test/perf.py baseline _local/perf/head/_build/<profile>-release
_pyvenv/bin/python sources/opeu/integration/perf.test/perf.py baseline _local/perf/head/_build/<profile>-debug
_pyvenv/bin/python sources/opeu/integration/perf.test/perf.py reference _local/perf/v1.0.1/_build/<profile>-release/bin/opeu v1.0.1
```

`baseline` plays every run 7 times with the calibration processes among
the plays and records medians and minima of CPU and wall time and the
calibration median, runs every benchmark 7 x 5 times, and writes the entry
of this machine and build type, leaving the others. The `.perf-rev` file
names the revision recorded. Commit the baseline on its own, with the
table of what moved in the body.

### Profiling

macOS (Instruments' command line; Xcode's Instruments app opens the trace):

```
xcrun xctrace record --template 'Time Profiler' --output _local/prof/kursk.trace \
  --launch -- _build/<profile>/bin/opeu --headless --ms 3600000 --input <script>
open _local/prof/kursk.trace
sample opeu 10 -file _local/prof/kursk.sample.txt   # a quick call-tree of a running process
```

`python3 sources/opeu/integration/play.test/play.py run <opeu> demo-kursk
_local/prof/kursk` plays a run and leaves its script in
`_local/prof/kursk/input.txt`; a longer `--ms` gives `sample` time to
attach.

Linux:

```
perf record -g --call-graph dwarf -o _local/prof/perf.data -- _build/<profile>/bin/opeu --headless ...
perf report -i _local/prof/perf.data --no-children
```

A benchmark profiles the same way with its executable
(`_build/<profile>/sources/opeu/services/opeu-services-benches
--benchmark_filter=Render --benchmark_min_time=5s`). Release builds carry
symbols for the call tree with `./buildutil build --relwithdebinfo`, which
the gate judges against the release baseline.

clang's instrumented profile counts every function and line (exact, not
sampled) for a run: configure a separate tree with
`-fprofile-instr-generate -fcoverage-mapping` in `CMAKE_CXX_FLAGS` and
`CMAKE_EXE_LINKER_FLAGS`, play the run with
`LLVM_PROFILE_FILE=_local/prof/run.profraw`, then

```
xcrun llvm-profdata merge -o _local/prof/run.profdata _local/prof/run.profraw
xcrun llvm-cov report _build/<profile>/bin/opeu -instr-profile=_local/prof/run.profdata
xcrun llvm-cov show _build/<profile>/bin/opeu -instr-profile=_local/prof/run.profdata -name=<function>
```

is that build; its counts double as a profile of how often each line runs. The same
`.profdata` feeds `-fprofile-instr-use` for a profile-guided build.

## Dependencies and the conan remote

`sources/CMakeLists.txt` names each dependency once with `Require()`. The
same line feeds `find_package` and the conan graph:

| Package | Used for |
|---|---|
| SDL3 (3.4 series) | window, input, timer and audio (`sources/opeu/platform/`); conan's `sdl` natively, the Emscripten SDK's own port in the browser |
| stb | `stb_image`, the asset pack's PNG reader; `stb_image_write`, the headless `--png` and `--shots` output (one header and one call; libpng would add zlib, setjmp error handling and a write-struct lifecycle). Rows are written with filter type 0, as the port's own writer did, so the pixels are the same as v1.0's and the PNG readers need no unfiltering |
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

## Continuous integration

`.gitlab-ci.yml` runs on every push to `main`, on every merge request
(one pipeline per MR, not a second one for its branch) and from the *Run
pipeline* button. Each job's script is one call of
`python3 tools/ci/pipeline.py <job>`; the driver does the work in Python,
prints each step as a collapsible section of the job log, and on a failure
ends with `pipeline: <job> FAILED at "<step>" (exit N)` and a line saying
what to look at first. Everything a job hands back is under `_local/ci/`
in its artifacts (kept two weeks, also for failed jobs): `logs/` (the
buildutil logs), `reports/`, `smoke/`, `web/`, `test-output/`, `perf/`.

| Job | Stage | Runner | What it does |
|---|---|---|---|
| `census` | check | docker | `tools/census/census.py` (the table of HEAD in `reports/census.md`) and the census gate's pytest suite; no build |
| `linux-build` | build | docker | `./buildutil build --release` with gcc 14, every compiler warning and error counted per flag and module (`reports/compiler-diagnostics.txt`), then the three smokes (`smoke/<name>/`: `final.png`, WAV, OPL log, `port.log`) with `ok`/`DIFF` against the goldens |
| `web` | build | docker | the Emscripten lane: emsdk 6.0.10, `./buildutil build --compiler emscripten --release --no-tests`, no `.data` file installed, the smoke under node (`web/`: `opeu.{html,js,wasm}`, `sizes.txt`, `web-smoke.{png,wav,opl,sha1}`) |
| `linux-test` | test | docker | `./buildutil test`: the GoogleTest suites, the pytest suites with the functional suite, the perf gates and the `tools/*/pytest.ini` suites (the census gate among them); on a failure, ctest's logs and the failing runs' directories in `test-output/` |
| `macos` | test | macOS ARM shell | `./buildutil build --release` and `./buildutil test` natively; manual and allowed to fail while that runner is offline |
| `perf-baseline` | test | docker | manual: records the docker runner's entry in the perf baseline (`perf/baseline.json`, to commit as `sources/opeu/integration/fixtures/perf/baseline.json`) |

The Linux jobs run in `ubuntu:24.04` on the docker runner (tag
`docker-linux`), the same image and the same package lists as the
container lanes above (`tools/ci/buildutil_lane.py`, `tools/ci/web_lane.py`);
their only shell line installs `python3`. The driver installs the packages
(archives cached in `_local/ci/apt/`), makes gcc 14 the default, lets
`./buildutil` bootstrap `_pyvenv/` from the vendored `.buildutil/` and
adds pip's cmake to it (Ubuntu's 3.28 is too old, see Prerequisites).
Caches: `_pyvenv/` keyed on `buildutil.toml` and the vendored buildutil's
version, `_conanhome/` keyed on `conanfile.py` and `sources/CMakeLists.txt`
(one for gcc, one for Emscripten), the emsdk in `_local/ci/emsdk/` keyed on
`tools/ci/web_lane.py`. `linux-build` writes the caches, `linux-test`
only reads them.

The `-Werror` warnings gate (Warnings above) applies under gcc as under
clang. When gcc stops a gated module, `linux-build` fails and names the
modules and `reports/compiler-diagnostics.txt`; the fix is in the code, not
in the flags.

The perf gates judge the runner against `fixtures/perf/baseline.json`
(Benchmarks and profiling above). Until the runner has its own entry the
numbers of the closest listed machine apply, measured with another compiler
and usually another architecture, so `linux-test` sets
`OPEU_PERF_BUDGET` to `CI_FOREIGN_PERF_BUDGET` (100 % by default, a
pipeline variable) and the gate catches only gross regressions there. Run
`perf-baseline` on `main` and commit its `baseline.json`; from then on the
runner is a listed machine and the gate's own 10 % applies. An explicit
`OPEU_PERF_BUDGET` variable wins over both.

Variables. The conan remote is the instance's masked CI/CD variables
`CI_ARTIFACTORY_HREF`, `CI_ARTIFACTORY_NAME`, `CI_ARTIFACTORY_USER` and
`CI_ARTIFACTORY_PASS` (not protected, so merge requests get them too),
the names this checkout's `.env` uses, which buildutil reads as the
fallback spellings of `CONAN_REMOTE_*`. With them a job downloads the
binaries other machines uploaded and uploads what it built; without them
(the log's `host` section says which) conan builds SDL's graph from
conancenter sources, which is slow but works. A project that does not
inherit them defines the same four, masked. Optional: `OPEU_PERF_BUDGET`,
`CI_FOREIGN_PERF_BUDGET`.

Runners: `census`, `linux-build`, `linux-test`, `web` and `perf-baseline`
need a Linux docker runner tagged `docker-linux` (network to the Ubuntu
mirror, GitHub for the emsdk, PyPI and the conan remote; a few GB of
disk for `_conanhome/`). `macos` needs a shell runner tagged `macos` and
`macbook-arm-shell` with Python 3, Xcode's command line tools, CMake 3.30
or newer (else pip's goes into `_pyvenv/`) and Ninja.

A job locally: `python3 tools/ci/pipeline.py <job>` on a machine like the
runner (it installs packages with apt, so on a workstation run it in a
throwaway container), or the container lanes above, which build the same
thing from a read-only mount.

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
