/* Routines of map/btlmines.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2eab:0086 battle_mine_damage (BX element)
 *   2eab:00c8 battle_mine_hit (BX element) [sp]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_mine_damage)
{
    return opeu_battle_mine_damage(ds, link_btlmines, ARG(0));
}

PORT_ROUTINE(opeu_battle_mine_hit)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_mine_hit(ds, link_btlmines, sp, ARG(0));
    btl_sp_leave();
    return 0;
}
