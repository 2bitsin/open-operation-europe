# Source this before emcmake when the emscripten install is not configured:
#
#   . port/tools/web_env.sh
#   emcmake cmake -S port -B port/build/web -DCMAKE_BUILD_TYPE=Release
#   cmake --build port/build/web -j8
#
# It only sets environment variables for the current shell:
#   EMSDK_PYTHON  a Python >= 3.10 (emscripten 6 refuses older ones; macOS'
#                 /usr/bin/python3 is 3.9 and may come first in PATH)
#   EM_CONFIG     port/build/emscripten.config, written here when the
#                 install has no config of its own (Homebrew's emscripten
#                 ships LLVM and Binaryen under libexec but no .emscripten)
#   EM_CACHE      port/build/emcache, so the system libraries and the SDL3
#                 port are built inside the checkout, not in the install
# Existing values of these variables are kept.

_opeu_root=$(cd "$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")/../.." && pwd)

if [ -z "$EMSDK_PYTHON" ]; then
  for _p in python3.14 python3.13 python3.12 python3.11 python3.10 python3; do
    _c=$(command -v "$_p" 2>/dev/null) || continue
    if "$_c" -c 'import sys; sys.exit(sys.version_info < (3, 10))' 2>/dev/null; then
      export EMSDK_PYTHON="$_c"
      break
    fi
  done
fi

if [ -z "$EM_CONFIG" ]; then
  _emcc=$(command -v emcc 2>/dev/null)
  if [ -n "$_emcc" ]; then
    # Homebrew's emcc is a wrapper script; the real tree is libexec/.
    _lib=$(sed -n 's|.*exec "\([^"]*\)/emcc".*|\1|p' "$_emcc" | head -n 1)
    [ -n "$_lib" ] || _lib=$(dirname "$(readlink -f "$_emcc")")
    if [ ! -f "$_lib/.emscripten" ] && [ -d "$_lib/llvm/bin" ] && [ -d "$_lib/binaryen" ]; then
      mkdir -p "$_opeu_root/port/build"
      {
        echo "LLVM_ROOT = '$_lib/llvm/bin'"
        echo "BINARYEN_ROOT = '$_lib/binaryen'"
        echo "NODE_JS = '$(command -v node)'"
      } > "$_opeu_root/port/build/emscripten.config"
      export EM_CONFIG="$_opeu_root/port/build/emscripten.config"
    fi
  fi
fi

[ -n "$EM_CACHE" ] || export EM_CACHE="$_opeu_root/port/build/emcache"

echo "web_env: EMSDK_PYTHON=${EMSDK_PYTHON:-<unset>} EM_CONFIG=${EM_CONFIG:-<install default>} EM_CACHE=$EM_CACHE"
unset _opeu_root _p _c _emcc _lib
