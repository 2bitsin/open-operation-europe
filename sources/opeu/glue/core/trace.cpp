/* Diagnostics: the trace log (OPEU_TRACE, OPEU_LOG), the fatal exit and the
 * report of an unlinked service. */
#include "port.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
