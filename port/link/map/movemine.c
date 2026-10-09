/* Link: map/movemine.c (32e6:05a8 minefield entry, 0686 element clamp, 06ba
 * movement rating). Events (host branch comment; callees from the DOS
 * branch):
 *   0 category_6(code)                    0b73:004a
 *   1 unit_message(unit,text)             1707:07ec opeu_unit_report (DIALOGS)
 *   2 selection_eligible(unit)            145c:0fc4
 *   3 unit_special(unit)                  145c:04de
 *   4 daily_show(0x48,side,05a0,32e6)     1707:03e4 (DIALOGS); the far
 *     presenter 32e6:05a0 move_mines_finish is called back by identifier
 *     (port/link/routines/movespot.c)
 *   5 unit_casualties(unit,100,3,15,0,1)  17c4:0b1c (CASUALTY)
 *   6 mark_cell(x,y,side)                 145c:0e76
 *   7 unit_erase(unit)                    15c4:0fe4 opeu_unit_remove_draw (MAPERASE)
 *   8..10 unit_average9/10/11(unit)       145c:04fa / 052e / 0562
 *  11 isqrt(value)                        0000:3fce opeu_isqrt */
#include "map_common.h"

uint16_t link_movemine(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: r = opeu_category_6((uint8_t)ARG(0)); break;            /* 0b73:004a */
    case 1: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:07ec */
    case 2: r = opeu_unit_selection_eligible(ds, ARG(0)); break;    /* 145c:0fc4 */
    case 3: r = opeu_unit_special(ds, ARG(0)); break;               /* 145c:04de */
    case 4: map_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break; /* 1707:03e4 */
    case 5:                                                         /* 17c4:0b1c */
        frame = map_frame_enter();
        opeu_unit_casualties(ds, link_casualty, frame, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        map_frame_leave();
        break;
    case 6: opeu_mark_cell(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break; /* 145c:0e76 */
    case 7:                                                         /* 15c4:0fe4 */
        frame = map_frame_enter();
        opeu_unit_remove_draw(ds, link_maperase, frame, ARG(0));
        map_frame_leave();
        break;
    case 8: r = opeu_unit_average9(ds, ARG(0)); break;              /* 145c:04fa */
    case 9: r = opeu_unit_average10(ds, ARG(0)); break;             /* 145c:052e */
    case 10: r = opeu_unit_average11(ds, ARG(0)); break;            /* 145c:0562 */
    case 11: r = opeu_isqrt(ARG(0)); break;                         /* 0000:3fce */
    default: port_unhandled("movemine", id, count);
    }
    return r;
}
