/* Link: core/divstr.c (06db:0d64 opeu_show_division_strength). Events, from
 * the host branch's comment, resolved by address (symbols.json):
 *   0 set text left(x) 0480:0790 opeu_text_set_left
 *   1 position(x,y) 0480:0808 opeu_context_position (host opeu_army_position)
 *   2 print(format,division,suffix,infantry,tanks,artillery) 0480:0bb8
 *     opeu_context_print (host opeu_cursor_print) */
#include "b1_util.h"

void link_divstr(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_text_set_left(ds, link_textout_host(), (int16_t)ARG(0)); break;
    case 1: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 2: b1_cursor_print(ds, args, count); break;
    default: port_unhandled("divstr", id, count);
    }
}
