#!/bin/sh
# Compile every recovered host branch to an object for the symbol inventory.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$ROOT/port/build/hostobj}
mkdir -p "$OUT"
fail=0
for f in "$ROOT"/src/main/core/*.c "$ROOT"/src/main/data/*.c "$ROOT"/src/main/map/*.c "$ROOT"/src/main/units/*.c "$ROOT"/src/main/io/*.c; do
  n=$(basename "$(dirname "$f")")_$(basename "$f" .c)
  if ! cc -std=c99 -Wall -Wextra -Werror -I "$ROOT/src/common" -I "$ROOT/src/main/include" -c "$f" -o "$OUT/$n.o" 2>"$OUT/$n.err"; then
    fail=$((fail+1)); echo "FAIL $f"
  fi
done
echo "failures: $fail"
