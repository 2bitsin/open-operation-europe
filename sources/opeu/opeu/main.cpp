// Operation Europe, native: the application. The entry is opeu_main (entry.cpp, beside this file), crt0's role for the
// recovered MAIN; it finds the game's data files and owns them for the run. An exception that reaches here is said on
// stderr.
#include <exception>
#include <print>

auto opeu_main(int argc, char** argv) -> int; // NOLINT(readability-identifier-naming): entry.cpp's, renamed with #17

namespace {
// An exception that reached main, said on stderr; the exit code.
auto Failed(std::exception const& failure) noexcept -> int {
  try {
    std::println(stderr, "opeu: {}", failure.what());
    return 1;
  } catch (...) {
    return 1; // nothing is left to report to
  }
}
}

auto main(int argc, char** argv) -> int {
  try {
    return opeu_main(argc, argv);
  } catch (std::exception const& failure) {
    return Failed(failure);
  }
}
