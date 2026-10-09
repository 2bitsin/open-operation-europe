/* Routines of core/settings.c reached by identifier: 2597:0d72
 * opeu_turn_action_10 (the Settings command), an entry of the turn-action
 * far table (data/ctrltext.c). No arguments; takes its entry SP
 * (settings.c host branch: message buffers at sp-2-102 at most, sub-action
 * entry SP at sp-14), placed in a PORT_FRAME on the modeled DGROUP stack. */
#include "b1_util.h"

PORT_ROUTINE(opeu_turn_action_10)
{
    uint16_t sp = b1_sp_enter();
    (void)args; (void)count;
    opeu_turn_action_10(ds, link_settings, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
