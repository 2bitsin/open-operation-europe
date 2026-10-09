/* Link: units/tally.c (20b3:10b8..15b7 allocation tallies). The module's
 * only service use is opeu_unit_name (unitname, 0b73:0188), which receives
 * this event and raises 2 request_format(buffer,format,name,number)
 * 0480:0b9a through it. */
#include "units_F.h"

uint16_t link_tally(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 2: r = lf_request_format(ds, args, count); break;
    default: port_unhandled("tally", id, count);
    }
    return r;
}
