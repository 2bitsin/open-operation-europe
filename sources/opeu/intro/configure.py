"""OPEN.EXE: its recovered code under recovered/ in namespace opeu::intro, and its runner (C++23)."""
import buildutil_configure as bc

intro = bc.source_dir()
opeu = intro.parent
# A finished module compiles under the warnings gate (docs/building.md "Warnings").
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
for source in sorted((intro / "recovered").rglob("*.cpp")):
  bc.declare(source, options=WARNINGS + ["-fno-strict-aliasing"])
# The runner, its link layer and image: the old executable's include roots.
roots = ["glue/include", "services", "platform", "game/common", "game", "intro"]
for source in sorted(intro.rglob("*.cpp")):
  if "recovered" not in source.relative_to(intro).parts:
    bc.declare(source, options=WARNINGS + [f"-I{opeu / root}" for root in roots])
