/* Routines of core/msnrept.c reached by identifier: the daily_show
 * presentation callbacks 0e8e:0042 mission_finish_present and 0e8e:004a
 * mission_delay_present (far code pointers of SHOW_FINISH/SHOW_DELAY). No
 * arguments. */
#include "link.h"

PORT_ROUTINE(opeu_mission_finish_present)
{
    (void)args; (void)count;
    opeu_mission_finish_present(ds, link_msnrept);
    return 0;
}

PORT_ROUTINE(opeu_mission_delay_present)
{
    (void)args; (void)count;
    opeu_mission_delay_present(ds, link_msnrept);
    return 0;
}
