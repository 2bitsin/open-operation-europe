// Diagnostic: writes OPEN.EXE's FM music bank to stdout (dump-open-bank.cpp).
#include "dump-open-bank.hpp"

auto main() -> int {
  return opeu::probes::DumpOpenBank();
}
