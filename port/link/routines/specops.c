/* Routines of core/specops.c and core/specfrc.c reached by identifier:
 * the five Special Forces missions (DGAIR far-code table DS:adb2, run by
 * SPECFRC service 14), their daily_show presenter 24d5:0000, and turn
 * action 9 (24d5:0b26, CTRLTEXT command table, run by CONTROL1's far
 * action). No arguments, no result. Each receives SP at entry. */
#include "link.h"

/* The missions keep their buffers down to BP-0x8e (32-byte name). */
#define SPECIAL_RESERVE PORT_FRAME

PORT_ROUTINE(opeu_special_present)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_special_present(ds, link_specops, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}

PORT_ROUTINE(opeu_special_raid)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(SPECIAL_RESERVE);
    opeu_special_raid(ds, link_specops, sp);
    port_stack_leave(SPECIAL_RESERVE);
    return 0;
}

PORT_ROUTINE(opeu_special_sabotage)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(SPECIAL_RESERVE);
    opeu_special_sabotage(ds, link_specops, sp);
    port_stack_leave(SPECIAL_RESERVE);
    return 0;
}

PORT_ROUTINE(opeu_special_cut_comm)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(SPECIAL_RESERVE);
    opeu_special_cut_comm(ds, link_specops, sp);
    port_stack_leave(SPECIAL_RESERVE);
    return 0;
}

PORT_ROUTINE(opeu_special_assassinate)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(SPECIAL_RESERVE);
    opeu_special_assassinate(ds, link_specops, sp);
    port_stack_leave(SPECIAL_RESERVE);
    return 0;
}

PORT_ROUTINE(opeu_special_recon)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(SPECIAL_RESERVE);
    opeu_special_recon(ds, link_specops, sp);
    port_stack_leave(SPECIAL_RESERVE);
    return 0;
}

PORT_ROUTINE(opeu_turn_action_9)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_turn_action_9(ds, link_specfrc, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
