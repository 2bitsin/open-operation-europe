/* Routines of core/persmenu.c reached by identifier (segment 1d82):
 *  0004 personnel_finish   picture callback of daily_show (no arguments)
 *  000c commander_always   list select callback (AL = row) -> AL
 *  003c division_toggle    list select callback (AL = row) -> AL
 *  0084 division_commander_menu, 010e commander_assign, 0412 commander_rest
 *                          DGORDERS far action table DS:8f6c (no arguments)
 *  04c8 turn_action_4      command action table DS:7fb4 (no arguments)
 * Host entries take sp = entry SP (BP = sp-2; commander_assign keeps
 * 0x12c bytes of menu texts below BP) and the run-time paragraph of 1d82. */
#include "core/b2_common.h"

#define PERS_SEGMENT 0x1d82u
#define PERS_STACK 0x200u

PORT_ROUTINE(opeu_personnel_finish)
{
    (void)args; (void)count;
    B2_SP_CALL(PORT_FRAME, opeu_personnel_finish(ds, link_persmenu, sp, PERS_SEGMENT));
    return 0;
}

PORT_ROUTINE(opeu_commander_always)
{
    uint8_t r = 0;
    B2_SP_CALL(PORT_FRAME, r = opeu_commander_always(ds, link_persmenu, sp, PERS_SEGMENT, (uint8_t)ARG(0)));
    return r;
}

PORT_ROUTINE(opeu_division_toggle)
{
    uint8_t r = 0;
    B2_SP_CALL(PORT_FRAME, r = opeu_division_toggle(ds, link_persmenu, sp, PERS_SEGMENT, (uint8_t)ARG(0)));
    return r;
}

PORT_ROUTINE(opeu_division_commander_menu)
{
    (void)args; (void)count;
    B2_SP_CALL(PORT_FRAME, opeu_division_commander_menu(ds, link_persmenu, sp, PERS_SEGMENT));
    return 0;
}

PORT_ROUTINE(opeu_commander_assign)
{
    (void)args; (void)count;
    B2_SP_CALL(PERS_STACK, opeu_commander_assign(ds, link_persmenu, sp, PERS_SEGMENT));
    return 0;
}

PORT_ROUTINE(opeu_commander_rest)
{
    (void)args; (void)count;
    B2_SP_CALL(PORT_FRAME, opeu_commander_rest(ds, link_persmenu, sp, PERS_SEGMENT));
    return 0;
}

PORT_ROUTINE(opeu_turn_action_4)
{
    (void)args; (void)count;
    B2_SP_CALL(PORT_FRAME, opeu_turn_action_4(ds, link_persmenu, sp, PERS_SEGMENT));
    return 0;
}
