/* Service table shared by the 23b2 air-force modules (airbomb, airdrop,
 * airforce, airhelp, airlift): every one documents the same ids (host
 * branch comments of airbomb.c, airhelp.c, airdrop.c, airforce.c):
 *  0 request_format(dest,format,...)   0480:0b9a
 *  1 control_message(text,person)      1707:0672
 *  2 control_confirm(text,person)      1707:076c (= confirm_message)
 *  3 message_lookup(id)                312c:000c
 *  4 army_print(format,...)            0480:0bb8 (= cursor_print)
 *  5 speaker_panel(person,show)        1707:04d4
 *  6 text_feed(text)                   1707:0652
 *  7 choose_value(first,last,cancel,items) 0541:0238
 *  8 delay_dialog(text,x,person,menu)  1707:0726
 *  9 army_title(text)                  1707:03aa
 * 10 map_pick_location(x addr,y addr)  17c4:0274
 * 11 list_dialog(window,limit,flags)   0a8e:0bc0
 * 12 army_position(x,y)                0480:0808
 * 13 daily_context(side)               1389:000e
 * 14 far action (segment, offset) of the DGAIR mission table (DS:a62e)
 * Conversion only; no game logic. */
#ifndef PORT_LINK_CORE_C2_AIR_H
#define PORT_LINK_CORE_C2_AIR_H

#include "c2_common.h"

/* Modeled stack reserve for a call into a routine with an `sp`/`bp`
 * parameter: covers the deepest documented frame of the air modules
 * (objective_damage's BP-0x9e buffers plus its callee frames, airbomb's
 * 0x6e-byte frame plus nested 23b2 calls). */
#define C2_AIR_FRAME 0x200u

/* Entry SP (far return-address slot) for an `sp` parameter. */
static inline uint16_t c2_air_sp_enter(void)
{
    return port_stack_enter(C2_AIR_FRAME);
}

/* BP of a far-called routine with a standard frame (entry SP - 2), for a
 * `bp`/`frame` parameter. */
static inline uint16_t c2_air_bp_enter(void)
{
    return (uint16_t)(port_stack_enter(C2_AIR_FRAME) - 2u);
}

static inline void c2_air_leave(void)
{
    port_stack_leave(C2_AIR_FRAME);
}

static inline uint16_t c2_air_23b2(uint8_t *ds, const char *module, uint16_t id,
                                   const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, frame, scratch;
    switch (id) {
    case 0: c2_request_format(ds, args, count); break;
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 2: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 3: r = opeu_message_lookup(ds, link_msglook, (short)ARG(0)); break;
    case 4: c2_cursor_print(ds, args, count); break;
    case 5: r = opeu_speaker_panel(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 6: opeu_text_feed(ds, link_dialogs_host(), ARG(0)); break;
    case 7:
        scratch = c2_scratch_enter(0x40);
        r = opeu_choose_value(ds, link_keyboard, ARG(0), ARG(1), (int)(int16_t)ARG(2), ARG(3), scratch);
        c2_scratch_leave(0x40);
        break;
    case 8: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 9: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;
    case 10:
        frame = c2_frame_enter();
        r = opeu_map_pick_location(ds, link_mappick, frame, ARG(0), ARG(1));
        c2_frame_leave();
        break;
    case 11: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 12: opeu_army_position(ds, link_textout_host(), (short)ARG(0), (short)ARG(1)); break;
    case 13: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;
    case 14: r = (uint16_t)port_call(ds, (uint32_t)ARG(0) * 16u + ARG(1), 0, 0); break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

#endif
