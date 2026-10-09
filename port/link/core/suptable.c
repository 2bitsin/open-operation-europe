/* Link: core/suptable.c (20b3:0d40..10b7 supply-allocation table views).
 * Services, from the host branch:
 *  0 column layout(column, row) -> 0a8e:0140 opeu_table_columns; the three
 *    byte outputs the original writes into unused frame bytes go to a
 *    modeled stack area (the host discards them, as the original does)
 *  1 position(x, y)               0480:0808 army_position
 *  2 print(format, arguments...)  0480:0bb8 army_print (cdecl)
 *  3 highlight(x, y, w, h, color) VGALIB 2B0E opeu_ui_highlight */
#include "b2_common.h"

void link_suptable(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: {
        uint16_t out = (uint16_t)(port_stack_enter(4) - 4);
        opeu_table_columns(ds, link_listui, (uint8_t)ARG(0), (uint8_t)ARG(1), out, (uint16_t)(out + 1),
                           (uint16_t)(out + 2));
        port_stack_leave(4);
        break;
    }
    case 1: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 2: b2_cursor_print(ds, args, count); break;
    case 3: opeu_ui_highlight(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    default: port_unhandled("suptable", id, count);
    }
}
