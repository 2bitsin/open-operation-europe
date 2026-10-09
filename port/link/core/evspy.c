/* Link: core/evspy.c (108c:0fd8..1301 the evening visitor event). Services
 * from the host branch comment:
 * 0 visit_unit_safe(unit), 1 message panel (1707:0346), 2 context_frame(x,y,w),
 * 3 army_print(format), 4 dos_seek(handle,low,high,origin),
 * 5 image_read_draw(x,y,w,h,mode,handle), 6 army_position(x,y),
 * 7 request_format(destination,format[,argument]), 8 message_text(text),
 * 9 ui_text(text), 10 answer (0541:060a), 11 set_turn_context(value),
 * 12 finish_side_initialization(value), 13 daily_show(id,side,offset,segment),
 * 14 control_message(text,person), 15 control_ask(text,person),
 * 16 message_lookup(id). */
#include "c1_common.h"

uint16_t link_evspy(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t scratch, r = 0;
    switch (id) {
    case 0: r = opeu_visit_unit_safe(ds, ARG(0)); break;                                  /* 108c:000e */
    case 1: opeu_message_panel_begin(ds, link_dialogs_host()); break;                    /* 1707:0346 */
    case 2: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;   /* 0a04:02a0 */
    case 3: c1_cursor_print(ds, args, count); break;                                     /* 0480:0bb8 */
    case 4:                                                                               /* FILEIO 009b */
        r = (uint16_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3));
        break;
    case 5:                                                                               /* 0a04:000c */
        opeu_image_read_draw(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), (int16_t)ARG(5));
        break;
    case 6: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break; /* 0480:0808 */
    case 7: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 8: opeu_text_feed(ds, link_dialogs_host(), ARG(0)); break;                      /* 1707:0652 message_text */
    case 9: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;                        /* 0480:0484 */
    case 10:                                                                              /* 0541:060a yes_no */
        scratch = c1_scratch_enter(16);
        r = (uint16_t)opeu_yes_no(ds, link_keyboard, scratch);
        c1_scratch_leave(16);
        break;
    case 11: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;              /* 0d65:00fe */
    case 12: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;    /* 0d65:0032 */
    case 13: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                   /* 1707:03e4 */
    case 14: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;       /* 1707:0672 */
    case 15: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;   /* 1707:076c control_ask */
    case 16: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;          /* 312c:000c */
    default: port_unhandled("evspy", id, count);
    }
    return r;
}
