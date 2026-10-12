"""The SDL3 backend and the build identity, C++23."""
import buildutil_configure as bc

# A finished module compiles under the warnings gate (docs/building.md "Warnings").
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  # The backend is stb_image_write's only user and compiles its implementation.
  stb = ["-DSTB_IMAGE_WRITE_IMPLEMENTATION"] if source.name == "platform-sdl3.cpp" else []
  bc.declare(source, options=WARNINGS + stb)
