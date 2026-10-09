/* Native services: hand-written C replacements for what the original did in
 * assembly (src/main/asm: FILEIO, VGALIB, SYSLIB, EMSDRV; PASSWORD is
 * never called by MAIN) and through DOS/BIOS/driver interrupts from C
 * (int86 in MOUSE, KEYBOARD/TEXTOUT, SOUND, STARTUP, RESOPEN; getch; exit).
 *
 * The assembly routines keep their original names and argument order; near
 * pointers become host pointers into port_ds, far segment:offset pairs
 * become host pointers from port_far() unless stated. The interrupt-level
 * services are named by what they do; each link adapter that receives a
 * module's int86 boundary calls the matching function for the one service
 * function that call site issues.
 */
#ifndef PORT_SERVICES_H
#define PORT_SERVICES_H

#include <stdint.h>
#include "vgalib.h"       /* VGALIB: planar 640x480x16 drawing (port/services/vgalib.c) */
#include "syslib_gfx.h"   /* SYSLIB drawing: battle cells, sprite compose */

/* ---- FILEIO (src/main/asm/fileio.asm), DOS handles over stdio ----------- */
int16_t  opeu_dos_open(const char *name, uint16_t mode);      /* handle or -1 */
int16_t  opeu_dos_close(int16_t handle);
int16_t  opeu_dos_read(int16_t handle, uint8_t *data, uint16_t size);   /* bytes */
int16_t  opeu_dos_read_far(int16_t handle, uint16_t segment, uint16_t offset, uint16_t size);
int16_t  opeu_dos_write(int16_t handle, const uint8_t *data, uint16_t size);
int16_t  opeu_dos_write_far(int16_t handle, uint16_t segment, uint16_t offset, uint16_t size);
int32_t  opeu_dos_seek(int16_t handle, int32_t position, int16_t origin);
int16_t  opeu_dos_create(const char *name);                    /* handle or -1 */
int16_t  opeu_dos_exists(const char *name);
uint16_t opeu_dos_alloc(uint16_t paragraphs);                  /* segment or 0 */
int16_t  opeu_dos_free(uint16_t segment);

/* ---- SYSLIB non-drawing routines (src/main/asm/syslib.asm) -------------- */
uint16_t opeu_isqrt(uint32_t value);
void     koei_install_dos_handlers(void);      /* no-op: break/critical-error handlers */
void     opeu_fm_noop(void);
void     opeu_fm_wait_start(uint16_t ticks);   /* INT 66h F101h: start the 60.3 Hz countdown */
uint16_t opeu_fm_busy(void);                   /* INT 66h F100h: remaining countdown ticks */
uint16_t opeu_turn_input_b(void);              /* INT 16h AH=1: 1 when a key is waiting */
int16_t  opeu_sound_open(uint16_t driver_off, uint16_t driver_seg, uint16_t effects_off, uint16_t effects_seg);

/* ---- EMSDRV (src/main/asm/emsdrv.asm): no expanded memory manager ------- */
int16_t opeu_ems_present(void);                /* 0: no EMM */
int16_t opeu_ems_status(void);
int16_t opeu_ems_version(void);
int16_t opeu_ems_page_count(int16_t *count);
int16_t opeu_ems_free_pages(int16_t *total, int16_t *available);
int16_t opeu_ems_frame(uint16_t *segment);
int16_t opeu_ems_allocate(int16_t pages, int16_t *handle);
int16_t opeu_ems_page_array(uint8_t *pages);
int16_t opeu_ems_map(int16_t handle, uint16_t physical, int16_t logical);
int16_t opeu_ems_release(int16_t handle);

/* ---- BIOS keyboard (INT 16h), as KEYBOARD/TEXTOUT use it ---------------- */
uint16_t port_key_available(void);             /* AH=01h: 1 if a key is queued */
uint16_t port_key_read(void);                  /* AH=00h: AH=scan, AL=ASCII (blocks, pumping) */

/* ---- mouse driver (INT 33h), the functions MOUSE issues ----------------- */
/* Each takes/returns the register words of that function; y is the
 * driver's (screen) coordinate, the game band starts at y = 40. */
uint16_t port_mouse_reset(void);               /* AX=0: returns AX (FFFF installed) */
void     port_mouse_show(void);                /* AX=1 */
void     port_mouse_hide(void);                /* AX=2 */
void     port_mouse_state(uint16_t *buttons, uint16_t *x, uint16_t *y);  /* AX=3: BX, CX, DX */
void     port_mouse_set_position(uint16_t x, uint16_t y);               /* AX=4 */
void     port_mouse_x_range(uint16_t min, uint16_t max);                 /* AX=7 */
void     port_mouse_y_range(uint16_t min, uint16_t max);                 /* AX=8 */

/* ---- FM sound driver (INT 66h), silent with a clear OPL hook ----------- */
/* Music/effect requests are recorded (port_fm_*) and answered as an idle
 * driver: status queries return AL=0 so the original busy-waits end. */
uint16_t port_fm_function(uint8_t ah, uint8_t al, uint16_t bx, uint16_t dx_off, uint16_t ds_seg);

/* ---- BIOS clock (INT 1Ah AH=0) and DOS odds and ends -------------------- */
uint16_t port_clock_ticks_low(void);           /* DX of INT 1Ah AH=0 */
void     port_disk_reset(void);                /* INT 21h AH=0Dh: flush open files */
int      port_getch(void);                     /* C runtime getch() */

/* ---- presentation / pacing --------------------------------------------- */
/* Pump host events, present the frame if due (about 60 Hz), and in
 * headless runs advance virtual time. Every polling service calls it. */
void port_pump(void);

#endif
