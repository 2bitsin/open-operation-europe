/* Link: units/labels.c (0b73:01de..0287 type/item/element names and
 * location labels). The only service is the one opeu_location_name raises
 * through the event it receives: 2 request_format(buffer, 0x538a, x*scale,
 * y*scale) (0480:0b9a), numbered as kind 2 of the TableEvent protocol. */
#include "units_F.h"

uint16_t link_labels(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 2: r = lf_request_format(ds, args, count); break;
    default: port_unhandled("labels", id, count);
    }
    return r;
}
