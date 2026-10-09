/* The request-menu service table shared by the 22f9 modules (rqcommon,
 * rqmenu, rqsupply; the ids and argument packing are those of their host
 * branches and of tests/validate_request_menu.py SERVICES). Only the ids
 * the C2-owned modules raise are linked here; each module's link callback
 * passes its own name for diagnostics. Conversion only. */
#ifndef PORT_LINK_CORE_C2_RQ_H
#define PORT_LINK_CORE_C2_RQ_H

#include "c2_common.h"

/* Callee scratch: frame buffers at scratch+4 (at most 100 bytes). */
#define C2_RQ_SCRATCH 0x80u

static inline uint16_t c2_rq_service(const char *module, uint8_t *ds, uint16_t id,
                                     const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, scratch, frame;
    switch (id) {
    case 3: r = opeu_request_pending(ds, (uint8_t)ARG(0)); break;                  /* 22f9:0036 */
    case 4:                                                           /* 22f9:0072 */
        scratch = c2_scratch_enter(C2_RQ_SCRATCH);
        r = opeu_request_cancel_prompt(ds, link_rqcommon, scratch, ARG(0));
        c2_scratch_leave(C2_RQ_SCRATCH);
        break;
    case 6:                                                           /* 22f9:00a2 */
        /* The original passes the address of its 4-byte request local
         * (target word, argument byte, kind byte). */
        scratch = c2_scratch_enter(4);
        ds[scratch] = (uint8_t)ARG(0);
        ds[(uint16_t)(scratch + 1)] = (uint8_t)(ARG(0) >> 8);
        ds[(uint16_t)(scratch + 2)] = (uint8_t)ARG(1);
        ds[(uint16_t)(scratch + 3)] = (uint8_t)ARG(2);
        opeu_request_file(ds, link_rqcommon, scratch);
        c2_scratch_leave(4);
        break;
    case 10: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    case 16: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;              /* 1707:03aa */
    case 17:                                                          /* 1707:0726 */
        r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3));
        break;
    case 19: c2_request_format(ds, args, count); break;                            /* 0480:0b9a */
    case 20: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:076c */
    case 21: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;        /* 0a8e:005c */
    case 22:                                                          /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), ARG32(2));  /* segment<<16 | offset */
        break;
    case 23:                                                          /* 17c4:0274 */
        frame = c2_frame_enter();
        r = opeu_map_pick_location(ds, link_mappick, frame, ARG(0), ARG(1));
        c2_frame_leave();
        break;
    case 26: opeu_army_position(ds, link_textout_host(), (short)ARG(0), (short)ARG(1)); break; /* 0480:0808 */
    case 27: c2_cursor_print(ds, args, count); break;                              /* 0480:0bb8 */
    case 28: r = opeu_request_mode_is(ds, (uint8_t)ARG(0)); break;                 /* 22f9:0052 */
    case 29: r = opeu_side_has_city(ds); break;                                    /* 22f9:00f6 */
    case 30:                                                          /* far handler (offset, segment) */
        r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), 0, 0);
        break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

#endif
