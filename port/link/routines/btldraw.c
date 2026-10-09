/* Routines of map/btldraw.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:0690 battle_draw_cursor (AL x, DL y)
 *   2864:06d6 battle_draw_frame (AL x, DL y)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_draw_cursor)
{
    opeu_battle_draw_cursor(ds, link_btldraw, (uint8_t)ARG(0), (uint8_t)ARG(1));
    return 0;
}

PORT_ROUTINE(opeu_battle_draw_frame)
{
    opeu_battle_draw_frame(ds, link_btldraw, (uint8_t)ARG(0), (uint8_t)ARG(1));
    return 0;
}
