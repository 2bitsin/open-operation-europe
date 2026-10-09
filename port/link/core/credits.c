/* Link: core/credits.c (0bc3:0790 opeu_division_credits). Events, from the
 * host branch's comment, resolved by address (symbols.json):
 *   0 input flush 0480:00a4 opeu_input_flush
 *   1 turn input A -> AX 05a8:0158 opeu_turn_input_a
 *   2 getch -> AX 0000:4320 C runtime getch (port_getch)
 *   3 backdrop fill color(c) 0a04:0720 opeu_backdrop_fill_color
 *   4 army position(x,y) 0480:0808
 *   5 army print(format) 0480:0bb8 (host opeu_cursor_print)
 *   6 turn input B -> AX 0000:4066 SYSLIB opeu_turn_input_b */
#include "b1_util.h"

uint16_t link_credits(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_input_flush(ds, link_textout_host()); break;
    case 1: r = (uint16_t)opeu_turn_input_a(ds, link_mouse); break;
    case 2: r = (uint16_t)port_getch(); break;
    case 3: opeu_backdrop_fill_color(ds, link_uiprim_host(), (uint8_t)ARG(0)); break;
    case 4: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 5: b1_cursor_print(ds, args, count); break;
    case 6: r = opeu_turn_input_b(); break;
    default: port_unhandled("credits", id, count);
    }
    return r;
}
