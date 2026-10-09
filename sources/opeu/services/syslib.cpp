/* SYSLIB non-drawing routines (src/main/asm/syslib.asm 3FCE..409A).
 * The drawing half is port/services/syslib_gfx.c. */
#include "port.hpp"
#include "services.hpp"

/* 3FCE isqrt: the argument is the 16-bit word at [bp+6]. Bit 15 set returns
 * FFFFh. Otherwise eight rounds of the bitwise root on CX:DX with the trial
 * value in BX; the result is (BX >> 1) & 0FFh (AH cleared). */
uint16_t opeu_isqrt(uint32_t value) {
  uint16_t dx = (uint16_t)value, cx = 0, bx = 0, al;
  int i;
  if (dx & 0x8000) return 0xffff;
  for (i = 0; i < 8; i++) {
    cx = (uint16_t)(cx << 2 | dx >> 14);
    dx = (uint16_t)(dx << 2);
    bx = (uint16_t)(bx << 1 | 1);
    al = 2;
    if (cx < bx) {
      al = 0;                       /* borrow: CX is restored */
    } else {
      cx = (uint16_t)(cx - bx);
    }
    bx = (uint16_t)((bx & 0xff00) | ((bx | al) & 0xfe));
  }
  return (uint16_t)((bx >> 1) & 0xff);
}

/* 402E: INT 21h 2523h/2524h. Ctrl-Break and critical errors have no
 * counterpart on the host (file errors return DOS error codes instead). */
void koei_install_dos_handlers(void) { }

/* 4048: loads ES and restores it; no effect. */
void opeu_fm_noop(void) { }

/* 4052: INT 66h AX=F101h, BX=ticks. */
void opeu_fm_wait_start(uint16_t ticks) {
  port_fm_function(0xf1, 0x01, ticks, 0, 0);
}

/* 405F: INT 66h AX=F100h; returns BX (xchg ax,bx). */
uint16_t opeu_fm_busy(void) {
  return port_fm_function(0xf1, 0x00, 0, 0, 0);
}

/* 4066: INT 16h AH=01h; 1 when a key is waiting. */
uint16_t opeu_turn_input_b(void) {
  return port_key_available() ? 1 : 0;
}

/* 4074: AH=00h with DS:DX = driver data; if AX=0, AH=10h with DS:DX = effect
 * data. Returns AX+1 where AX is 0 when both calls returned 0 and FFFFh
 * otherwise: 1 = driver open, 0 = failed. */
int16_t opeu_sound_open(uint16_t driver_off, uint16_t driver_seg, uint16_t effects_off, uint16_t effects_seg) {
  uint16_t ax = port_fm_function(0x00, 0, 0, driver_off, driver_seg);
  if (ax == 0) ax = port_fm_function(0x10, 0, 0, effects_off, effects_seg);
  if (ax != 0) ax = 0xffff;
  return (int16_t)(uint16_t)(ax + 1);
}
