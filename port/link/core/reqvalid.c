/* Link: core/reqvalid.c (1270:036e request validation). The same callback
 * is handed to opeu_arrival_locate (map/arrplace.c, 1270:000e), whose only
 * service is id 0 with the same meaning.
 * Services (host branch macros, word arguments a,b,c,d):
 * 0 request_cells(side, predicate offset, predicate segment) 145c:1208
 *   (predicates 145c:1114 request_cell_allowed, 145c:1152
 *   arrival_cell_allowed), 1 daily_show(id, side, present offset, present
 *   segment) 1707:03e4. */
#include "c2_common.h"

uint16_t link_reqvalid(uint8_t *ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    uint16_t r = 0;
    switch (id) {
    case 0: r = opeu_request_cells(ds, (uint8_t)a, c2_cell_predicate(b, c)); break; /* 145c:1208 */
    case 1:                                                           /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c);
        break;
    default: port_unhandled("reqvalid", id, 4);
    }
    return r;
}
