/* Routine link table lookup: the call by DS address through the generated
 * table (gen/link-table.cpp). The trace and the fatal exit are the glue's
 * (glue/core/trace.cpp). */
#include "link/link.hpp"
#include "common/game.hpp"
#include "port.hpp"

#include <stdio.h>
#include <string.h>

extern const PortLink port_links[];
extern const unsigned port_link_count;

static const PortLink* find(uint32_t address) {
  unsigned lo = 0, hi = port_link_count;
  while (lo < hi) {
    unsigned mid = (lo + hi) / 2;
    if (port_links[mid].address < address)
      lo = mid + 1;
    else
      hi = mid;
  }
  if (lo < port_link_count && port_links[lo].address == address) return &port_links[lo];
  return nullptr;
}

const char* port_routine_name(uint32_t address) {
  const PortLink* l = find(address);
  return l ? l->name : nullptr;
}

uint32_t port_routine_address(const char* name) {
  unsigned i;
  for (i = 0; i < port_link_count; i++)
    if (strcmp(port_links[i].name, name) == 0) return port_links[i].address;
  return 0;
}

uint32_t port_call(Game& game, uint32_t address, const uint16_t* args, unsigned count) {
  const PortLink* l = find(address);
  uint32_t        r;
  if (!l || !l->routine) port_fatal("call to unlinked routine %s (%05x) with %u args", l ? l->name : "?", (unsigned)address, count);
  if (port_trace_enabled("link")) {
    char     buf[160];
    int      n = 0;
    unsigned i;
    for (i = 0; i < count && n < 140; i++) n += snprintf(buf + n, sizeof buf - (size_t)n, " %04x", args[i]);
    buf[n] = 0;
    port_trace("link", "-> %s%s", l->name, buf);
  }
  r = l->routine(game, args, count);
  port_trace("link", "<- %s = %08x", l->name, (unsigned)r);
  return r;
}

uint16_t port_service(Game& game, uint32_t address, const uint16_t* args, unsigned count) {
  return (uint16_t)port_call(game, address, args, count);
}
