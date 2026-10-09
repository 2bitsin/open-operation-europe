/* Link: core/notend.c (3424:06be opeu_notice_end). Services, from the
 * host branch and tests/validate_notice_end.py SERVICES:
 * 0 sound cue 10, 1 wait(units), 2 arrival_location(x,y,side),
 * 3 unit_erase(unit), 4 arrival_prepare(unit),
 * 5 request_format(destination,format,...) (cdecl), 6 unit_message(unit,text),
 * 7 daily_show(id,side,callback offset,callback segment),
 * 8 element_flag_side(element) -> side, 9 unit_name(unit,buffer),
 * 10 item_name(item,buffer), 11 type_name(type,buffer),
 * 12 string_copy(destination,source), 13 string_append(destination,source),
 * 14 report_element(element), 15 report_element_type(pointer),
 * 16 control_message(text,person), 17 turn_prepare(), 18 daily_context(side).
 * Pointer arguments/results are DS offsets. */
#include "c1_common.h"

uint16_t link_notend(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: opeu_sound_cue10(ds, link_soundcue); break;                                  /* 0d92:0058 */
    case 1: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;                  /* 0a8e:004c */
    case 2:                                                                               /* 15c4:1408 */
        /* map frame convention: the callee's BP (entry SP - 2). */
        sp = port_stack_enter(PORT_FRAME);
        opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2), (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        port_stack_leave(PORT_FRAME);
        break;
    case 3:                                                                               /* 15c4:0fe4 unit_erase */
        sp = port_stack_enter(PORT_FRAME);
        opeu_unit_remove_draw(ds, link_maperase, (uint16_t)(sp - 2), ARG(0));
        port_stack_leave(PORT_FRAME);
        break;
    case 4:                                                                               /* 15c4:0fd0 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_arrival_prepare(ds, link_maperase, (uint16_t)(sp - 2), ARG(0));
        port_stack_leave(PORT_FRAME);
        break;
    case 5: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 6: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;            /* 1707:07ec */
    case 7:                                                                               /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    case 8: r = opeu_battle_element_side(ds, link_btllist, ARG(0)); break;               /* 2864:000e */
    case 9: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;                /* 0b73:0188 */
    case 10: r = opeu_item_name(ds, ARG(0), ARG(1)); break;                              /* 0b73:0210 */
    case 11: r = opeu_type_name(ds, ARG(0), ARG(1)); break;                              /* 0b73:01de */
    case 12: r = opeu_string_copy(ds, ARG(0), ARG(1)); break;                            /* 09cc:01d0 */
    case 13: r = opeu_string_append(ds, ARG(0), ARG(1)); break;                          /* 09cc:01f6 */
    case 14: opeu_group_element_panel(ds, link_dialogs_host(), ARG(0)); break;           /* 1707:0b00 report_element */
    case 15: opeu_element_panel(ds, link_dialogs_host(), ARG(0)); break;                 /* 1707:0b76 report_element_type */
    case 16: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;       /* 1707:0672 */
    case 17: opeu_turn_prepare(ds, link_dialogs_host()); break;                          /* 1707:00c8 */
    case 18: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;               /* 1389:000e */
    default: port_unhandled("notend", id, count);
    }
    return r;
}
