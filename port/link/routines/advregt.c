/* Routines of core/advregt.c reached by identifier: 1dd2:1140, entry 2 of
 * the advice action table DS:9060 (data/dgorders.c). No arguments; `sp` is
 * the entry SP, far_text the FADVICE segment 3ad9; AX returned. */
#include "core/c2_adv.h"

PORT_ROUTINE(opeu_code_1dd2_1140)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_code_1dd2_1140(ds, C2_ADVICE_TEXT(), link_advregt, sp);
    port_stack_leave(PORT_FRAME);
    return r;
}
