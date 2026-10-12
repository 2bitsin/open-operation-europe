// Formats the name and number of a regiment ("<entry name> <number>") into a buffer.
#include "types.hpp"
#include "units/unitname.hpp"
#include "common/data-segment.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

// The regiment's index in the unit table selects the entry name (index % 8) and the number shown: 11-18 in a side's
// first group, 21-28 in the second, and so on.
uint16_t opeu_unit_name(uint8_t ds[65536], HostEvent event, uint16_t unit,
                        uint16_t buffer) {
  auto const&         data{ Data(ds) };
  auto const          offset{ static_cast<std::int16_t>(
      static_cast<std::uint16_t>(unit - UNITS_AT)) };
  std::uint8_t const  index = offset / static_cast<std::int16_t>(sizeof(Unit));
  std::uint16_t const args[4]{
    buffer, Near(data, data.unit_name_format), data.entry_names[index % 8],
    static_cast<std::uint16_t>((index / 8 % 4) * 10 + index % 8 + 11)
  };
  event(ds, 2, args, 4);
  return buffer;
}

// The service the module calls out to: request_format (id 2) writes the formatted name.
uint16_t svc_unitname(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  uint16_t r = 0;
  switch (id) {
    case 2:  r = lf_request_format(ds, args, count); break;
    default: port_unhandled("unitname", id, count);
  }
  return r;
}

auto port_routine_opeu_unit_name(std::uint8_t* ds, std::uint16_t const* args,
                                 unsigned count) -> std::uint32_t {
  return opeu_unit_name(ds, svc_unitname, Arg(args, count, 1),
                        Arg(args, count, 0));
}
