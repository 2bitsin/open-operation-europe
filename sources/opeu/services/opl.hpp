/* The YM3812 (OPL2) chip as a native audio device: ymfm's emulation of the
 * sound chip (port/third_party/ymfm, BSD-3-Clause) behind a C interface.
 * The FM driver (fmdriver.c) writes registers; the audio path pulls
 * samples at the chip's own rate (3579545 Hz / 72 = 49716 Hz, mono). */
#pragma once

#include <stdint.h>

inline constexpr auto OPL_CLOCK{ 3579545u };
inline constexpr auto OPL_SAMPLE_RATE{ 49716 }; /* OPL_CLOCK / 72, rounded */

void opl_reset(void);
/* One register write (address port 388h, then data port 389h). */
void opl_write(uint8_t reg, uint8_t value);
/* Render `frames` samples as interleaved stereo int16 (both channels carry
 * the chip's mono output). */
void opl_render(int16_t* stereo, int frames);
