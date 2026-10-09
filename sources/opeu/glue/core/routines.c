/* Address-keyed handlers of the native services (moved from the former
 * port/link/routines/native_E.c). The recovered code reaches these
 * assembly-library and C-runtime routines by their original address (battle
 * services, far code pointers in the data); each handler converts the words
 * of port.h's convention to the native service's parameters. Handlers of
 * recovered routines are generated into their game/ module (PORT_ROUTINE in
 * the native-build section, tools/game-services.json "routines"). */
#include "port.h"
#include "services.h"

#define ARG(i) ((uint16_t)((i) < count ? args[(i)] : 0))

PORT_ROUTINE(getch)
{
    (void)ds; (void)args; (void)count;
    return (uint16_t)port_getch();
}

PORT_ROUTINE(opeu_gfx_blit)
{
    (void)ds;
    return (uint16_t)opeu_gfx_blit(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
}

PORT_ROUTINE(opeu_isqrt)
{
    (void)ds;
    return opeu_isqrt(ARG(0));
}

PORT_ROUTINE(opeu_screen_copy)
{
    (void)ds;
    opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
    return 0;
}

PORT_ROUTINE(opeu_turn_input_b)
{
    (void)ds; (void)args; (void)count;
    return opeu_turn_input_b();
}

PORT_ROUTINE(opeu_ui_line)
{
    (void)ds;
    opeu_ui_line(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_vga_clear)
{
    (void)ds; (void)args; (void)count;
    return (uint16_t)opeu_vga_clear();
}

PORT_ROUTINE(opeu_vram_draw)
{
    opeu_vram_draw(ds + ARG(0), port_far(ARG(1), ARG(2)));
    return 0;
}

PORT_ROUTINE(opeu_vram_draw_near)
{
    opeu_vram_draw_near(ds + ARG(0), ds + ARG(1));
    return 0;
}

PORT_ROUTINE(opeu_vram_glyph)
{
    opeu_vram_glyph(ds + ARG(0), ds + ARG(1), ARG(2), ARG(3));
    return 0;
}

PORT_ROUTINE(opeu_vram_glyph_color)
{
    opeu_vram_glyph_color(ds + ARG(0), ds + ARG(1), ARG(2), ARG(3), ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_vram_load)
{
    opeu_vram_load(ds + ARG(0), port_far(ARG(1), ARG(2)));
    return 0;
}

PORT_ROUTINE(opeu_vram_restore)
{
    opeu_vram_restore(ARG(0), ARG(1), ds + ARG(2));
    return 0;
}

PORT_ROUTINE(opeu_vram_restore_odd)
{
    opeu_vram_restore_odd(ARG(0), ARG(1), ds + ARG(2));
    return 0;
}

PORT_ROUTINE(opeu_vram_save)
{
    opeu_vram_save(ARG(0), ARG(1), ds + ARG(2));
    return 0;
}

PORT_ROUTINE(opeu_vram_save_odd)
{
    opeu_vram_save_odd(ARG(0), ARG(1), ds + ARG(2));
    return 0;
}
