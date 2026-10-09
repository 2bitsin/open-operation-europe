/* Link: core/scenbrf.c (0d9b:0000..0933 scenario briefings). Services, from
 * the host branch's comment:
 *   0 daily activate(side) 15c4:0ff8 opeu_daily_activate (MAPERASE, BP frame)
 *   1 control message(text,speaker) 1707:0672 opeu_control_message (DIALOGS)
 *   2 overview pixel(x pointer,y pointer) 15c4:054a opeu_overview_pixel
 *   3 invert(x,y,w,h,mode) 0000:2b0e opeu_ui_invert (VGALIB opeu_ui_highlight)
 *   4 format(destination,format,person) 0480:0b9a opeu_request_format
 * The briefings and the message hook are reached through far code pointers
 * (side-initializer table DS:735a, message hook DS:3ec2): their PORT_ROUTINE
 * handlers are in port/link/routines/scenbrf.c. */
#include "b1_util.h"

uint16_t link_scenbrf(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, f;
    switch (id) {
    case 0:
        f = b1_frame_enter();
        opeu_daily_activate(ds, link_maperase, f, (uint8_t)ARG(0));
        b1_frame_leave();
        break;
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 2: opeu_overview_pixel(ds, ARG(0), ARG(1)); break;
    case 3: r = (uint16_t)opeu_ui_highlight(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 4: b1_request_format(ds, args, count); break;
    default: port_unhandled("scenbrf", id, count);
    }
    return r;
}
