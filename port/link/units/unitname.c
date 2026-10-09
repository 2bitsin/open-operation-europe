/* Link: units/unitname.c (0b73:0188 unit name). The only service is the
 * one opeu_unit_name raises through the event it receives:
 * 2 request_format(buffer, 0x537c, entry name, number) (0480:0b9a),
 * numbered as kind 2 of the TableEvent protocol. */
#include "units_F.h"

uint16_t link_unitname(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 2: r = lf_request_format(ds, args, count); break;
    default: port_unhandled("unitname", id, count);
    }
    return r;
}
