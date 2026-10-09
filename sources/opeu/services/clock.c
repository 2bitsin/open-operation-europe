/* BIOS clock (INT 1Ah AH=00h) and the C runtime getch().
 * (INT 21h AH=0Dh disk reset lives in fileio.c next to the handle table.) */
#include "port.h"
#include "services.h"
#include "hostinput.h"

#include <time.h>

/* DX of INT 1Ah AH=0: low word of the timer ticks since midnight
 * (1193182/65536 = 18.2065 Hz). Window runs count from the local midnight;
 * headless runs from virtual time 0, so they are reproducible. */
uint16_t port_clock_ticks_low(void)
{
    uint64_t us = port_time_us();
    uint64_t ticks;
    if (!port_options.headless) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        if (t)
            us = (uint64_t)(t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec) * 1000000u + us % 1000000u;
    }
    ticks = us * 1193182u / 65536u / 1000000u;
    port_trace("svc", "int1a ticks -> %04x", (unsigned)(uint16_t)ticks);
    return (uint16_t)ticks;
}

/* MS C getch(): INT 21h AH=08h, the ASCII of the next key; a key with
 * ASCII 0 returns 0 and its scan code on the next call. */
int port_getch(void)
{
    static int pending = -1;
    uint16_t k;
    if (pending >= 0) {
        int r = pending;
        pending = -1;
        return r;
    }
    k = port_key_read();
    if ((k & 0xff) == 0) {
        pending = k >> 8;
        return 0;
    }
    return k & 0xff;
}
