/* Assembly-library and C-runtime routines reached by address from the battle
 * modules; they run as the native services of port/services. All are cdecl:
 * the words are the pushed arguments in order. Near pointers are DS
 * offsets, far ones (segment, offset) pairs as the callers push them.
 *   VGALIB 2a96 gfx_blit (x, y, w, h, mode, segment, offset)
 *          2aac vga_clear ()      2d16 ui_line (x1, y1, x2, y2, color)
 *          301f screen_copy (x1, y1, w, h, x2, y2)
 *   SYSLIB 34b8/354f vram_restore(_odd) (x, y, vram)
 *          37e4/3882 vram_save(_odd) (x, y, vram)
 *          3a04 vram_draw (vram, source segment, source offset)
 *          3a2a vram_draw_near (vram, source)
 *          3a4d vram_glyph_color (vram, glyph, x, y, color)
 *          3ac3 vram_glyph (vram, glyph, x, y)
 *          3b1e vram_load (vram, mask segment, mask offset)
 *          3fce isqrt (value) -> AX          4066 turn_input_b () -> AX
 *   C runtime 4320 getch () -> AX */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_gfx_blit)
{
    (void)ds;
    return (uint16_t)opeu_gfx_blit(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
}

PORT_ROUTINE(opeu_vga_clear)
{
    (void)ds; (void)args; (void)count;
    return (uint16_t)opeu_vga_clear();
}

PORT_ROUTINE(opeu_ui_line)
{
    (void)ds;
    opeu_ui_line(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_screen_copy)
{
    (void)ds;
    opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
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

PORT_ROUTINE(opeu_vram_glyph_color)
{
    opeu_vram_glyph_color(ds + ARG(0), ds + ARG(1), ARG(2), ARG(3), ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_vram_glyph)
{
    opeu_vram_glyph(ds + ARG(0), ds + ARG(1), ARG(2), ARG(3));
    return 0;
}

PORT_ROUTINE(opeu_vram_load)
{
    opeu_vram_load(ds + ARG(0), port_far(ARG(1), ARG(2)));
    return 0;
}

PORT_ROUTINE(opeu_isqrt)
{
    (void)ds;
    return opeu_isqrt(ARG(0));
}

PORT_ROUTINE(opeu_turn_input_b)
{
    (void)ds; (void)args; (void)count;
    return opeu_turn_input_b();
}

PORT_ROUTINE(getch)
{
    (void)ds; (void)args; (void)count;
    return (uint16_t)port_getch();
}
