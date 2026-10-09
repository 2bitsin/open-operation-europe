"""OPEN.EXE: its recovered C under recovered/, renamed by a forced open_names.h, and its runner."""
import buildutil_configure as bc

intro = bc.source_dir()
opeu = intro.parent
# OPEN and MAIN were separate DOS programs and reuse names: every symbol a
# recovered OPEN module defines is renamed by the forced include. otext.c's
# host branch #defines its DGROUP variables over those renames, and the
# module's own macro wins (-Wno-macro-redefined, clang's spelling; gcc
# ignores an unknown -Wno-).
recovered = ["-Wall", "-Wextra", "-fno-strict-aliasing", "-Wno-macro-redefined",
             f"-include{intro / 'open_names.h'}", f"-I{intro / 'recovered'}"]
for source in sorted((intro / "recovered").rglob("*.c")):
  bc.declare(source, options=recovered)
# The runner, its link layer and image: the old executable's include roots.
roots = ["glue/include", "services", "platform", "game/common", "game", "intro"]
runner = ["-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(intro.rglob("*.c")):
  if "recovered" not in source.relative_to(intro).parts:
    bc.declare(source, options=runner + [f"-I{opeu / root}" for root in roots])
