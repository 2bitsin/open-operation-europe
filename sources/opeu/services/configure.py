"""The services and the event pump, C++23 since phase 2, with the old port's flags; port.hpp and services.hpp live in the glue."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
roots = ["services", "glue/include", "platform"]
flags = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
# The files that were C++ before phase 2 (the ymfm bridge, the GoogleTest suites) keep buildutil's defaults; the
# drawing suites take the services' flags and include roots.
cxx = {"opl-ymfm.cpp", "file-layer.cpp"}
for source in sorted(s for s in bc.source_dir().rglob("*.cpp") if s.name not in cxx):
  bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
