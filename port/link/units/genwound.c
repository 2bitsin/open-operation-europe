/* Link: units/genwound.c (17c4:086e general casualty). Services from the
 * host branch's list:
 *  0 person_lacks_side0_flag(person) 0b73:02bc -> byte
 *  1 request_format(buffer,format,person,text) 0480:0b9a
 *  2 initialize_deputy(person) 0b73:048a
 *  3 control_message(text,person) 1707:0672
 *  4 wait_confirm(value) 0541:011e
 *  5 set_turn_context(value) 0d65:00fe
 *  6 finish_side_initialization(value) 0d65:0032
 *  7 daily_show(id,side,0866h,17c4h) 1707:03e4; the presenter 17c4:0866
 *    (opeu_blast_pause) is reached by identifier through the dialogs
 *    service 47 (routines/blast_F.c). */
#include "units_F.h"

uint16_t link_genwound(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: r = opeu_person_lacks_side0_flag(ds, ARG(0)); break;
    case 1: r = lf_request_format(ds, args, count); break;
    case 2: opeu_initialize_deputy(ds, ARG(0)); break;
    case 3: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 4: opeu_wait_confirm(ds, link_keyboard, (int16_t)ARG(0)); break;
    case 5: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;
    case 6: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;
    case 7:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1),
                        (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    default: port_unhandled("genwound", id, count);
    }
    return r;
}
