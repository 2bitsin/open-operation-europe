/*
 * (orders, organization, transfers, allocation, tables, supply).
 * Only calling conventions live here: modeled DGROUP frames for the host
 * entries that take `sp`/`frame`/`scratch`, the variadic request_format
 * argument block, and the numbered services that the order modules share
 * (ORDTGT/ORDUNIT/ORDLIST/ORDISSUE/ORDACT document one numbering). */
#ifndef PORT_LINK_UNITS_F_H
#define PORT_LINK_UNITS_F_H

#include "link.h"

/* Reserve below the modeled SP for one callee taking an entry SP (or a BP
 * derived from it): its locals plus the frames of the routines it calls
 * internally with sp-n. Generous: the DGROUP stack is 5 KiB. */
#define LF_FRAME 0x200u

/* Variadic request_format(dest, fmt, args...) (0480:0b9a): the host entry
 * takes the DS offset of the argument words as the original stack holds
 * them after the far return address. The words are laid out in the
 * reserved part of the modeled stack; the callee's own buffers go below. */
static inline uint16_t lf_request_format(uint8_t *ds, const uint16_t *args, uint16_t count)
{
    uint16_t sp = port_stack_enter((uint16_t)(LF_FRAME + 2u * count));
    uint16_t at = (uint16_t)(sp - 2u * count);
    uint16_t i;
    for (i = 0; i < count; i++)
        port_ds_set_word((uint16_t)(at + 2u * i), args[i]);
    opeu_request_format(ds, link_textout_host(), at);
    port_stack_leave((uint16_t)(LF_FRAME + 2u * count));
    return 0;
}

/* army_print / context_print / turn_label (0480:0bb8, printf to the text
 * console): same argument block as request_format. */
static inline uint16_t lf_cursor_print(uint8_t *ds, const uint16_t *args, uint16_t count)
{
    uint16_t sp = port_stack_enter((uint16_t)(LF_FRAME + 2u * count));
    uint16_t at = (uint16_t)(sp - 2u * count);
    uint16_t i;
    for (i = 0; i < count; i++)
        port_ds_set_word((uint16_t)(at + 2u * i), args[i]);
    opeu_cursor_print(ds, link_textout_host(), at);
    port_stack_leave((uint16_t)(LF_FRAME + 2u * count));
    return 0;
}

/* Shared order-module services (ORDTGT host comment, map/ordtgt.c):
 * 0 request_format(dest,fmt,args...)   0480:0b9a
 * 1 control_message(text,person)       1707:0672
 * 2 confirm_message(text,person)       1707:076c
 * 3 arrival_location(x,y,side)         15c4:1408
 * 4 pick_location(x_address,y_address) 17c4:0274
 * 5 control_panel()                    1707:02e2 (title_panel_begin)
 * 6 group_show(group,person)           15c4:0692
 * 7 list_dialog(list,mode,flags)       0a8e:0bc0
 * 8 unit_order_menu(group)             1c23:03c0
 * 9 speaker_panel(person,show)         1707:04d4
 * 10 unit_announce(unit,group)         15c4:064a (takes the unit only)
 * 11 army_position(x,y)                0480:0808
 * 12 prompt_value_cancel(text,max,items) 0541:0462
 * 13 control_position(x,y)             15c4:13d8
 * 14 message_lookup(id)                312c:000c
 * 15 delay_dialog(text,x,person,menu)  1707:0726
 * 16 delay_units(units)                0a8e:004c
 * 17 daily_context(side)               1389:000e
 * Returns 1 and stores the result when `id` is one of them. */
static inline int lf_order_service(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count, uint16_t *result)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: r = lf_request_format(ds, args, count); break;
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 2: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 3:
        sp = port_stack_enter(LF_FRAME);
        opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        port_stack_leave(LF_FRAME);
        break;
    case 4:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_map_pick_location(ds, link_mappick, (uint16_t)(sp - 2u), ARG(0), ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 5: opeu_title_panel_begin(ds, link_dialogs_host()); break;
    case 6:
        sp = port_stack_enter(LF_FRAME);
        opeu_group_show(ds, link_mapunit, (uint16_t)(sp - 2u), ARG(0), (uint8_t)ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 7: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 8:
        sp = port_stack_enter(LF_FRAME);
        opeu_unit_order_menu(ds, link_ordissue, sp, (uint8_t)ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 9: r = opeu_speaker_panel(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 10:
        sp = port_stack_enter(LF_FRAME);
        opeu_unit_announce(ds, link_mapunit, (uint16_t)(sp - 2u), ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 11: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 12:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_prompt_value_cancel(ds, link_keyboard, ARG(0), ARG(1), ARG(2), (uint16_t)(sp - 0x10u));
        port_stack_leave(LF_FRAME);
        break;
    case 13: opeu_control_position(ds, ARG(0), ARG(1)); break;
    case 14: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    case 15: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 16: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 17: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;
    default: return 0;
    }
    *result = r;
    return 1;
}

#endif
