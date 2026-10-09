/* Link: core/setmark.c (06db:0006/0074 setup-screen marker show/hide).
 * Services, from the host branch (original argument words):
 * 0 bitmap save (x, y, w, h, segment, offset)        -> VGALIB 2ED8
 * 1 far blit (x, y, w, h, mode, segment, offset)     -> VGALIB 27CA
 * 2 bitmap restore (x, y, w, h, mode, segment, offset) -> VGALIB 2674 */
#include "b2_common.h"

uint16_t link_setmark(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    (void)ds;
    switch (id) {
    case 0: opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), B2_FAR(ARG(4), ARG(5))); break;
    case 1: opeu_blit_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), B2_FAR(ARG(5), ARG(6))); break;
    case 2: opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), B2_FAR(ARG(5), ARG(6))); break;
    default: port_unhandled("setmark", id, count);
    }
    return 0;
}
