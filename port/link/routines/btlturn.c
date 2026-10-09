/* Routines of map/btlturn.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:0d96 battle_computer_turn (() -> AL)
 *   2673:0f44 battle_turn_loop (() -> AX) [frame]
 *   2673:0ff8 battle_return_garrison (())
 *   2673:1128 battle_daily_condition (())
 *   2673:1234 battle_clear_positions (())
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_computer_turn)
{
    (void)args; (void)count;
    return opeu_battle_computer_turn(ds, link_btlturn);
}

PORT_ROUTINE(opeu_battle_turn_loop)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    (void)args; (void)count;
    r = opeu_battle_turn_loop(ds, link_btlturn, BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_return_garrison)
{
    (void)args; (void)count;
    opeu_battle_return_garrison(ds, link_btlturn);
    return 0;
}

PORT_ROUTINE(opeu_battle_daily_condition)
{
    (void)args; (void)count;
    opeu_battle_daily_condition(ds, link_btlturn);
    return 0;
}

PORT_ROUTINE(opeu_battle_clear_positions)
{
    (void)args; (void)count;
    opeu_battle_clear_positions(ds, link_btlturn);
    return 0;
}
