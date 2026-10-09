/* Routines of map/btlorder.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:0572 battle_execute_order (AL order, BX element) [frame]
 *   2673:0742 battle_attack_or_hold (AX target, BX element)
 *   2673:07cc battle_continue_order (BX element) [frame]
 *   2673:08dc battle_advance (BX element) [frame]
 *   2673:0902 battle_seek_target (BX element) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_execute_order)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_execute_order(ds, link_btlorder, ARG(1), (uint8_t)ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_attack_or_hold)
{
    opeu_battle_attack_or_hold(ds, link_btlorder, ARG(1), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_continue_order)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_continue_order(ds, link_btlorder, ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_advance)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_advance(ds, link_btlorder, ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_seek_target)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_seek_target(ds, link_btlorder, ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
