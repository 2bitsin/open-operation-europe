// The side of a battle element and of a battle unit, the battle unit owning an element and a unit's element count.
#include "map/btllist.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ranges>

using namespace opeu::game;

uint8_t opeu_battle_element_side(Game& game, uint16_t element) {
  return BattleSide(Record<Element>(Data(game), element));
}

uint8_t opeu_battle_unit_side(Game& game, uint16_t unit) {
  auto& data{ Data(game) };
  return BattleSide(Record<Element>(data, Record<Unit>(data, unit).first));
}

uint16_t opeu_battle_element_unit(Game& game, uint16_t element) {
  auto& data{ Data(game) };
  for (auto const side : std::views::iota(unsigned{ 0 }, SIDE_COUNT))
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_LIST_LENGTH))
      if (auto const unit{ BattleList(data, side, i) })
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
          if (it.Near() == element) return unit;
  return 0;
}

uint8_t opeu_battle_element_count(Game& game, uint16_t unit) {
  auto&   data{ Data(game) };
  uint8_t count{ 0 };
  for ([[maybe_unused]] auto const& element : Elements(data, Record<Unit>(data, unit))) ++count;
  return count;
}
