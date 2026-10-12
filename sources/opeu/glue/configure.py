"""The glue's base: the 16-bit memory model, the diagnostics and the run's options, C++23."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# A finished module compiles under the warnings gate (docs/building.md "Warnings").
WARNINGS = ["-Wall", "-Wextra", "-Wconversion", "-Wsign-conversion", "-Wshadow", "-Werror"]
# port.hpp, the layer contract, is the only header the base reads.
roots = ["glue/include"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  bc.declare(source, options=WARNINGS + [f"-I{opeu / root}" for root in roots])
