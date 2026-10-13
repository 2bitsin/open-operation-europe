/* The fixture's link table. */
#include "link/link.hpp"

PortHandler port_routine_opeu_step;
PortHandler port_routine_opeu_mark;
PortHandler port_routine_opeu_show;

const PortLink port_links[] = {
  { 0x01000u, "opeu_step", port_routine_opeu_step },
  { 0x01010u, "opeu_mark", port_routine_opeu_mark },
  { 0x01020u, "opeu_show", port_routine_opeu_show },
};
