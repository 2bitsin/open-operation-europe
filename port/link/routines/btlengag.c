/* Routines of map/btlengag.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:00d2 battle_supply_notice (AL supply, BX element) [frame]
 *   2673:0164 battle_target_in_range (BX element)
 *   2673:01c6 battle_cell_in_range (BX element)
 *   2673:020a battle_animate_facing (AL mode, BX element) [frame]
 *   2673:02ee battle_step_into (AL x, DL y, BX element) [frame]
 *   2673:0414 battle_next_step (AX x pointer, DX y pointer, BX element) [frame]
 *   2673:0510 battle_element_path_step (AX x pointer, DX y pointer, BX element) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_supply_notice)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_supply_notice(ds, link_btlengag, ARG(1), (uint8_t)ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_target_in_range)
{
    return opeu_battle_target_in_range(ds, link_btlengag, ARG(0));
}

PORT_ROUTINE(opeu_battle_cell_in_range)
{
    return opeu_battle_cell_in_range(ds, link_btlengag, ARG(0));
}

PORT_ROUTINE(opeu_battle_animate_facing)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_animate_facing(ds, link_btlengag, ARG(1), (uint8_t)ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_step_into)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_step_into(ds, link_btlengag, ARG(2), (uint8_t)ARG(0), (uint8_t)ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_next_step)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_next_step(ds, link_btlengag, ARG(2), ARG(0), ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_element_path_step)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_element_path_step(ds, link_btlengag, ARG(2), ARG(0), ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return r;
}
