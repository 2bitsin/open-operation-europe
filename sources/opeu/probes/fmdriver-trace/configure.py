"""The trace runner reads fmdriver.hpp from the services."""
import buildutil_configure as bc

services = bc.source_dir().parents[1] / "services"
for source in [bc.source_dir() / "fmdriver-trace.cpp"]:  # main.cpp keeps buildutil's defaults
  bc.declare(source, options=["-Wall", "-Wextra", f"-I{services}"])
