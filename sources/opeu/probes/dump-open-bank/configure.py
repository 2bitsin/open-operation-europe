"""The bank dump names OPEN's symbols through the intro's forced open_names.h, as ofmusic.c is compiled."""
import buildutil_configure as bc

intro = bc.source_dir().parents[1] / "intro"
for source in sorted(bc.source_dir().rglob("*.c")):
  bc.declare(source, options=["-Wall", "-Wextra", f"-include{intro / 'open_names.h'}",
                              f"-I{intro / 'recovered'}"])
