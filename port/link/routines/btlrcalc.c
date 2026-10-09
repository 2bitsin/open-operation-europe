/* Routines of map/btlrcalc.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2db6:0174 battle_type_skill (BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_type_skill)
{
    return opeu_battle_type_skill(ds, link_btlrcalc, ARG(0));
}
