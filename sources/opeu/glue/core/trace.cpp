/* Diagnostics: the trace log (OPEU_TRACE, OPEU_LOG), the fatal exit and the
 * report of an unlinked service. */
#include "port.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>

static FILE*       log_file;
static const char* trace_filter;
static int         trace_checked;

static void log_open() {
  if (log_file == nullptr) {
    const char* path = getenv("OPEU_LOG");
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): the log stays open until the fatal exit closes it
    log_file = fopen((path != nullptr) ? path : "port.log", "w");
  }
}

auto port_trace_enabled(const char* category) -> int {
  if (trace_checked == 0) {
    trace_filter  = getenv("OPEU_TRACE");
    trace_checked = 1;
  }
  if ((trace_filter == nullptr) || ((*trace_filter) == 0)) return 0;
  return static_cast<int>(strcmp(trace_filter, "all") == 0 || strstr(trace_filter, category) != nullptr);
}

void port_trace(const char* category, const char* fmt, ...) {
  va_list ap{};
  if (port_trace_enabled(category) == 0) return;
  log_open();
  if (log_file == nullptr) return;
  std::print(log_file, "[{}] ", category);
  va_start(ap, fmt);
  vfprintf(log_file, fmt, ap);
  va_end(ap);
  fputc('\n', log_file);
  fflush(log_file);
}

void port_fatal(const char* fmt, ...) {
  va_list ap{};
  log_open();
  fputs("opeu: fatal: ", stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  if (log_file != nullptr) {
    fputs("fatal: ", log_file);
    va_start(ap, fmt);
    vfprintf(log_file, fmt, ap);
    va_end(ap);
    fputc('\n', log_file);
    fclose(log_file); // NOLINT(cppcoreguidelines-owning-memory): the handle log_open opened
    log_file = nullptr;
  }
  exit(70);
}

void port_unhandled(const char* module, unsigned id, unsigned count) {
  port_fatal("%s: service %u (%u args) is not linked", module, id, count);
}
