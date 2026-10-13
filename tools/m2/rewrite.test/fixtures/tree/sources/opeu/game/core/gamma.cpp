// The fixture's callback bindings: one routine bound to dispatchers that reach the same code, one to dispatchers
// that reach different code, and a Call in another file than its dispatcher (it needs the target's header).
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/alpha.hpp"

#include "port.hpp"

uint16_t svc_gamma(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case SHOW: opeu_pair(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  default:   break;
  }
  return 0;
}

void opeu_same(std::uint8_t* ds, HostEvent event) {
  Call(ds, event, SHOW, 1, 2);
}

void opeu_shared(std::uint8_t* ds, HostEvent event) {
  Call(ds, event, SHOW, 1, 2);
}

void opeu_gamma_main(std::uint8_t* ds) {
  opeu_same(ds, svc_alpha);
  opeu_same(ds, svc_beta);
  opeu_shared(ds, svc_alpha);
  opeu_shared(ds, svc_gamma);
}
