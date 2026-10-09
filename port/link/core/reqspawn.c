/* Link: core/reqspawn.c (1270:0876 request action 2, request unit).
 * Services (host branch macros, word arguments a,b,c,d):
 * 1 arrival_prepare(unit) 15c4:0fd0, 2 set_turn_context(value) 0d65:00fe,
 * 3 arrival_location(x,y,side) 15c4:1408, 4 arrival_view(unit) 15c4:0808,
 * 5 daily_show(id, side, present offset, present segment) 1707:03e4,
 * 6 finish_side_initialization(value) 0d65:0032. */
#include "c2_common.h"

uint16_t link_reqspawn(uint8_t *ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    uint16_t frame;
    switch (id) {
    case 1:                                                           /* 15c4:0fd0 */
        frame = c2_frame_enter();
        opeu_arrival_prepare(ds, link_maperase, frame, a);
        c2_frame_leave();
        break;
    case 2: opeu_set_turn_context(ds, link_sound, (uint8_t)a); break;            /* 0d65:00fe */
    case 3:                                                           /* 15c4:1408 */
        frame = c2_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)a, (uint8_t)b, (uint8_t)c);
        c2_frame_leave();
        break;
    case 4:                                                           /* 15c4:0808 */
        frame = c2_frame_enter();
        opeu_arrival_view(ds, link_mapunit, frame, a);
        c2_frame_leave();
        break;
    case 5:                                                           /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c);
        break;
    case 6: opeu_finish_side_initialization(ds, link_sound, (uint8_t)a); break;  /* 0d65:0032 */
    default: port_unhandled("reqspawn", id, 4);
    }
    return 0;
}
