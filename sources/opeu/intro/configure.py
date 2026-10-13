"""OPEN.EXE: its recovered code under recovered/ in namespace opeu::intro, and its runner (C++23)."""
import buildutil_configure as bc

intro = bc.source_dir()
opeu = intro.parent
# A finished module compiles under the warnings gate (docs/building.md "Warnings").
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
# The include roots of the services and the glue the opening calls.
ROOTS = [f"-I{opeu / root}" for root in ["glue/include", "services", "platform", "intro"]]
for source in sorted(intro.rglob("*.cpp")):
  recovered = "recovered" in source.relative_to(intro).parts
  bc.declare(source, options=WARNINGS + ROOTS + (["-fno-strict-aliasing"] if recovered else []))
