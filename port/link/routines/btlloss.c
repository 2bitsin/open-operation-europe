/* Routines of map/btlloss.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   29f5:050e battle_loss_check (AX target, BX attacker)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_loss_check)
{
    opeu_battle_loss_check(ds, link_btlloss, ARG(1), ARG(0));
    return 0;
}
