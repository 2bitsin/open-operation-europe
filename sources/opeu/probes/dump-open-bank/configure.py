"""The bank dump reads OPEN's song bank, recovered/data/ofmusic.cpp of the intro."""
import buildutil_configure as bc

intro = bc.source_dir().parents[1] / "intro"
for source in [bc.source_dir() / "dump-open-bank.cpp"]:  # main.cpp keeps buildutil's defaults
  bc.declare(source, options=["-Wall", "-Wextra", f"-I{intro}"])
