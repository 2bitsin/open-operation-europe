/* Routines of core/scenbrf.c reached by identifier:
 *  - the six briefings 0d9b:0038..0784, far callbacks of the scenario
 *    side-initializer table DS:735a (called by opeu_initialize_turn_side),
 *    no arguments;
 *  - opeu_brief_blink 0d9b:0000, stored as the message hook DS:3ec2/3ec4,
 *    no arguments.
 * The briefings keep their 100-byte closing-text buffer in the original
 * frame; the host takes its DS offset (`scratch`), reserved here on the
 * modeled DGROUP stack. */
#include "b1_util.h"

#define CLOSING_BUFFER 100u

typedef void (*Briefing)(uint8_t *, uint16_t (*)(uint8_t *, uint16_t, const uint16_t *, uint16_t), uint16_t);

static uint32_t run_briefing(uint8_t *ds, Briefing briefing)
{
    uint16_t closing = b1_stack_alloc(CLOSING_BUFFER);
    briefing(ds, link_scenbrf, closing);
    b1_stack_free(CLOSING_BUFFER);
    return 0;
}

PORT_ROUTINE(opeu_brief_france)
{
    (void)args; (void)count;
    return run_briefing(ds, opeu_brief_france);
}

PORT_ROUTINE(opeu_brief_africa)
{
    (void)args; (void)count;
    return run_briefing(ds, opeu_brief_africa);
}

PORT_ROUTINE(opeu_brief_kursk)
{
    (void)args; (void)count;
    return run_briefing(ds, opeu_brief_kursk);
}

PORT_ROUTINE(opeu_brief_normandy)
{
    (void)args; (void)count;
    return run_briefing(ds, opeu_brief_normandy);
}

PORT_ROUTINE(opeu_brief_ardennes)
{
    (void)args; (void)count;
    return run_briefing(ds, opeu_brief_ardennes);
}

PORT_ROUTINE(opeu_brief_berlin)
{
    (void)args; (void)count;
    return run_briefing(ds, opeu_brief_berlin);
}

PORT_ROUTINE(opeu_brief_blink)
{
    (void)args; (void)count;
    opeu_brief_blink(ds, link_scenbrf);
    return 0;
}
