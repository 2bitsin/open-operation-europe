"""The SDL3 backend (C++23 since phase 2) with the old port's flags; the build identity keeps buildutil's defaults."""
import buildutil_configure as bc

flags = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
# The backend is stb_image_write's only user and compiles its implementation.
bc.declare(bc.source_dir() / "sdl3/platform-sdl3.cpp", options=flags + ["-DSTB_IMAGE_WRITE_IMPLEMENTATION"])
