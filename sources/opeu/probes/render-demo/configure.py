"""The render demo under the warnings gate (docs/building.md "Warnings")."""
import buildutil_configure as bc

WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  bc.declare(source, options=WARNINGS)
