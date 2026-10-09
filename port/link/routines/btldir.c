/* Routines of map/btldir.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:0008 battle_cell_facing (AL x, DL y, BL to_x, stack to_y)
 *   2673:003e battle_element_facing (AX other, BX element)
 *   2673:00ac battle_terrain_mode (AX other, BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_cell_facing)
{
    return opeu_battle_cell_facing(ds, link_btldir, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3));
}

PORT_ROUTINE(opeu_battle_element_facing)
{
    return opeu_battle_element_facing(ds, link_btldir, ARG(1), ARG(0));
}

PORT_ROUTINE(opeu_battle_terrain_mode)
{
    return opeu_battle_terrain_mode(ds, link_btldir, ARG(1), ARG(0));
}
