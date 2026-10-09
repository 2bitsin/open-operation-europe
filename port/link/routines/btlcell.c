/* Routines of map/btlcell.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:0ab2 battle_draw_cell (AL x, DL y)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_draw_cell)
{
    opeu_battle_draw_cell(ds, link_btlcell, (uint8_t)ARG(0), (uint8_t)ARG(1));
    return 0;
}
