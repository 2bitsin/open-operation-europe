"""The extractor under the warnings gate (docs/building.md "Warnings"); png.cpp also compiles stb_image_write's
implementation, static so it cannot meet the platform layer's copy."""
import buildutil_configure as bc

WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
STB = ["-DSTB_IMAGE_WRITE_IMPLEMENTATION", "-DSTB_IMAGE_WRITE_STATIC"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  bc.declare(source, options=WARNINGS + (STB if source.name == "png.cpp" else []))
