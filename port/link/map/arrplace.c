/* Link: map/arrplace.c (1270:000e pick an arrival cell from the candidate
 * grid). Event (host branch COUNT):
 *   0 request_cells(side, predicate offset, predicate segment)
 *        145c:1208 opeu_request_cells (REQCELLS); ARRPLACE passes the far
 *        predicate 145c:1152 opeu_arrival_cell_allowed.
 * REQCELLS' host entry takes the predicate as a host function of exactly
 * the recovered predicates' type; the far pointer is resolved by address
 * (the two CELLPRED predicates of segment 145c). */
#include "map_common.h"

uint16_t link_arrplace(uint8_t *ds, uint16_t id, uint16_t side, uint16_t off, uint16_t seg, uint16_t unused)
{
    (void)unused;
    switch (id) {
    case 0:                                                         /* 145c:1208 */
        if (seg == 0x145c && off == 0x1152)
            return opeu_request_cells(ds, (uint8_t)side, opeu_arrival_cell_allowed);
        if (seg == 0x145c && off == 0x1114)
            return opeu_request_cells(ds, (uint8_t)side, opeu_request_cell_allowed);
        port_fatal("arrplace: request_cells predicate %04x:%04x is not a recovered CELLPRED predicate", seg, off);
        break;
    default: port_unhandled("arrplace", id, 0);
    }
    return 0;
}
