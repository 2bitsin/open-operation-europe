/* Routines of map/btlcalc.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2808:0008 battle_pause (AL ticks)
 *   2808:000e battle_unit_direction (BX unit)
 *   2808:0064 battle_skill (AL kind, BX element)
 *   2808:013e battle_element_strength (BX element)
 *   2808:0224 battle_odds (AL rating, DL range)
 *   2808:025e battle_element_rating (AX target, BX attacker)
 *   2808:02b8 battle_element_range (BX element)
 *   2808:02d6 battle_element_shots (AX target, BX attacker)
 *   2808:034a battle_cell_terrain (AL x, DL y)
 *   2808:036c battle_can_enter (AL x, DL y, BX element)
 *   2808:03a4 battle_move_cost (AL x, DL y, BX element)
 *   2808:04e8 battle_step (AX y pointer, DL direction, BX x pointer)
 *   2808:0534 battle_unit_controlled (BX unit)
 *   2808:0556 battle_element_at (AL x, DL y)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_pause)
{
    opeu_battle_pause(ds, link_btlcalc, (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_unit_direction)
{
    return opeu_battle_unit_direction(ds, link_btlcalc, ARG(0));
}

PORT_ROUTINE(opeu_battle_skill)
{
    return opeu_battle_skill(ds, link_btlcalc, ARG(1), (uint8_t)ARG(0));
}

PORT_ROUTINE(opeu_battle_element_strength)
{
    return opeu_battle_element_strength(ds, link_btlcalc, ARG(0));
}

PORT_ROUTINE(opeu_battle_odds)
{
    return opeu_battle_odds(ds, link_btlcalc, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_battle_element_rating)
{
    return opeu_battle_element_rating(ds, link_btlcalc, ARG(1), ARG(0));
}

PORT_ROUTINE(opeu_battle_element_range)
{
    return opeu_battle_element_range(ds, link_btlcalc, ARG(0));
}

PORT_ROUTINE(opeu_battle_element_shots)
{
    return opeu_battle_element_shots(ds, link_btlcalc, ARG(1), ARG(0));
}

PORT_ROUTINE(opeu_battle_cell_terrain)
{
    return opeu_battle_cell_terrain(ds, link_btlcalc, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_battle_can_enter)
{
    return opeu_battle_can_enter(ds, link_btlcalc, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2));
}

PORT_ROUTINE(opeu_battle_move_cost)
{
    return opeu_battle_move_cost(ds, link_btlcalc, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2));
}

PORT_ROUTINE(opeu_battle_step)
{
    return opeu_battle_step(ds, link_btlcalc, PORT_DS(ARG(2)), PORT_DS(ARG(0)), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_battle_unit_controlled)
{
    return opeu_battle_unit_controlled(ds, link_btlcalc, ARG(0));
}

PORT_ROUTINE(opeu_battle_element_at)
{
    return opeu_battle_element_at(ds, link_btlcalc, (uint8_t)ARG(0), (uint8_t)ARG(1));
}
