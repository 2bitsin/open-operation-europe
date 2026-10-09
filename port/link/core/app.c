/* Link: core/app.c (0977:03dc application entry). Services, from the host
 * branch's event list: initialize=0, startup=1, turn=2, notice
 * begin/one/other/end=3..6, victory=7 (word result), finish=8. */
#include "link.h"

uint16_t link_app(uint8_t *ds, uint16_t id)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: opeu_app_initialize(ds, link_appinit); break;           /* 0977:037c */
    case 1: opeu_app_startup(ds, link_appinit); break;              /* 0977:0372 */
    case 2: opeu_process_selected_turn(ds, link_dispatch); break;   /* 1389:0ba8 */
    case 3:                                                          /* 33ad:0766 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_notice_begin(ds, link_notice, sp);
        port_stack_leave(PORT_FRAME);
        break;
    case 4: opeu_notice_one(ds, link_btlsetup); break;              /* 2673:1924 */
    case 5: opeu_notice_other(ds, link_btlend); break;              /* 2fe9:1420 */
    case 6:                                                          /* 3424:06be */
        sp = port_stack_enter(PORT_FRAME);
        opeu_notice_end(ds, link_notend, sp);
        port_stack_leave(PORT_FRAME);
        break;
    case 7: r = opeu_check_victory(ds); break;                       /* 0977:0296 */
    case 8:                                                          /* 090d:06a8 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_app_finish(ds, link_scenend, sp);
        port_stack_leave(PORT_FRAME);
        break;
    default: port_unhandled("app", id, 0);
    }
    return r;
}
