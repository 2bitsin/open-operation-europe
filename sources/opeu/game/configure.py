"""The recovered MAIN game with the flags the generator checked it with (minus -Werror)."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# common/ (types.h, records.h) before the module's own areas, then the glue's
# headers (port.h, services.h, glue.h) and the services' (vgalib.h).
roots = ["game/common", "game", "glue/include", "services"]
# The services are direct calls without OPEU_HOST_EVENTS, so the `event`
# parameters are unused.
flags = ["-std=c99", "-fno-strict-aliasing", "-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(bc.source_dir().rglob("*.c")):
  bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
