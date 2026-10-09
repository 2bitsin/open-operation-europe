/* Routines of map/btlpanel.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:02c8 battle_element_panel (AX orders_hidden, BX element) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_element_panel)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_element_panel(ds, link_btlpanel, ARG(1), ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
