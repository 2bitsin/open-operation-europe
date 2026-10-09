/* Routines of core/airlift.c reached by identifier: 23b2:0e20, the airlift
 * command, from the Air Force mission table (DGAIR DS:a62e, airforce
 * service 14). No arguments; takes its entry SP (airlift.c host comment). */
#include "core/c2_air.h"

PORT_ROUTINE(opeu_code_23b2_0e20)
{
    uint16_t sp = c2_air_sp_enter();
    (void)args; (void)count;
    opeu_code_23b2_0e20(ds, link_airlift, sp);
    c2_air_leave();
    return 0;
}
