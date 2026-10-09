/* OSYSLIB (src/open/asm/osyslib.asm, OPEN.EXE _TEXT 1310..13F9): the
 * routines the opening has and MAIN's SYSLIB does not. The shared ones
 * (1310 install_dos_handlers, 132A fm_wait_start, 1337 fm_busy, 1352
 * fm_noop, 13D1 turn_input_b) are instruction-identical to MAIN's 402E..4073
 * and use port/services/syslib.c. Not called by OPEN's C, so not written:
 * 135C DOS getche, 1361/1366/137A the PC-98 / BIOS machine reset.
 *
 * The FM driver's two countdowns: F1h (counter B) is the timeline OPEN's
 * scenes wait on (fm_wait_start / fm_busy); F0h (counter A) is the one the
 * opening's uninterruptible delay uses (these two). Both run at 60.3 Hz in
 * the driver (port/services/fmdriver.c).
 */
#include "port.hpp"
#include "services.hpp"
#include "open-services.hpp"

/* 133E: INT 66h AX=F001h, BX=ticks. */
void opeu_open_sfx_play(uint16_t ticks)
{
    port_fm_function(0xf0, 0x01, ticks, 0, 0);
}

/* 134B: INT 66h AX=F000h; returns BX (xchg bx,ax). */
uint16_t opeu_open_sfx_busy(void)
{
    return port_fm_function(0xf0, 0x00, 0, 0, 0);
}

/* 13E0: AH=00h with DS:DX = the song bank (MAIN's 4074 without its second,
 * AH=10h effect-bank call). AX = 0 from the driver gives 1, anything else 0. */
int16_t opeu_open_sound_open(uint16_t offset, uint16_t segment)
{
    uint16_t ax = port_fm_function(0x00, 0, 0, offset, segment);
    if (ax != 0)
        ax = 0xffff;
    return (int16_t)(uint16_t)(ax + 1);
}
