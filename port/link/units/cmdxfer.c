/* Link: units/cmdxfer.c (1d42:0000..0403, turn action 3: transfer command).
 * Services from the host branch's list (cmdxfer.c header comment):
 *  0 request_format(dest,format,args...) 0480:0b9a
 *  1 control_message(text,person)        1707:0672
 *  2 confirm_message(text,person)->word  1707:076c
 *  3 control_panel()                     1707:02e2 (opeu_title_panel_begin)
 *  4 list_dialog(window,limit,flags)     0a8e:0bc0
 *  5 group_show(group,index)             15c4:0692
 *  6 unit_announce(index,unit)           15c4:064a: the original passes the
 *    index in AL and the unit in BX (tests/validate_command_transfer.py
 *    'albx'); the callee takes only the unit (BX), so args[1] is passed.
 *  7 daily_show(id,side,offset,segment)  1707:03e4, far presenter seg:off
 *  8 pending_finish(value)               0a8e:005c */
#include "units_F.h"

uint16_t link_cmdxfer(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: r = lf_request_format(ds, args, count); break;
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 2: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 3: opeu_title_panel_begin(ds, link_dialogs_host()); break;
    case 4: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 5:
        sp = port_stack_enter(LF_FRAME);
        opeu_group_show(ds, link_mapunit, (uint16_t)(sp - 2u), ARG(0), (uint8_t)ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 6:
        sp = port_stack_enter(LF_FRAME);
        opeu_unit_announce(ds, link_mapunit, (uint16_t)(sp - 2u), ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 7:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1),
                        (unsigned int)((uint32_t)ARG(3) << 16 | ARG(2)));
        break;
    case 8: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;
    default: port_unhandled("cmdxfer", id, count);
    }
    return r;
}
