/* Routines of map/btlmove.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:12ce battle_remove_unit (AX element, BX unit)
 *   2864:1396 battle_remove_list (BX unit)
 *   2864:1646 battle_update_element (BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_remove_unit)
{
    opeu_battle_remove_unit(ds, link_btlmove, ARG(1), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_remove_list)
{
    opeu_battle_remove_list(ds, link_btlmove, ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_update_element)
{
    opeu_battle_update_element(ds, link_btlmove, ARG(0));
    return 0;
}
