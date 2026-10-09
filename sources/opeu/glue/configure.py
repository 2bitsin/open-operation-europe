"""The glue's base: the 16-bit memory model, the diagnostics and the run's options, C++23, with the old port's flags."""
import buildutil_configure as bc

opeu = bc.source_dir().parent
# port.hpp, the layer contract, is the only header the base reads.
roots = ["glue/include"]
flags = ["-Wall", "-Wextra", "-Wno-unused-parameter"]
for source in sorted(bc.source_dir().rglob("*.cpp")):
  bc.declare(source, options=flags + [f"-I{opeu / root}" for root in roots])
