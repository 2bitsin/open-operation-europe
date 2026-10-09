# Operation Europe, native

*Operation Europe: Path to Victory 1939-45* (KOEI, 1994) for DOS, recovered into C and C++ source and running natively
on SDL3 on macOS, Linux and in the browser. There is no emulation: no CPU emulator, no DOS runtime, no recompiled
machine code. The game is compiled from its recovered, human-readable source and plays from the original data files.

## Legal note

This is an unofficial project, not affiliated with or endorsed by KOEI or Koei Tecmo; *Operation Europe* is their
trademark and copyright. No game files are included or distributed here: you need your own copy of the 1994 DOS
release, whose data files the game reads at run time. The recovered source does carry tables taken from the original
program (unit and commander names, message texts, scenario tables, the music and sound banks, the font glyphs), because
the game logic cannot be expressed without them.

## Build and run

```
./buildutil install                                  # the project's venv and build tool
./buildutil build                                    # conan fetches SDL3 and the rest; cmake and ninja build
./buildutil run -- --data <your game directory>      # the directory holding the DOS release's files
./buildutil run -- --data <your game directory> --check-data   # list what is missing
./buildutil test                                     # the test suites
```

You need Python 3, a C++23 compiler (Apple Clang, Clang or GCC 14) and git; the details, options and platforms are in
[docs/building.md](docs/building.md).

In the browser: on Linux with the Emscripten SDK, `./buildutil build --compiler emscripten --release --no-tests` builds
the page into `_install/opeu/`.

## Licence

The project's own code is under the MIT licence ([LICENSE](LICENSE)). `sources/opeu/ymfm/` is ymfm by Aaron Giles,
under the BSD-3-Clause licence.
