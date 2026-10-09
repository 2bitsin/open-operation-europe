/* Link: map/movespot.c (32e6:0368 spotting, 051e enemy adjacent, 05a0
 * mines finish). Events (host branch comment; callees from the DOS branch):
 *   3 opposite_side(side)          0b73:04c8
 *   4 code_outside_5_to_19(hour)   0b73:0178
 *   5 rand_bounded(n)              09f6:00d4 (state in DS:c6f4)
 *   6 unit_special(unit)           145c:04de
 *   7 unit_selection_eligible(unit) 145c:0fc4
 *   8 grid_distance(x,y,x2,y2)     17c4:1424
 *   9 grid_step(x*,y*,dir)         145c:0d3e (DS offsets of the bytes)
 *  10 cell_side(x,y)               145c:0e42
 *  11 find_unit_at(x,y)            145c:011c
 *  12 pending_finish(value)        0a8e:005c (WAITKEY) */
#include "map_common.h"

uint16_t link_movespot(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 3: r = opeu_opposite_side((uint8_t)ARG(0)); break;         /* 0b73:04c8 */
    case 4: r = opeu_code_outside_5_to_19((uint8_t)ARG(0)); break;  /* 0b73:0178 */
    case 5: r = map_rand_bounded(ds, ARG(0)); break;                /* 09f6:00d4 */
    case 6: r = opeu_unit_special(ds, ARG(0)); break;               /* 145c:04de */
    case 7: r = opeu_unit_selection_eligible(ds, ARG(0)); break;    /* 145c:0fc4 */
    case 8: r = opeu_grid_distance((uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3)); break; /* 17c4:1424 */
    case 9: r = opeu_grid_step(PORT_DS(ARG(0)), PORT_DS(ARG(1)), (uint8_t)ARG(2)); break; /* 145c:0d3e */
    case 10: r = opeu_cell_side(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break; /* 145c:0e42 */
    case 11: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break; /* 145c:011c */
    case 12: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break; /* 0a8e:005c */
    default: port_unhandled("movespot", id, count);
    }
    return r;
}
