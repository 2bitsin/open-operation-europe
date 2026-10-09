/* Routines of core/delay.c reached by identifier: opeu_choose_turn_delay
 * 1389:012c, entry 0 of the turn far-action table opeu_control_actions
 * (data/ctrltext.c). No arguments. Its host keeps the original frame (36
 * text bytes, then the result word) at `scratch`, reserved here on the
 * modeled DGROUP stack. */
#include "b1_util.h"

#define DELAY_FRAME 38u

PORT_ROUTINE(opeu_choose_turn_delay)
{
    uint16_t scratch;
    (void)args; (void)count;
    scratch = b1_stack_alloc(DELAY_FRAME);
    opeu_choose_turn_delay(ds, link_delay, scratch);
    b1_stack_free(DELAY_FRAME);
    return 0;
}
