/* The fixture's link table. */
#include "port.hpp"

PortHandler port_routine_opeu_grid_step;
PortHandler port_routine_opeu_set_order;
PortHandler port_routine_opeu_ask;
PortHandler port_routine_opeu_mark_cell;

const PortLink port_links[] = {
  { 0x01000u, "opeu_grid_step",  port_routine_opeu_grid_step },
  { 0x01010u, "opeu_set_order",  port_routine_opeu_set_order },
  { 0x01030u, "opeu_unlinked",   nullptr                     },
  { 0x01060u, "opeu_ask",        port_routine_opeu_ask       },
  { 0x01070u, "opeu_mark_cell",  port_routine_opeu_mark_cell },
  { 0x02000u, "opeu_action_one", nullptr                     },
};
