// A battle fought out of sight: its run loop, the elements' battle records, the damage to the objective, and the
// entry.
#include "map/btlend.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "map/btlcalc.hpp"
#include "map/btlfight.hpp"
#include "map/btllist.hpp"
#include "map/btlretr.hpp"
#include "map/btlsetup.hpp"
#include "map/btlturn.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t ROUNDS{ 20 };
constexpr uint8_t NO_DIRECTION{ 0xff };
} // namespace

// Up to 20 rounds; the battle ends early when the computer turn decides it or a side gives way.
void opeu_battle_run(Game& game) {
  auto& data{ Data(game) };
  for (data.battle_level_limit = 0; data.battle_level_limit < ROUNDS; ++data.battle_level_limit) {
    opeu_battle_morale_check(game);
    if (static_cast<uint8_t>(opeu_battle_computer_turn(game)) || !opeu_battle_rounds(game)) {
      opeu_battle_finish_turn(game);
      return;
    }
  }
  opeu_battle_finish_none(game);
}

// Each element's rounds between shots come from its strength, and the enemy sees it.
void opeu_battle_refresh_strengths(Game& game) {
  auto& data{ Data(game) };
  for (auto const side : std::views::iota(unsigned{ 0 }, SIDE_COUNT))
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_LIST_LENGTH))
      if (auto const unit{ BattleList(data, side, i) })
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) {
          auto const rounds{ static_cast<uint8_t>(opeu_battle_element_strength(game, it.Near())) };
          PositionOf(data, it.Near()).rounds  = rounds;
          (*it).flags                        |= Bits(ElementFlag::SPOTTED);
        }
}

// The element's battle record is the index-th: off the field, at rest, with no goal.
void opeu_battle_init_element(Game& game, uint16_t element, uint8_t side, uint8_t index) {
  auto& data{ Data(game) };
  auto& record{ Record<Element>(data, element) };
  if (!side)
    record.flags &= static_cast<uint8_t>(~Bits(ElementFlag::SIDE_1));
  else
    record.flags |= Bits(ElementFlag::SIDE_1);
  record.position = static_cast<uint16_t>(Near(data, data.battle_positions) + index * sizeof(BattlePosition));
  auto& position{ PositionOf(data, element) };
  position.x      = 0;
  position.y      = BATTLE_UNPLACED;
  position.state  = 1;
  position.goal_x = 0;
  position.goal_y = 0;
  opeu_battle_set_order(game, element, static_cast<uint8_t>(0));
}

// The garrison fights for the objective's side, every other unit for its own.
void opeu_battle_init_elements(Game& game) {
  auto&   data{ Data(game) };
  uint8_t index{ 0 };
  for (auto const side : std::views::iota(unsigned{ 0 }, SIDE_COUNT))
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_LIST_LENGTH)) {
      auto const unit{ BattleList(data, side, i) };
      if (unit == 0) continue;
      if (unit == Near(data, data.battle_garrison)) {
        auto const site{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) {
          auto const owner{ opeu_objective_side(game, site) };
          opeu_battle_init_element(game, it.Near(), owner, index++);
        }
      } else {
        auto const team{ opeu_unit_side(unit) };
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) opeu_battle_init_element(game, it.Near(), team, index++);
      }
    }
}

// In two battles of five the objective loses strength. The counts are those of each side's last unit; the original's
// counters are uninitialised when a list is empty and start at zero here.
void opeu_battle_objective_damage(Game& game) {
  auto&      data{ Data(game) };
  auto const site{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
  if (site == 0) return;
  if (port_rand_bounded(game, 100) > 40) return;
  uint8_t ours{ 0 };
  uint8_t enemies{ 0 };
  for (unsigned i{ 0 }; BattleList(data, 0, i) != 0; ++i) ours = static_cast<uint8_t>(opeu_battle_element_count(game, BattleList(data, 0, i)));
  for (unsigned i{ 0 }; BattleList(data, 1, i) != 0; ++i) enemies = static_cast<uint8_t>(opeu_battle_element_count(game, BattleList(data, 1, i)));
  auto loss{ static_cast<uint16_t>(ours - enemies) };
  if (loss < 0xfffc) return;
  if (loss < 1) {
    auto const r{ static_cast<uint16_t>(port_rand_bounded(game, 51) + 50) };
    loss = opeu_mul_div(static_cast<uint16_t>(loss + 5), r, 100);
  }
  if (loss > 7) {
    auto const r{ static_cast<uint16_t>(port_rand_bounded(game, 101) + 50) };
    loss = opeu_mul_div(7, r, 100);
  }
  opeu_sub_byte(static_cast<uint8_t>(loss), &Record<Objective>(data, site).strength);
}

void opeu_notice_other(Game& game) {
  Data(game).battle_direction = NO_DIRECTION;
  opeu_battle_init_elements(game);
  opeu_battle_refresh_strengths(game);
  opeu_battle_objective_damage(game);
  opeu_battle_tire_leaders(game);
  opeu_battle_run(game);
}
