// The fixture's word-array service and scalar event: a dispatcher of an array without a count that forwards a group
// of calls, a pointer local of the dispatcher, a span of a literal length, and a raw call of a scalar event.
#include "common/events.hpp"
#include "common/memory.hpp"

#include "port.hpp"

#include <span>

template <typename... Words>
inline auto Input(std::uint8_t* ds, HostService service, std::uint16_t call, Words... words) -> std::uint16_t {
  std::uint16_t args[7]{ static_cast<std::uint16_t>(words)... };
  return service(ds, call, args);
}

auto svc_draw(std::uint8_t* ds, std::uint16_t call, std::uint16_t* w) -> std::uint16_t {
  switch (call) {
  case 1:  opeu_fill(ds, { w, 3 }); break;
  default: break;
  }
  return 0;
}

auto svc_input(std::uint8_t* ds, std::uint16_t call, std::uint16_t* w) -> std::uint16_t {
  auto* const text{ static_cast<TextHost*>(port_textout_host()) };
  switch (call) {
  case 1:
  case 2:  return svc_draw(ds, call, w);
  case 3:  opeu_put(ds, text, static_cast<std::uint8_t>(w[0])); break;
  default: break;
  }
  return 0;
}

auto svc_pending(std::uint8_t* ds, std::uint16_t id, std::uint16_t a0, std::uint16_t a1, std::uint16_t a2, std::uint16_t a3) -> std::uint16_t {
  switch (id) {
  case 4:  return opeu_pending(ds, a1, a0);
  default: return 0;
  }
}

void opeu_input_main(std::uint8_t* ds, HostService service, PendingEvent event) {
  Input(ds, service, 1, 7, 8);
  Input(ds, service, 3, 'x');
  event(ds, 4, 5, 6, 0, 0);
}

void opeu_input_run(std::uint8_t* ds) {
  opeu_input_main(ds, svc_input, svc_pending);
}
