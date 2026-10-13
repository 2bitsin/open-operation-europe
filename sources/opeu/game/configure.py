"""The recovered MAIN game as C++23 with the flags the generator checked it with (minus -Werror and -std=c99), and its
link layer (link/: the data image, the host structs and the service adapters) with the old port's flags."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# common/ (data-segment.hpp and the other shared headers) before the module's own areas, then the glue's
# headers (port.hpp, services.hpp, glue.hpp) and the services' (vgalib.hpp).
roots = ["game/common", "game", "glue/include", "services"]
# The services are direct calls without OPEU_HOST_EVENTS, so the `event`
# parameters are unused.
# -Wno-narrowing: the service tables build their argument words with brace
# initialisers (`uint16_t args[] = {x, y}`) from int expressions; C converts
# them implicitly and C++ refuses the narrowing. The flag keeps C's conversion
# (the same value) without a cast at each of the ~270 sites.
flags = ["-fno-strict-aliasing", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-narrowing"]
game = bc.source_dir()
for source in sorted(game.rglob("*.cpp")):
  if source.relative_to(game).parts[0] != "link":
    bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
# link/ (the old glue core/ and gen/): it names the game's data by symbol and
# holds the host structs and service adapters the game calls, so it lives in
# the game's archive. The glue's flags and include roots.
link_roots = ["glue/include", "services", "game/common", "game"]
link_flags = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted((game / "link").rglob("*.cpp")):
  bc.declare(source, options=link_flags + [f"-I{opeu / root}" for root in link_roots])
