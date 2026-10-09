/* Routines of map/btlfire.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2fe9:022a battle_fire_hits (AX target, BX attacker)
 *   2fe9:0394 battle_fire_power (AX target, BX element)
 *   2fe9:0498 battle_fire_defense (BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_fire_hits)
{
    return opeu_battle_fire_hits(ds, link_btlfire, ARG(1), ARG(0));
}

PORT_ROUTINE(opeu_battle_fire_power)
{
    return opeu_battle_fire_power(ds, link_btlfire, ARG(1), ARG(0));
}

PORT_ROUTINE(opeu_battle_fire_defense)
{
    return opeu_battle_fire_defense(ds, link_btlfire, ARG(0));
}
