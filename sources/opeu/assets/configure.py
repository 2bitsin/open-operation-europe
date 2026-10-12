"""The decoders and the pack loader, C++23, under the warnings gate of a finished module (docs/building.md
"Warnings"); png-reader.cpp also compiles stb_image's implementation, PNG only and static, so it cannot meet another
module's copy."""
import buildutil_configure as bc

WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
STB = ["-DSTB_IMAGE_IMPLEMENTATION", "-DSTB_IMAGE_STATIC", "-DSTBI_ONLY_PNG"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  bc.declare(source, options=WARNINGS + (STB if source.name == "png-reader.cpp" else []))
