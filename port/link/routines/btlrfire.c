/* Routines of map/btlrfire.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2db6:0cc8 battle_ranged_fire (AX target, BX shooter) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_ranged_fire)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_ranged_fire(ds, link_btlrfire, ARG(1), ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
