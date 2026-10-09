/* Link: core/staffpnl.c (06db:09c8..0c20 staff selection panel). Services,
 * from the host branch: copy=0 (six words) -> VGALIB 301F opeu_screen_copy,
 * outline=1 (x1, y1, x2, y2, colour) -> 0a04:065a opeu_ui_outline. */
#include "b2_common.h"

void link_staffpnl(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); break;
    case 1: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    default: port_unhandled("staffpnl", id, count);
    }
}
