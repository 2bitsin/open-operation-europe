/* Routines of map/btlhit.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2fe9:0958 battle_resolve_attack (AX target, BX attacker)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_resolve_attack)
{
    opeu_battle_resolve_attack(ds, link_btlhit, ARG(1), ARG(0));
    return 0;
}
