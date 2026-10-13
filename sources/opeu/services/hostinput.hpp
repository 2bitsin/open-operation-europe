/* Internal interface between the event pump (port/core/pump.c) and the
 * keyboard/mouse driver state (port/services/input.c). */
#pragma once

#include <stdint.h>

/* A key press as the BIOS keyboard interrupt stores it: AH=scan, AL=ASCII. */
void PortInputKey(uint8_t scan, uint8_t ascii);
/* Host pointer position in screen pixels (0..639, 0..479). */
void PortInputMouseMove(int x, int y);
/* Button 0 left, 1 right; down 1/0. */
void PortInputMouseButton(int button, int down, int x, int y);

/* Overlay the driver's pointer on a rendered 640x480 frame of DAC indices
 * while it is shown; `pal` (256 RGB triples) receives the two pointer
 * colours. Returns 1 if a pointer was drawn. */
int PortInputDrawPointer(uint8_t* frame, uint8_t pal_rgb8[768]);
/* Changes since the last call that need a new frame (pointer moved/shown). */
int PortInputPointerDirty(void);

/* Monotonic time base of the services: host time, or virtual time when
 * headless (advanced by the pump). */
uint64_t port_time_us(void);
