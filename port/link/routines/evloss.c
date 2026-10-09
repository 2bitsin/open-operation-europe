/* Routines of core/evloss.c reached by identifier: the two hourly loss
 * events (evreport service 5) and their far presenters passed to
 * daily_show. No arguments, no result. The events take their BP
 * (entry SP - 2). */
#include "link.h"

PORT_ROUTINE(opeu_loss_present_a)
{
    (void)args; (void)count;
    opeu_loss_present_a(ds, link_evloss);
    return 0;
}

PORT_ROUTINE(opeu_loss_present_b)
{
    (void)args; (void)count;
    opeu_loss_present_b(ds, link_evloss);
    return 0;
}

PORT_ROUTINE(opeu_loss_event_a)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_loss_event_a(ds, link_evloss, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return 0;
}

PORT_ROUTINE(opeu_loss_event_b)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_loss_event_b(ds, link_evloss, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return 0;
}
