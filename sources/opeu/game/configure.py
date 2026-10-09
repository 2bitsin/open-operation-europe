"""The recovered MAIN game as C++23 with the flags the generator checked it with (minus -Werror and -std=c99)."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# common/ (types.hpp, records.hpp) before the module's own areas, then the glue's
# headers (port.hpp, services.hpp, glue.hpp) and the services' (vgalib.hpp).
roots = ["game/common", "game", "glue/include", "services"]
# The services are direct calls without OPEU_HOST_EVENTS, so the `event`
# parameters are unused.
# -Wno-narrowing: the service tables build their argument words with brace
# initialisers (`uint16_t args[] = {x, y}`) from int expressions; C converts
# them implicitly and C++ refuses the narrowing. The flag keeps C's conversion
# (the same value) without a cast at each of the ~270 sites.
flags = ["-fno-strict-aliasing", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-narrowing"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
