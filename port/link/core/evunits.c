/* Link: core/evunits.c (11e5:0606 hourly unit condition event). The only
 * service, from the host branch comment: 7 unit message dialog (unit,text). */
#include "c1_common.h"

uint16_t link_evunits(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 7: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;    /* 1707:07ec unit_message */
    default: port_unhandled("evunits", id, count);
    }
    return 0;
}
