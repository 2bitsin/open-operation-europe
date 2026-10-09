/* INT 66h: the KOEI FM driver (FMDRV, src/fmdrv/driver.asm) as MAIN uses
 * it, "present but silent" (platform-surface.md 4.4, design A).
 *
 *  AH=00h  set music data (DS:DX)      -> AX=0
 *  AH=10h  set effect data (DS:DX)     -> AX=0
 *  AH=01h  music start (AL=song)       recorded, no sound
 *  AH=02h  music stop/fade (AL=speed)  recorded
 *  AH=04h  loop flag (AL)              recorded
 *  AH=07h  effect voice key-off        recorded
 *  AH=11h  effect start (AL=effect+1)  recorded
 *  AH=12h  effect stop                 recorded
 *  AH=03h  music status                -> AL=0 (idle), BX=0
 *  AH=13h  effect status               -> AL=0 (idle)
 *  AX=F0xxh/F1xxh countdowns 0/1: AL=0 returns BX=remaining, AL!=0 sets
 *          the countdown to BX. They tick at the driver's rate: the PIT runs
 *          at 1193182/4096 = 291.3 Hz and each countdown accumulator adds
 *          35h/256 per PIT tick (driver.asm 01fc..021e), about 60.3 Hz.
 *          Setting a countdown clears its accumulator.
 * Other functions (FE version, FF uninstall, 05/06/08/09/80h..) are not
 * issued by MAIN and return AX unchanged (0 here).
 *
 * OPL HOOK: every music/effect request is stored in port_fm_requests below
 * (latest song/effect, loop flag, data pointers, request counters). A future
 * OPL emulator plays them from there: give it the FMUSIC data at
 * port_fm_requests.music_seg:music_off (3D91:0000, 7BBAh bytes) and the
 * effect data at effect_seg:effect_off (3D91:7BBA), feed plat_audio_open,
 * and make AH=03h/13h report the playing song/effect (AL!=0) instead of 0.
 */
#include "port.h"
#include "services.h"
#include "hostinput.h"

/* ---- OPL hook: the recorded driver requests ---------------------------- */
typedef struct {
    uint16_t music_off, music_seg;      /* AH=00h data pointer */
    uint16_t effect_off, effect_seg;    /* AH=10h data pointer */
    uint8_t song;                       /* AH=01h AL: last requested song (0 = silence) */
    uint8_t loop;                       /* AH=04h AL */
    uint8_t fade;                       /* AH=02h AL of the last stop */
    uint8_t effect;                     /* AH=11h AL: last effect + 1 */
    unsigned starts, stops, effect_starts, effect_stops, key_offs;
} PortFmRequests;

PortFmRequests port_fm_requests;

/* ---- countdowns ---------------------------------------------------------- */
static struct {
    uint16_t value;
    uint64_t set_us;
} countdown[2];

/* PIT ticks since the countdown was set times 53/256, without overflow. */
static uint16_t countdown_remaining(int n)
{
    uint64_t elapsed = port_time_us() - countdown[n].set_us;
    /* pit ticks = elapsed_us * 1193182 / (4096 * 1e6) */
    uint64_t pit = elapsed * 1193182u / 4096000000u;
    uint64_t dec = pit * 53u / 256u;
    if (dec >= countdown[n].value)
        return 0;
    return (uint16_t)(countdown[n].value - dec);
}

uint16_t port_fm_function(uint8_t ah, uint8_t al, uint16_t bx, uint16_t dx_off, uint16_t ds_seg)
{
    uint16_t r = 0;
    switch (ah) {
    case 0xf0:
    case 0xf1: {
        int n = ah & 1;
        if (al == 0) {
            r = countdown_remaining(n);
            if (r)
                port_pump();          /* every busy-wait polls here */
        } else {
            countdown[n].value = bx;
            countdown[n].set_us = port_time_us();
            r = 0;
        }
        return r;
    }
    case 0x00:
        port_fm_requests.music_off = dx_off;
        port_fm_requests.music_seg = ds_seg;
        break;
    case 0x10:
        port_fm_requests.effect_off = dx_off;
        port_fm_requests.effect_seg = ds_seg;
        break;
    case 0x01:
        port_fm_requests.song = al;
        port_fm_requests.starts++;
        break;
    case 0x02:
        port_fm_requests.fade = al;
        port_fm_requests.stops++;
        break;
    case 0x04:
        port_fm_requests.loop = al;
        break;
    case 0x07:
        port_fm_requests.key_offs++;
        break;
    case 0x11:
        port_fm_requests.effect = al;
        port_fm_requests.effect_starts++;
        break;
    case 0x12:
        port_fm_requests.effect_stops++;
        break;
    case 0x03:
    case 0x13:
        /* idle; the callers busy-wait on these, so pump the host */
        port_pump();
        r = 0;
        break;
    default:
        break;
    }
    port_trace("fm", "int66 ah=%02x al=%02x bx=%04x dx=%04x ds=%04x -> %04x", ah, al, bx, dx_off, ds_seg, r);
    return r;
}
