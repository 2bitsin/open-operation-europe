/* Routines of core/airdrop.c reached by identifier:
 *  - 23b2:0b28, the paratroop command, from the Air Force mission table
 *    (DGAIR DS:a62e, airforce service 14); no arguments;
 *  - the list-dialog row callbacks 23b2:06ca/06de/077a, stored as far
 *    pointers in the dialog windows (listdlg service 20 passes the choice
 *    count in AL, which these routines do not read); byte result in AL.
 * Each takes its entry SP (`sp`, airdrop.c host comment). */
#include "core/c2_air.h"

PORT_ROUTINE(opeu_code_23b2_0b28)
{
    uint16_t sp = c2_air_sp_enter();
    (void)args; (void)count;
    opeu_code_23b2_0b28(ds, link_airdrop, sp);
    c2_air_leave();
    return 0;
}

PORT_ROUTINE(opeu_air_mark_regiment)
{
    uint16_t sp = c2_air_sp_enter(), r;
    (void)args; (void)count;
    r = opeu_air_mark_regiment(ds, link_airdrop, sp);
    c2_air_leave();
    return r;
}

PORT_ROUTINE(opeu_air_assign_troops)
{
    uint16_t sp = c2_air_sp_enter(), r;
    (void)args; (void)count;
    r = opeu_air_assign_troops(ds, link_airdrop, sp);
    c2_air_leave();
    return r;
}

PORT_ROUTINE(opeu_air_mark_battalion)
{
    uint16_t sp = c2_air_sp_enter(), r;
    (void)args; (void)count;
    r = opeu_air_mark_battalion(ds, link_airdrop, sp);
    c2_air_leave();
    return r;
}
