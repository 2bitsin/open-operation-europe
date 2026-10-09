/* Link: core/noonevt.c (0e57 dated noon events). Services, from the host
 * branch and tests/validate_noon_events.py SERVICES:
 * 0 message_panel_begin, 1 ui_text(text), 2 context_frame(x,y,width),
 * 3 army_print(format,arg) (cdecl), 4 context_portrait(x,y,person),
 * 5 army_position(x,y), 6 wait_confirm(value), 7 text_feed(text),
 * 8 daily_context(side), 9 turn_active_begin, 10 sound_cue10,
 * 11 control_message(text,person), 12 far action (offset, segment) outside
 * segment 0e57. */
#include "c1_common.h"

uint16_t link_noonevt(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_message_panel_begin(ds, link_dialogs_host()); break;                   /* 1707:0346 */
    case 1: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;    /* 0480:0484 */
    case 2: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;  /* 0a04:02a0 */
    case 3: c1_cursor_print(ds, args, count); break;    /* 0480:0bb8 */
    case 4: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break; /* 0a04:046c */
    case 5: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break; /* 0480:0808 */
    case 6: opeu_wait_confirm(ds, link_keyboard, (int16_t)ARG(0)); break;               /* 0541:011e */
    case 7: opeu_text_feed(ds, link_dialogs_host(), ARG(0)); break;    /* 1707:0652 */
    case 8: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;               /* 1389:000e */
    case 9: opeu_turn_active_begin(ds, link_dialogs_host()); break;    /* 1707:020c */
    case 10: opeu_sound_cue10(ds, link_soundcue); break;    /* 0d92:0058 */
    case 11: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;      /* 1707:0672 */
    case 12: port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), NULL, 0); break;            /* far action */
    default: port_unhandled("noonevt", id, count);
    }
    return 0;
}
