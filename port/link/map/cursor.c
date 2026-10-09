/* Link: map/cursor.c (17c4:0164 map cursor status line). Events (host
 * branch comment):
 *   0 begin          1707:020c opeu_turn_active_begin (DIALOGS)
 *   1 text(ptr)      0480:0484 cursor_text = opeu_ui_text (TEXTOUT)
 *   2 UNITNAME's formatter (buffer, format, name, number), raised by the
 *     recovered opeu_unit_name that CURSOR runs with this event:
 *                    0480:0b9a request_format (sprintf)
 *   3 print(format, two vararg words)
 *                    0480:0bb8 cursor_print (printf) */
#include "map_common.h"

uint16_t link_cursor(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_turn_active_begin(ds, link_dialogs_host()); break;     /* 1707:020c */
    case 1: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;       /* 0480:0484 */
    case 2: map_request_format(ds, args, count); break;                 /* 0480:0b9a */
    case 3: map_cursor_print(ds, args, count); break;                   /* 0480:0bb8 */
    default: port_unhandled("cursor", id, count);
    }
    return 0;
}
