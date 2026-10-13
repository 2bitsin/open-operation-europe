// Diagnostic: the native FM driver's trace runner (fmdriver-trace.cpp). Reads a
// command script on stdin and prints every OPL register write and call result.
auto FmdriverTrace() -> int;

auto main() -> int {
  return FmdriverTrace();
}
