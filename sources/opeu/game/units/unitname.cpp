// Formats the name and number of a regiment ("<entry name> <number>") into a buffer.
#include "units/unitname.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

// The regiment's index in the unit table selects the entry name (index % 8) and the number shown: 11-18 in a side's
// first group, 21-28 in the second, and so on.
uint16_t opeu_unit_name(Game& game, uint16_t unit, uint16_t buffer) {
  auto const&         data{ Data(game) };
  auto const          offset{ static_cast<std::int16_t>(static_cast<std::uint16_t>(unit - UNITS_AT)) };
  std::uint8_t const  index = offset / static_cast<std::int16_t>(sizeof(Unit));
  std::uint16_t const args[4]{ buffer, Near(data, data.unit_name_format), data.entry_names[index % 8], static_cast<std::uint16_t>((index / 8 % 4) * 10 + index % 8 + 11) };
  opeu_request_format(game, args);
  return buffer;
}
