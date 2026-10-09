/* Link: core/supstep.c (20b3:05b8 opeu_supply_step, interactive supply
 * allocation). Services, from the host branch:
 *  1..7 the SUPPANEL input services raised by opeu_supply_input (20b3:0256),
 *    which the step calls with this callback: 1 mouse show 05a8:003e,
 *    2 delay ticks(t) 0a8e:000c, 3 mouse position (x, y returned in
 *    args[0], args[1]) 05a8:0086, 4 right down 05a8:012e, 5 left down
 *    05a8:0104, 6 mouse hide 05a8:0062, 7 keyboard key 0541:00ba
 * 20 row(row, column)       20b3:0452 opeu_supply_draw_row
 * 21 chief_text(text)       20b3:007a opeu_ops_chief_text
 * 22 input reset()          05a8:01b4 opeu_input_reset
 * 23 highlight(x,y,w,h,c)   VGALIB 2B0E opeu_ui_highlight
 * 24 chief_wait(text) -> AX 20b3:0106 opeu_ops_chief_wait */
#include "b2_common.h"

uint16_t link_supstep(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        r = link_suppanel(ds, id, args, count);   /* the same SUPPANEL services */
        break;
    case 20: opeu_supply_draw_row(ds, link_suprow, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 21: opeu_ops_chief_text(ds, link_chief, ARG(0)); break;
    case 22: opeu_input_reset(ds, link_mouse); break;
    case 23: opeu_ui_highlight(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 24: r = opeu_ops_chief_wait(ds, link_chief, ARG(0)); break;
    default: port_unhandled("supstep", id, count);
    }
    return r;
}
