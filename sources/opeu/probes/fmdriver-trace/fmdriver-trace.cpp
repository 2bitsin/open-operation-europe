/* Trace runner for the native FM driver (port/services/fmdriver.c).
 *
 * Reads a command script on stdin and prints every OPL register write and
 * every call result on stdout, in the form port/tests/fmdriver_diff.py
 * compares with the original FMDRV.COM run under the Unicorn harness of
 * tests/validate_fmdrv.py. Script lines (numbers in hex):
 *
 *   machine <opl_present>        fresh banks and driver; chip present or not
 *   mem <seg> <off> <hexbytes>   bank bytes at seg:off (a 64 KiB block per
 *                                segment, at most four; elsewhere reads 0)
 *   install                      install with chip detection
 *   uninstall                    INT 66h AH=FFh (the installer's OFF)
 *   int <ax> <bx> <dx> <ds>      INT 66h call  -> "ret ax bx dx ds"
 *   far <ax> <bx> <dx> <ds>      far-call entry -> "ret ax bx dx ds"
 *   tick <n>                     n PIT ticks, each printed as "tick"
 *   callbacks                    -> "callbacks <n>" (client callback count)
 *   none                         no driver activity (an installer run that
 *                                only reports, e.g. "already installed")
 *
 * Output: "> <command>" when a command starts (one "> tick" per tick),
 * "opl <reg> <value>" per register write, in order.
 */
#include "fmdriver.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The caller's song/effect banks: up to four 64 KiB blocks, each named by
 * the segment it starts at. */
typedef struct {
    int used;
    uint16_t seg;
    uint8_t bytes[0x10000];
} Bank;
static Bank banks[4];
static FmDriver driver;
static int opl_present;
static uint8_t status;
static unsigned callbacks;

static void write_register(void *user, uint8_t reg, uint8_t value)
{
    (void)user;
    printf("opl %02x %02x\n", reg, value);
    /* The validator's chip model: timer 1 overflows during the detection
     * delay; an IRQ reset (bit 7) clears the status. */
    if (opl_present && reg == 4) {
        if (value & 0x80)
            status = 0;
        else if (value & 1)
            status = 0xc0;
    }
}

static uint8_t read_status(void *user)
{
    (void)user;
    return status;
}

static uint8_t peek(void *user, uint16_t seg, uint16_t off)
{
    uint32_t at = (uint32_t)seg * 16 + off;
    const Bank *best = NULL;
    int i;
    (void)user;
    /* blocks may overlap (banks 800h paragraphs apart): the nearest one
     * starting at or below the address holds it */
    for (i = 0; i < 4; i++) {
        uint32_t base = (uint32_t)banks[i].seg * 16;
        if (banks[i].used && at >= base && at - base < 0x10000 && (!best || banks[i].seg > best->seg))
            best = &banks[i];
    }
    return best ? best->bytes[at - (uint32_t)best->seg * 16] : 0;
}

static Bank *bank_of(uint16_t seg)
{
    int i;
    for (i = 0; i < 4; i++)
        if (banks[i].used && banks[i].seg == seg)
            return &banks[i];
    for (i = 0; i < 4; i++)
        if (!banks[i].used) {
            banks[i].used = 1;
            banks[i].seg = seg;
            return &banks[i];
        }
    return NULL;
}

static void callback(void *user, uint16_t seg, uint16_t off, FmdCallbackRegs *regs)
{
    (void)user;
    (void)seg;
    (void)off;
    (void)regs;                 /* the validator's callback only counts */
    callbacks++;
}

static int hex(const char *s, unsigned *out)
{
    char *end;
    unsigned long v = strtoul(s, &end, 16);
    if (end == s)
        return 0;
    *out = (unsigned)v;
    return 1;
}

/* main.cpp calls this. */
int opeu_fmdriver_trace(void)
{
    char line[1 << 16];
    FmdHost host;
    host.user = NULL;
    host.write_register = write_register;
    host.read_status = read_status;
    host.peek = peek;
    host.callback = callback;
    while (fgets(line, sizeof line, stdin)) {
        char verb[32];
        unsigned a = 0, b = 0, c = 0, e = 0;
        int n = 0;
        if (sscanf(line, "%31s%n", verb, &n) != 1)
            continue;
        if (strcmp(verb, "mem") && strcmp(verb, "tick"))
            printf("> %s\n", verb);
        if (!strcmp(verb, "machine")) {
            sscanf(line + n, "%x", &a);
            memset(banks, 0, sizeof banks);
            memset(&driver, 0, sizeof driver);
            opl_present = (int)a;
            status = 0;
            callbacks = 0;
        } else if (!strcmp(verb, "mem")) {
            static char bytes[1 << 16];
            Bank *bank;
            size_t i;
            if (sscanf(line + n, "%x %x %65535s", &a, &b, bytes) != 3)
                return 2;
            bank = bank_of((uint16_t)a);
            if (!bank)
                return 2;
            for (i = 0; bytes[i] && bytes[i + 1]; i += 2) {
                char pair[3] = {bytes[i], bytes[i + 1], 0};
                unsigned v;
                if (!hex(pair, &v))
                    return 2;
                bank->bytes[(uint16_t)(b + i / 2)] = (uint8_t)v;
            }
        } else if (!strcmp(verb, "install")) {
            fmd_install(&driver, &host, 1);
        } else if (!strcmp(verb, "uninstall")) {
            fmd_uninstall(&driver);
        } else if (!strcmp(verb, "int") || !strcmp(verb, "far")) {
            FmdRegs r;
            if (sscanf(line + n, "%x %x %x %x", &a, &b, &c, &e) != 4)
                return 2;
            r.ax = (uint16_t)a;
            r.bx = (uint16_t)b;
            r.dx = (uint16_t)c;
            r.ds = (uint16_t)e;
            if (verb[0] == 'i')
                fmd_interrupt(&driver, &r);
            else
                fmd_far_call(&driver, &r);
            printf("ret %04x %04x %04x %04x\n", r.ax, r.bx, r.dx, r.ds);
        } else if (!strcmp(verb, "tick")) {
            unsigned i;
            sscanf(line + n, "%x", &a);
            for (i = 0; i < a; i++) {
                printf("> tick\n");
                fmd_timer_tick(&driver);
            }
        } else if (!strcmp(verb, "none")) {
        } else if (!strcmp(verb, "callbacks")) {
            printf("callbacks %x\n", callbacks);
        } else {
            fprintf(stderr, "fmdriver_trace: unknown command %s\n", verb);
            return 2;
        }
    }
    return 0;
}
