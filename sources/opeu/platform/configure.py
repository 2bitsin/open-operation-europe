"""The SDL3 backend (C++23 since phase 2) with the old port's flags; the build identity keeps buildutil's defaults."""
import buildutil_configure as bc

flags = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted((bc.source_dir() / "sdl3").rglob("*.cpp")):
  bc.declare(source, options=flags)
