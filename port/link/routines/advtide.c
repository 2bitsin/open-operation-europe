/* Routines of core/advtide.c reached by identifier: 1dd2:1e16, entry 4 of
 * the advice action table DS:9060 (data/dgorders.c). No arguments; `sp` is
 * the entry SP, far_text the FADVICE segment 3ad9. */
#include "core/c2_adv.h"

PORT_ROUTINE(opeu_code_1dd2_1e16)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_code_1dd2_1e16(ds, C2_ADVICE_TEXT(), link_advtide, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
