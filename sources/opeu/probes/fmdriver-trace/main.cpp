// Diagnostic: the native FM driver's trace runner (fmdriver-trace.cpp). Reads a
// command script on stdin and prints every OPL register write and call result.
auto opeu_fmdriver_trace() -> int;

auto main() -> int {
  return opeu_fmdriver_trace();
}
