/* Routines of map/hqloss.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2beb:0132 battle_auto_hq (()) [sp]
 *   2beb:05a2 battle_hq_lost (()) [sp]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_auto_hq)
{
    uint16_t sp = btl_sp_enter();
    (void)args; (void)count;
    opeu_battle_auto_hq(ds, link_hqloss, sp);
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_hq_lost)
{
    uint16_t sp = btl_sp_enter();
    (void)args; (void)count;
    opeu_battle_hq_lost(ds, link_hqloss, sp);
    btl_sp_leave();
    return 0;
}
