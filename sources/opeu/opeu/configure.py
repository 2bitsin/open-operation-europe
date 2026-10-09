"""The entry (entry.cpp, the old port/main.c) with the old port's flags and include roots; main.cpp and the assets keep
buildutil's defaults."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# The headers the entry reads unqualified: port.hpp and glue.hpp, the services,
# the platform layer, the recovered game's core/app.hpp and the opening's intro.hpp.
roots = ["glue/include", "services", "platform", "game/common", "game", "intro"]
flags = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
bc.declare(bc.source_dir() / "entry.cpp", options=flags + [f"-I{opeu / root}" for root in roots])
