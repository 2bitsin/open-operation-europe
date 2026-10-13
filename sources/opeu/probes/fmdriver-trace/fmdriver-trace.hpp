// Diagnostic: the native FM driver's trace runner. Reads a command script on stdin and prints every OPL register write
// and call result (fmdriver-trace.cpp).
#pragma once

namespace opeu::probes::detail::fmdriver_trace {
// 0 when the script ran, 2 when it is malformed.
auto FmdriverTrace() -> int;
}

namespace opeu::probes {
using detail::fmdriver_trace::FmdriverTrace;
}
