/* Link: units/ordunit.c (1c23:01fa..02ee unit order helpers). The only
 * service is 2 confirm_message(text,person) (1707:076c); the module's
 * callers (ordissue, ordact) pass their own order event, so their link
 * files serve it too. link_ordunit serves a direct use of this module. */
#include "units_F.h"

uint16_t link_ordunit(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 2:
        lf_order_service(ds, id, args, count, &r);
        break;
    default: port_unhandled("ordunit", id, count);
    }
    return r;
}
