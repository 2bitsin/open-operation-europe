/* Routines of units/engineer.c reached by identifier: the five engineering
 * handlers of the DGBATTLE far-code order table (DS:c0c8, finalact service
 * 9; no arguments, return AX, take their BP = entry SP - 2) and the four
 * finish stubs passed to daily_show (far pointers 319e:00a0/016a/02b8/03ba,
 * dialogs service 47). */
#include "link.h"

PORT_ROUTINE(opeu_engineer_lay_mines)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_engineer_lay_mines(ds, link_engineer, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_engineer_remove_mines)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_engineer_remove_mines(ds, link_engineer, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_engineer_build_bridge)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_engineer_build_bridge(ds, link_engineer, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_engineer_repair_objective)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_engineer_repair_objective(ds, link_engineer, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_engineer_repair_unit)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_engineer_repair_unit(ds, link_engineer, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_mines_laid_finish)
{
    (void)args; (void)count;
    opeu_mines_laid_finish(ds, link_engineer);
    return 0;
}

PORT_ROUTINE(opeu_mines_removed_finish)
{
    (void)args; (void)count;
    opeu_mines_removed_finish(ds, link_engineer);
    return 0;
}

PORT_ROUTINE(opeu_bridge_built_finish)
{
    (void)args; (void)count;
    opeu_bridge_built_finish(ds, link_engineer);
    return 0;
}

PORT_ROUTINE(opeu_arms_repaired_finish)
{
    (void)args; (void)count;
    opeu_arms_repaired_finish(ds, link_engineer);
    return 0;
}
