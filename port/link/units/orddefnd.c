/* Link: units/orddefnd.c (3131:0508..0596 defend order handler). Services
 * of tests/validate_order_handlers.py used here: 0 unit_average11,
 * 16 move_level_lower, 24 unit_person, 25 unit_average10,
 * 27 unit_order_destination (see ordhand_F.h). */
#include "ordhand_F.h"

uint16_t link_orddefnd(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: case 16: case 24: case 25: case 27:
        lf_order_handler_service(ds, id, args, count, &r);
        break;
    default: port_unhandled("orddefnd", id, count);
    }
    return r;
}
