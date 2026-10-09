"""The trace runner reads fmdriver.h from the services."""
import buildutil_configure as bc

services = bc.source_dir().parents[1] / "services"
for source in sorted(bc.source_dir().rglob("*.c")):
  bc.declare(source, options=["-std=c99", "-Wall", "-Wextra", f"-I{services}"])
