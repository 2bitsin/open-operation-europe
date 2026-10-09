/* Routines of core/airforce.c reached by identifier: 23b2:10da
 * opeu_turn_action_8 (the Air Force command), an entry of the turn-action
 * far table (data/ctrltext.c). No arguments; takes its entry SP
 * (airforce.c host comment). */
#include "core/c2_air.h"

PORT_ROUTINE(opeu_turn_action_8)
{
    uint16_t sp = c2_air_sp_enter();
    (void)args; (void)count;
    opeu_turn_action_8(ds, link_airforce, sp);
    c2_air_leave();
    return 0;
}
