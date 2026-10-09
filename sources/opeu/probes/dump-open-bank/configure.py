"""The bank dump names OPEN's symbols through the intro's forced names.hpp, as ofmusic.cpp is compiled."""
import buildutil_configure as bc

intro = bc.source_dir().parents[1] / "intro"
for source in [bc.source_dir() / "dump-open-bank.cpp"]:  # main.cpp keeps buildutil's defaults
  bc.declare(source, options=["-Wall", "-Wextra", f"-include{intro / 'names.hpp'}",
                              f"-I{intro / 'recovered'}"])
