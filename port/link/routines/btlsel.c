/* Routines of map/btlsel.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2ec8:0722 battle_mouse_select (AX show_info) [frame]
 *   2ec8:07ea battle_key_select (AX show_info)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_mouse_select)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_mouse_select(ds, link_btlsel, ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_key_select)
{
    return opeu_battle_key_select(ds, link_btlsel, ARG(0));
}
