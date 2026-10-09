/* Routines of core/advhints.c reached by identifier: 1dd2:207a, entry 5 of
 * the advice action table DS:9060 (data/dgorders.c). No arguments; `sp` is
 * the entry SP, far_text the FADVICE segment 3ad9; AX returned. */
#include "core/c2_adv.h"

PORT_ROUTINE(opeu_code_1dd2_207a)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_code_1dd2_207a(ds, C2_ADVICE_TEXT(), link_advhints, sp);
    port_stack_leave(PORT_FRAME);
    return r;
}
