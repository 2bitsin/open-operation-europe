"""OPEN.EXE: its recovered code under recovered/, renamed by a forced names.hpp, and its runner (C++23)."""
import buildutil_configure as bc

intro = bc.source_dir()
opeu = intro.parent
# OPEN and MAIN were separate DOS programs and reuse names: every symbol a
# recovered OPEN module defines is renamed by the forced include. otext.cpp's
# host branch #defines its DGROUP variables over those renames, and the
# module's own macro wins (-Wno-macro-redefined, clang's spelling; gcc
# ignores an unknown -Wno-).
recovered = ["-Wall", "-Wextra", "-fno-strict-aliasing", "-Wno-macro-redefined",
             f"-include{intro / 'names.hpp'}", f"-I{intro / 'recovered'}"]
for source in sorted((intro / "recovered").rglob("*.cpp")):
  bc.declare(source, options=recovered)
# The runner, its link layer and image: the old executable's include roots.
roots = ["glue/include", "services", "platform", "game/common", "game", "intro"]
runner = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(intro.rglob("*.cpp")):
  if "recovered" not in source.relative_to(intro).parts:
    bc.declare(source, options=runner + [f"-I{opeu / root}" for root in roots])
