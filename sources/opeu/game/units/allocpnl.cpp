// The type panel of the equipment allocation screen, the test that both units' amounts are nonempty, the strength
// values a new element takes from its type's averages, and the capped proportional share of a value.
#include "units/allocpnl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The services the panel calls through its event (svc_allocpnl).
enum class Service : std::uint16_t {
  BLIT     = 0, // (x, y, width, height, to x, to y): restore a screen area from the off-screen copy
  FRAME    = 1, // (x, y, width): a caption frame
  PRINT    = 2, // (format, ...)
  SYMBOL   = 3, // (x, y, picture, type code): an equipment symbol
  POSITION = 4, // (x, y)
};
// The field width of each statistic the panel prints.
constexpr std::uint16_t STAT_WIDTH{ 6 };
// The largest share opeu_alloc_scaled returns.
constexpr std::int32_t SHARE_CAP{ 60000 };

// The average of supply type `slot` in `averages`; the slot past the last reads on into the next table, as the game
// does when an element's type is not among the supply types.
[[nodiscard]] auto Average(Game& game, std::array<std::uint8_t, 12> const& averages, std::uint8_t slot) -> std::uint8_t {
  return Byte(game, static_cast<std::uint16_t>(Near(Data(game), averages) + slot));
}
} // namespace

// Restores the type panel's background from the off-screen copy.
void opeu_alloc_clear_panel(Game& game, HostWordsEvent event) {
  for (std::uint8_t row{ 0 }; row < 4; ++row) Call(game, event, Service::BLIT, 15, 0x108, 0x1a, 0x20, 1, row * 32 + 0x108);
  for (std::uint8_t column{ 0 }; column < 2; ++column) Call(game, event, Service::BLIT, 0x22, 0xb8, 6, 0x50, column * 6 + 0x1c, 0x128);
}

// The type's name, symbol and six statistics: hard and soft rating, range, speed, the load (usage times weight over
// ten, nothing for a type of category 0) and the mode.
void opeu_alloc_type_panel(Game& game, HostWordsEvent event, uint8_t slot, uint16_t type) {
  auto&       data{ Data(game) };
  auto const& kind{ Record<ElementType>(data, type) };
  opeu_alloc_clear_panel(game, event);
  Call(game, event, Service::FRAME, 1, 0x10a, static_cast<std::uint8_t>(opeu_string_length(game, kind.name)) + 2);
  Call(game, event, Service::PRINT, Near(data, data.alloc_name_format), kind.name);
  Call(game, event, Service::SYMBOL, 3, 0x12c, 0, data.supply_types[slot]);
  Call(game, event, Service::POSITION, 0x1c, 0x128);
  std::uint16_t const load{ static_cast<std::uint16_t>(kind.category == 0 ? 0 : static_cast<unsigned>(kind.fuel_economy * kind.fuel_capacity) / 10) };
  Call(game, event, Service::PRINT, Near(data, data.alloc_stats_format), STAT_WIDTH, kind.anti_personnel, STAT_WIDTH, kind.anti_tank, STAT_WIDTH, kind.defense, STAT_WIDTH, kind.speed, STAT_WIDTH,
       load, STAT_WIDTH, kind.gun_range);
}

// Whether the first unit's `first` allocated amounts, and the second unit's `second` after them (when it has any),
// each hold a nonzero total.
uint16_t opeu_alloc_groups_nonempty(Game& game, uint8_t first, uint8_t second) {
  auto&         data{ Data(game) };
  std::uint16_t total{ 0 };
  for (std::uint16_t slot{ 0 }; slot < first + second; ++slot) {
    total += data.alloc_slots[slot].quantity;
    if (slot == first - 1) {
      if (!total) return 0;
      if (!second) return 1;
      total = 0;
    }
  }
  return total ? 1 : 0;
}

// Gives the element the morale, training and experience averages of its type among the `count` supply types.
void opeu_alloc_element_values(Game& game, uint16_t element, uint8_t count) {
  auto&        data{ Data(game) };
  auto&        unit_element{ Record<Element>(data, element) };
  std::uint8_t slot{ 0 };
  while (slot < count && data.supply_types[slot] != unit_element.type) ++slot;
  unit_element.morale     = Average(game, data.supply_morale_averages, slot);
  unit_element.training   = Average(game, data.supply_training_averages, slot);
  unit_element.experience = Average(game, data.supply_experience_averages, slot);
}

// value * (amount * 100 / total) / 100, capped at 60000; nothing when there is no total. The product keeps its low
// 32 bits.
uint16_t opeu_alloc_scaled(Game& game, int32_t value, uint16_t amount, int32_t total) {
  (void)game;
  if (!total) return 0;
  auto const percent{ static_cast<std::int32_t>(amount) * 100 / total };
  auto const share{ static_cast<std::int32_t>(static_cast<std::uint32_t>(percent) * static_cast<std::uint32_t>(value)) / 100 };
  return static_cast<std::uint16_t>(share > SHARE_CAP ? SHARE_CAP : share);
}

// The panel's services.
uint16_t svc_allocpnl(Game& game, uint16_t id, uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<Service>(id)) {
  case Service::BLIT:     opeu_vga_copy(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5)); break;
  case Service::FRAME:    opeu_context_frame(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case Service::PRINT:    opeu_cursor_print(game, { args, count }); break;
  case Service::SYMBOL:   opeu_face_draw(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), (uint8_t)Arg(args, count, 3)); break;
  case Service::POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  default:                port_unhandled("allocpnl", id, count);
  }
  return r;
}
