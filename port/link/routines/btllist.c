/* Routines of map/btllist.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:000e battle_element_side (BX element)
 *   2864:001c battle_unit_side (BX unit)
 *   2864:002c battle_element_unit (BX element)
 *   2864:009c battle_element_count (BX unit)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_element_side)
{
    return opeu_battle_element_side(ds, link_btllist, ARG(0));
}

PORT_ROUTINE(opeu_battle_unit_side)
{
    return opeu_battle_unit_side(ds, link_btllist, ARG(0));
}

PORT_ROUTINE(opeu_battle_element_unit)
{
    return opeu_battle_element_unit(ds, link_btllist, ARG(0));
}

PORT_ROUTINE(opeu_battle_element_count)
{
    return opeu_battle_element_count(ds, link_btllist, ARG(0));
}
