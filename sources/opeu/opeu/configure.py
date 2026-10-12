"""The entry (entry.cpp) with its include roots; main.cpp, the assets and their test. All under the warnings gate."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# A finished module compiles under the warnings gate (docs/building.md "Warnings").
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
# The headers the entry reads unqualified: port.hpp and glue.hpp, the services,
# the platform layer, the recovered game's core/app.hpp and the opening's intro.hpp.
roots = ["glue/include", "services", "platform", "game/common", "game", "intro"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  includes = [f"-I{opeu / root}" for root in roots] if source.name == "entry.cpp" else []
  bc.declare(source, options=WARNINGS + includes)
