/* Link: core/evwound.c (11e5:0004..01a4 hourly wound recovery). Services
 * from the host branch comment: 0 delay_units(units),
 * 1 set_turn_context(value), 2 finish_side_initialization(value),
 * 3 daily_show(id,side,callback offset,callback segment),
 * 4 control_message(text,person). */
#include "c1_common.h"

uint16_t link_evwound(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;                  /* 0a8e:004c */
    case 1: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;               /* 0d65:00fe */
    case 2: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;     /* 0d65:0032 */
    case 3: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                    /* 1707:03e4 */
    case 4: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;        /* 1707:0672 */
    default: port_unhandled("evwound", id, count);
    }
    return 0;
}
