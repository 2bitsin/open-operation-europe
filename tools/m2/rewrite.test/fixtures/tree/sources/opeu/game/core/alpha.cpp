// The fixture's numbered dispatcher: one case of each kind, and a routine whose Calls exercise each substitution rule.
#include "core/alpha.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/target.hpp"

#include "port.hpp"

void opeu_alpha_run(std::uint8_t* ds, HostEvent event, std::uint16_t x, std::int16_t signed_word, unsigned wide) {
  Call(ds, event, SHOW, x, 7);
  auto const width{ Call(ds, event, WIDTH, wide) };
  if (Call(ds, event, LOOKUP, signed_word) == 0xFFFF) return;
  Call(ds, event, FORMAT, x, width);
  Call(ds, event, PAIR, opeu_tick(ds), opeu_tick(ds));
  Call(ds, event, SHOW, x);
  auto const nothing{ Call(ds, event, SHOW, x, 1) };
  Call(ds, event, TWICE);
  Call(ds, event, HOST);
}

void opeu_alpha_main(std::uint8_t* ds) {
  opeu_alpha_run(ds, svc_beta, 1, 2, 3);
}

uint16_t svc_alpha(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t   result{ 0 };
  auto const byte{ [&](unsigned i) { return static_cast<std::uint8_t>(Arg(args, count, i)); } };
  auto const dialogs{ static_cast<void const*>(port_dialogs_host()) };
  switch (id) {
  case SHOW:   opeu_show(ds, Arg(args, count, 0), byte(1)); break;
  case LOOKUP: result = opeu_lookup(ds, static_cast<std::int16_t>(Arg(args, count, 0))); break;
  case WIDTH:  return opeu_width(ds, Arg(args, count, 0));
  case FORMAT: opeu_format(ds, { args, count }); break;
  case PAIR:   opeu_pair(ds, Arg(args, count, 1), Arg(args, count, 0)); break;
  case TWICE:  {
    opeu_tick(ds);
    opeu_tick(ds);
    break;
  }
  case WRITE_BACK: const_cast<uint16_t*>(args)[0] = opeu_tick(ds); break;
  case HOST:       opeu_host(ds, dialogs); break;
  case FAR_ACTION: port_call(ds, static_cast<std::uint32_t>(Arg(args, count, 1)) * 16u + Arg(args, count, 0), nullptr, 0); break;
  default:         break;
  }
  return result;
}

uint16_t svc_beta(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_alpha(ds, id, args, count);
}
