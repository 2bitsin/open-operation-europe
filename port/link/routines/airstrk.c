/* Routines of core/airstrk.c reached by identifier: 0f43:0406
 * opeu_air_strike_present, the daily-show presenter whose far address
 * opeu_air_strike_event passes (airstrk service 22). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_air_strike_present)
{
    (void)args; (void)count;
    opeu_air_strike_present(ds, link_airstrk);
    return 0;
}
