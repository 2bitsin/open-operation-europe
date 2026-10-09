"""The glue's C (the old port/main.c, core/ and gen/) with the old port's flags and include roots."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# The headers the glue reads unqualified: its own include/, the services, the
# platform layer, the recovered game (common/ first, then its areas) and intro.
roots = ["glue/include", "services", "platform", "game/common", "game", "intro"]
flags = ["-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(bc.source_dir().rglob("*.c")):
  bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
