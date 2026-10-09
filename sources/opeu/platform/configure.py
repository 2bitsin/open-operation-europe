"""The SDL3 backend's C with the old port's flags."""
import buildutil_configure as bc

flags = ["-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(bc.source_dir().rglob("*.c")):
  bc.declare(source, options=flags)
