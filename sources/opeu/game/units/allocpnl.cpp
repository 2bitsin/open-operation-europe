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
#include <array>

using namespace opeu::game;

namespace {
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
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.alloc_name_format)), static_cast<std::uint16_t>(kind.name) });
  opeu_face_draw(game, static_cast<const UiHost*>(port_uiprim_host()), 3, 0x12c, 0, static_cast<uint8_t>(data.supply_types[slot]));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x1c), static_cast<int16_t>(0x128));
  std::uint16_t const load{ static_cast<std::uint16_t>(kind.category == 0 ? 0 : static_cast<unsigned>(kind.fuel_economy * kind.fuel_capacity) / 10) };
  opeu_cursor_print(game, std::array<std::uint16_t, 13>{ static_cast<std::uint16_t>(Near(data, data.alloc_stats_format)), static_cast<std::uint16_t>(STAT_WIDTH),
                                                         static_cast<std::uint16_t>(kind.anti_personnel), static_cast<std::uint16_t>(STAT_WIDTH), static_cast<std::uint16_t>(kind.anti_tank),
                                                         static_cast<std::uint16_t>(STAT_WIDTH), static_cast<std::uint16_t>(kind.defense), static_cast<std::uint16_t>(STAT_WIDTH),
                                                         static_cast<std::uint16_t>(kind.speed), static_cast<std::uint16_t>(STAT_WIDTH), static_cast<std::uint16_t>(load),
                                                         static_cast<std::uint16_t>(STAT_WIDTH), static_cast<std::uint16_t>(kind.gun_range) });
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
