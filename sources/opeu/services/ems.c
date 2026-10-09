/* EMSDRV (src/main/asm/emsdrv.asm, 069D:000A..00F5) with no expanded
 * memory manager installed.
 *
 * present() compares the INT 67h vector's device name with "EMMXXXX0"; with
 * no EMM it returns 0, and core/emsmap.c's detection (06ac:01fc) stops right
 * there, so the game allocates every buffer in DOS memory (the arena of
 * port/core/memory.c). The other entries are unreachable after a failed
 * detection; should one be called anyway, it answers like a missing
 * manager: EMM status 80h (internal error) in AX, outputs untouched.
 */
#include "port.h"
#include "services.h"

#define EMM_FAILURE 0x80

int16_t opeu_ems_present(void) { return 0; }
int16_t opeu_ems_status(void) { return EMM_FAILURE; }
int16_t opeu_ems_version(void) { return EMM_FAILURE; }
int16_t opeu_ems_page_count(int16_t *count) { (void)count; return EMM_FAILURE; }
int16_t opeu_ems_free_pages(int16_t *total, int16_t *available) { (void)total; (void)available; return EMM_FAILURE; }
int16_t opeu_ems_frame(uint16_t *segment) { (void)segment; return EMM_FAILURE; }
int16_t opeu_ems_allocate(int16_t pages, int16_t *handle) { (void)pages; (void)handle; return EMM_FAILURE; }
int16_t opeu_ems_page_array(uint8_t *pages) { (void)pages; return EMM_FAILURE; }
int16_t opeu_ems_map(int16_t handle, uint16_t physical, int16_t logical)
{
    (void)handle; (void)physical; (void)logical;
    return EMM_FAILURE;
}
int16_t opeu_ems_release(int16_t handle) { (void)handle; return EMM_FAILURE; }
