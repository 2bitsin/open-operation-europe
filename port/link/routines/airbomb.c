/* Routines of core/airbomb.c reached by identifier: 23b2:05b0, the bombing
 * command, from the Air Force mission table (DGAIR DS:a62e, airforce
 * service 14). No arguments; takes its entry SP (airbomb.c host comment). */
#include "core/c2_air.h"

PORT_ROUTINE(opeu_code_23b2_05b0)
{
    uint16_t sp = c2_air_sp_enter();
    (void)args; (void)count;
    opeu_code_23b2_05b0(ds, link_airbomb, sp);
    c2_air_leave();
    return 0;
}
