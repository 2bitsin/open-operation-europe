/* Link: units/ordattk.c (3131:000c..0508 occupy and attack order handlers).
 * Services 0..26, 31 pending_finish and 32 wait_units in the numbering of
 * tests/validate_order_handlers.py (see ordhand_F.h). */
#include "ordhand_F.h"

uint16_t link_ordattk(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    if ((id > 26 && id != 31 && id != 32) || !lf_order_handler_service(ds, id, args, count, &r))
        port_unhandled("ordattk", id, count);
    return r;
}
