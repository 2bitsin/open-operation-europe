// The type panel of the equipment allocation screen, the test that both units' amounts are nonempty, and the strength
// values a new element takes from its type's averages.
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
#include <array>

using namespace opeu::game;

namespace {
// The field width of each statistic the panel prints.
constexpr std::uint16_t STAT_WIDTH{ 6 };

// The average of supply type `slot` in `averages`; the slot past the last reads on into the next table, as the game
// does when an element's type is not among the supply types.
[[nodiscard]] auto Average(Game& game, std::array<std::uint8_t, 12> const& averages, std::uint8_t slot) -> std::uint8_t {
  return Byte(game, static_cast<std::uint16_t>(Near(Data(game), averages) + slot));
}
} // namespace

// Restores the type panel's background from the off-screen copy.
void opeu_alloc_clear_panel(Game& game) {
  for (std::uint8_t row{ 0 }; row < 4; ++row) opeu_vga_copy(15, 0x108, 0x1a, 0x20, 1, static_cast<std::uint16_t>(row * 32 + 0x108));
  for (std::uint8_t column{ 0 }; column < 2; ++column) opeu_vga_copy(0x22, 0xb8, 6, 0x50, static_cast<std::uint16_t>(column * 6 + 0x1c), 0x128);
}

// The type's name, symbol and six statistics: hard and soft rating, range, speed, the load (usage times weight over
// ten, nothing for a type of category 0) and the mode.
void opeu_alloc_type_panel(Game& game, uint8_t slot, uint16_t type) {
  auto&       data{ Data(game) };
  auto const& kind{ Record<ElementType>(data, type) };
  opeu_alloc_clear_panel(game);
  opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 1, 0x10a, static_cast<std::uint8_t>(opeu_string_length(game, kind.name)) + 2);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.alloc_name_format), kind.name });
  opeu_face_draw(game, static_cast<const UiHost*>(port_uiprim_host()), 3, 0x12c, 0, static_cast<uint8_t>(data.supply_types[slot]));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x1c, 0x128);
  std::uint16_t const load{ static_cast<std::uint16_t>(kind.category == 0 ? 0 : static_cast<unsigned>(kind.fuel_economy * kind.fuel_capacity) / 10) };
  opeu_cursor_print(game, std::array<std::uint16_t, 13>{ Near(data, data.alloc_stats_format), STAT_WIDTH, kind.anti_personnel, STAT_WIDTH, kind.anti_tank, STAT_WIDTH, kind.defense, STAT_WIDTH,
                                                         kind.speed, STAT_WIDTH, load, STAT_WIDTH, kind.gun_range });
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
