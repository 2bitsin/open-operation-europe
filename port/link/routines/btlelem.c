/* Routines of map/btlelem.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:0f1e battle_draw_element_at (AL x, DL y, BX element, stack mode) [frame]
 *   2864:1060 battle_redraw_cell (AL x, DL y, BX element, stack mode) [frame]
 *   2864:109a battle_refresh_cell (AL x, DL y) [frame]
 *   2864:121e battle_animate_element (AL x, DL y, BX turn, stack pause, offset, element) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_draw_element_at)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_draw_element_at(ds, link_btlelem, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), (uint8_t)ARG(3), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_redraw_cell)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_redraw_cell(ds, link_btlelem, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), (uint8_t)ARG(3), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_refresh_cell)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_refresh_cell(ds, link_btlelem, (uint8_t)ARG(0), (uint8_t)ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_animate_element)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_animate_element(ds, link_btlelem, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
