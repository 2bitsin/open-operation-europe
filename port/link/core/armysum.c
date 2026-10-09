/* Link: core/armysum.c (0bc3:0042 opeu_army_summary). Services, from the
 * host branch: 0 army_title(text) 1707:03aa, 1 army_text(text) = ui_text
 * 0480:0484, 2 request_format(dest, format, ...) 0480:0b9a (raised by
 * units/unitname.c 0b73:0188, which the summary calls with this callback),
 * 3 army_print(format, ...) = cursor_print 0480:0bb8 (cdecl words as the
 * format converts them), 4 army_position(x, y) 0480:0808. */
#include "b2_common.h"

uint16_t link_armysum(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;
    case 1: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 2: b2_request_format(ds, args, count); break;
    case 3: b2_cursor_print(ds, args, count); break;
    case 4: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    default: port_unhandled("armysum", id, count);
    }
    return 0;
}
