/* The fixture's link table. */
#include "port.hpp"

PortHandler port_routine_opeu_grid_step;
PortHandler port_routine_opeu_set_order;

const PortLink port_links[] = {
  { 0x01000u, "opeu_grid_step",    port_routine_opeu_grid_step },
  { 0x01010u, "opeu_set_order",    port_routine_opeu_set_order },
  { 0x01030u, "opeu_unlinked",     nullptr                     },
  { 0x02000u, "opeu_action_one",   nullptr                     },
};
