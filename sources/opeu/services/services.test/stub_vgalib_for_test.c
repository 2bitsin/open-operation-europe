/* Minimal stand-ins for vgalib.c so test_syslib_gfx.c builds on its own. */
#include "vgalib.h"
uint8_t vga_plane[4][VGA_PLANE_SIZE] = {{0}};
int vga_dirty;
void vga_reset_gc(void) {}
