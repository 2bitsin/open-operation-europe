/* The run's options (set by the entry from the command line, read by the
 * services, the pump and the opening) and the program's exit. */
#include "port.hpp"

#include <cstdlib>

PortOptions port_options;

void port_exit(int code) {
  port_trace("svc", "exit(%d)", code);
  exit(code == 100 ? 0 : code);
}
