"""The services' C with the old port's flags; port.h and services.h live in the glue."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
roots = ["services", "glue/include", "platform"]
flags = ["-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(bc.source_dir().rglob("*.c")):
  bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
