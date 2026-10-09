/* Routines of map/btlunit.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2864:1732 battle_order (BX element)
 *   2864:173a battle_set_order (AL order, BX element)
 *   2864:176a battle_clear_cell (AL x, DL y)
 *   2864:1786 battle_element_flag2 (BX element)
 *   2864:1794 battle_element_active (BX element)
 *   2864:17ae battle_unit_active (BX unit)
 *   2864:17e6 battle_mode (BX element)
 *   2864:1852 battle_cell_occupied (AL x, DL y)
 *   2864:1906 battle_level_allowed (BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_order)
{
    return opeu_battle_order(ds, link_btlunit, ARG(0));
}

PORT_ROUTINE(opeu_battle_set_order)
{
    opeu_battle_set_order(ds, link_btlunit, ARG(1), (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_clear_cell)
{
    opeu_battle_clear_cell(ds, link_btlunit, (uint8_t)ARG(0), (uint8_t)ARG(1));
    return 0;
}

PORT_ROUTINE(opeu_battle_element_flag2)
{
    return opeu_battle_element_flag2(ds, link_btlunit, ARG(0));
}

PORT_ROUTINE(opeu_battle_element_active)
{
    return opeu_battle_element_active(ds, link_btlunit, ARG(0));
}

PORT_ROUTINE(opeu_battle_unit_active)
{
    return opeu_battle_unit_active(ds, link_btlunit, ARG(0));
}

PORT_ROUTINE(opeu_battle_mode)
{
    return opeu_battle_mode(ds, link_btlunit, ARG(0));
}

PORT_ROUTINE(opeu_battle_cell_occupied)
{
    return opeu_battle_cell_occupied(ds, link_btlunit, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_battle_level_allowed)
{
    return opeu_battle_level_allowed(ds, link_btlunit, ARG(0));
}
