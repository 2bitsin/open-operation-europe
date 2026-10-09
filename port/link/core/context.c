/* Link: core/context.c (1389:000e opeu_daily_context, 1389:011a
 * opeu_turn_pre_context). Events, resolved from the DOS branch's macros:
 *   0 begin 1707:0260 opeu_context_begin (DIALOGS)
 *   1 save 0480:0844 opeu_context_save
 *   2 frame(44,24,w) 0a04:02a0 opeu_context_frame (UIPRIM)
 *   3 print(format,...) 0480:0bb8 opeu_context_print (host opeu_cursor_print)
 *   4 restore 0480:086a opeu_context_restore
 *   5 portrait(70,40,person) 0a04:046c opeu_context_portrait (UIPRIM)
 *   6 position(45,y) 0480:0808 opeu_context_position (host opeu_army_position)
 *   7 finish 0bc3:0042 opeu_army_summary (82-byte workspace scratch-2..+79)
 *   8 context(v) 0d65:00fe opeu_set_turn_context */
#include "b1_util.h"

#define ARMY_WORKSPACE 82u

uint16_t link_context(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t s;
    switch (id) {
    case 0: opeu_context_begin(ds, link_dialogs_host()); break;
    case 1: opeu_context_save(ds, link_textout_host()); break;
    case 2: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 3: b1_cursor_print(ds, args, count); break;
    case 4: opeu_context_restore(ds, link_textout_host()); break;
    case 5: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 6: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 7:
        s = b1_stack_alloc(ARMY_WORKSPACE);
        opeu_army_summary(ds, link_armysum, (uint16_t)(s + 2));
        b1_stack_free(ARMY_WORKSPACE);
        break;
    case 8: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;
    default: port_unhandled("context", id, count);
    }
    return 0;
}
