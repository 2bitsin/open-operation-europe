/* Routine link table lookup and diagnostics. */
#include "port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const PortLink port_links[];
extern const unsigned port_link_count;

static const PortLink *find(uint32_t address)
{
    unsigned lo = 0, hi = port_link_count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (port_links[mid].address < address)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < port_link_count && port_links[lo].address == address)
        return &port_links[lo];
    return NULL;
}

const char *port_routine_name(uint32_t address)
{
    const PortLink *l = find(address);
    return l ? l->name : NULL;
}

uint32_t port_routine_address(const char *name)
{
    unsigned i;
    for (i = 0; i < port_link_count; i++)
        if (strcmp(port_links[i].name, name) == 0)
            return port_links[i].address;
    return 0;
}

uint32_t port_call(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    const PortLink *l = find(address);
    uint32_t r;
    if (!l || !l->routine)
        port_fatal("call to unlinked routine %s (%05x) with %u args", l ? l->name : "?", (unsigned)address, count);
    if (port_trace_enabled("link")) {
        char buf[160];
        int n = 0;
        unsigned i;
        for (i = 0; i < count && n < 140; i++)
            n += snprintf(buf + n, sizeof buf - (size_t)n, " %04x", args[i]);
        buf[n] = 0;
        port_trace("link", "-> %s%s", l->name, buf);
    }
    r = l->routine(ds, args, count);
    port_trace("link", "<- %s = %08x", l->name, (unsigned)r);
    return r;
}

uint16_t port_service(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    return (uint16_t)port_call(ds, address, args, count);
}

/* ---- diagnostics ------------------------------------------------------- */

static FILE *log_file;
static const char *trace_filter;
static int trace_checked;

static void log_open(void)
{
    if (!log_file) {
        const char *path = getenv("OPEU_LOG");
        log_file = fopen(path ? path : "port.log", "w");
    }
}

int port_trace_enabled(const char *category)
{
    if (!trace_checked) {
        trace_filter = getenv("OPEU_TRACE");
        trace_checked = 1;
    }
    if (!trace_filter || !*trace_filter)
        return 0;
    return strcmp(trace_filter, "all") == 0 || strstr(trace_filter, category) != NULL;
}

void port_trace(const char *category, const char *fmt, ...)
{
    va_list ap;
    if (!port_trace_enabled(category))
        return;
    log_open();
    if (!log_file)
        return;
    fprintf(log_file, "[%s] ", category);
    va_start(ap, fmt);
    vfprintf(log_file, fmt, ap);
    va_end(ap);
    fputc('\n', log_file);
    fflush(log_file);
}

void port_fatal(const char *fmt, ...)
{
    va_list ap;
    log_open();
    fputs("opeu: fatal: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    if (log_file) {
        fputs("fatal: ", log_file);
        va_start(ap, fmt);
        vfprintf(log_file, fmt, ap);
        va_end(ap);
        fputc('\n', log_file);
        fclose(log_file);
        log_file = NULL;
    }
    exit(70);
}

void port_unhandled(const char *module, unsigned id, unsigned count)
{
    port_fatal("%s: service %u (%u args) is not linked", module, id, count);
}
