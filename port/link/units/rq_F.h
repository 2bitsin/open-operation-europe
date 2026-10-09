/* The request-menu service numbering (tests/validate_request_menu.py
 * SERVICES) as RQARMS and RQREINF raise it. Conversion only; each module's
 * link callback passes its name for diagnostics.
 *  0 rand_bounded(bound)              09f6:00d4
 *  1 min16(a,b)                       09cc:027e
 *  2 max16(a,b)                       09cc:028c
 *  3 request_pending(kind)            22f9:0036
 *  4 request_cancel_prompt(text)      22f9:0072 (scratch: buffer at +4)
 *  5 request_no_more(text)            22f9:000e (scratch: buffer at +4)
 *  6 request_file(target,argument,kind) 22f9:00a2: the original passes the
 *    address of its 4-byte request local; the words are laid out on the
 *    modeled stack
 *  7 collect_free_units(list,mask)    145c:154e (opeu_release_side_units)
 *  8 collect_free_elements(list,mask) 145c:14d2 (opeu_release_side_elements)
 *  9 control_panel()                  1707:02e2 (opeu_title_panel_begin)
 * 10 control_message(text,person)     1707:0672
 * 11 list_dialog(dialog,mode,height)  0a8e:0bc0
 * 12 unit_index(unit)                 145c:0034
 * 13 category_0_to_3(code)            0b73:002e
 * 14 category_4_or_5(code)            0b73:003a
 * 15 category_6(code)                 0b73:004a
 * 16 army_title(text)                 1707:03aa
 * 17 delay_dialog(text,last,person,menu) 1707:0726
 * 18 confirm_ok(person)               1707:079e
 * 19 request_format(dest,format,...)  0480:0b9a
 * 20 confirm_message(text,person)     1707:076c
 * 23 map_pick_location(x_address,y_address) 17c4:0274 (frame)
 * 25 available_element_types(mask,category,out) 145c:10a2
 *    (opeu_list_due_records(list=out, mask, kind=category); registers
 *    AL mask, DL category, BX out) */
#ifndef PORT_LINK_UNITS_RQ_F_H
#define PORT_LINK_UNITS_RQ_F_H

#include "units_F.h"

/* Caller-supplied scratch of the 22f9 host entries: a DS area inside a
 * modeled frame with room for the frame buffers at scratch+4. */
#define LF_RQ_SCRATCH 0x80u

static inline uint16_t lf_rq_service(const char *module, uint8_t *ds, uint16_t id,
                                     const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: r = opeu_rand_bounded(ARG(0)); break;
    case 1: r = opeu_min16(ARG(0), ARG(1)); break;
    case 2: r = opeu_max16(ARG(0), ARG(1)); break;
    case 3: r = opeu_request_pending(ds, (uint8_t)ARG(0)); break;
    case 4:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_request_cancel_prompt(ds, link_rqcommon, (uint16_t)(sp - LF_RQ_SCRATCH), ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 5:
        sp = port_stack_enter(LF_FRAME);
        opeu_request_no_more(ds, link_rqcommon, (uint16_t)(sp - LF_RQ_SCRATCH), ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 6:
        sp = port_stack_enter(LF_FRAME);
        port_ds_set_word((uint16_t)(sp - 4u), ARG(0));
        ds[(uint16_t)(sp - 2u)] = (uint8_t)ARG(1);
        ds[(uint16_t)(sp - 1u)] = (uint8_t)ARG(2);
        opeu_request_file(ds, link_rqcommon, (uint16_t)(sp - 4u));
        port_stack_leave(LF_FRAME);
        break;
    case 7: r = opeu_release_side_units(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 8: r = opeu_release_side_elements(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 9: opeu_title_panel_begin(ds, link_dialogs_host()); break;
    case 10: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 11: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 12: r = opeu_unit_index(ARG(0)); break;
    case 13: r = opeu_category_0_to_3((uint8_t)ARG(0)); break;
    case 14: r = opeu_category_4_or_5((uint8_t)ARG(0)); break;
    case 15: r = opeu_category_6((uint8_t)ARG(0)); break;
    case 16: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;
    case 17: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 18: r = opeu_confirm_ok(ds, link_dialogs_host(), ARG(0)); break;
    case 19: r = lf_request_format(ds, args, count); break;
    case 20: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 23:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_map_pick_location(ds, link_mappick, (uint16_t)(sp - 2u), ARG(0), ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 25: r = opeu_list_due_records(ds, ARG(2), (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

#endif
