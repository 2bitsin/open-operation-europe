// Diagnostic: writes OPEN.EXE's FM music bank to stdout (dump-open-bank.cpp).
#pragma once

namespace opeu::probes::detail::dump_open_bank {
auto DumpOpenBank() -> int;
}

namespace opeu::probes {
using detail::dump_open_bank::DumpOpenBank;
}
