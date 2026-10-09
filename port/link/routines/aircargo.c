/* Routines of core/aircargo.c reached by identifier: 0f43:1042
 * opeu_cargo_drop_present, the daily-show presenter whose far address
 * opeu_cargo_drop_event passes (aircargo service 18). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_cargo_drop_present)
{
    (void)args; (void)count;
    opeu_cargo_drop_present(ds, link_aircargo);
    return 0;
}
