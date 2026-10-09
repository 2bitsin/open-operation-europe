/* Link: core/appinit.c (0977:0372 app_startup, 0977:037c app_initialize,
 * the first-run input-device prompt). Events, from the host branch's
 * comment (src/main/core/appinit.c):
 *   0 startup service       06db:209e opeu_app_startup_service (STARTUP)
 *   1 VGA clear             0000:2aac opeu_vga_clear
 *   2 mouse window(x,y,w,h) 05a8:0210 opeu_mouse_window (MOUSE)
 *   3 input flush           0480:00a4 opeu_input_flush (TEXTOUT)
 *   4 text position(x,y)    0480:0808 opeu_army_position
 *   5 text(DS offset)       0480:0484 opeu_ui_text
 *   6 KEYBOARD() poll       05a8:0158 opeu_turn_input_a (any mouse button;
 *                           the DOS macro names are swapped, appinit.c:17-18)
 *   7 MOUSE() poll          0000:4066 opeu_turn_input_b (key waiting) */
#include "link.h"

uint16_t link_appinit(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_app_startup_service(ds, link_startup); break;
    case 1: opeu_vga_clear(); break;
    case 2:
        opeu_mouse_window(ds, link_mouse, (int16_t)ARG(0), (int16_t)ARG(1), (int16_t)ARG(2), (int16_t)ARG(3));
        break;
    case 3: opeu_input_flush(ds, link_textout_host()); break;
    case 4: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 5: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 6: return (uint16_t)opeu_turn_input_a(ds, link_mouse);
    case 7: return opeu_turn_input_b();
    default: port_unhandled("appinit", id, count);
    }
    return 0;
}
