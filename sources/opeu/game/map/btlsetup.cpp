// The battle set-up and round loop run by opeu_notice_one.
//
// opeu_battle_reset_elements gives every element of the engaged units a fresh position record and its side bit;
// opeu_battle_tire_leaders takes health from each engaged group's general (or the side's commander) and
// opeu_battle_run_rounds runs up to ten rounds.
#include "map/btlsetup.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/sound.hpp"
#include "map/btlaitrn.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmdr.hpp"
#include "map/btllist.hpp"
#include "map/btlmap.hpp"
#include "map/btlready.hpp"
#include "map/btlround.hpp"
#include "map/btlturn.hpp"
#include "map/btlunit.hpp"
#include "map/dplside.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {
constexpr uint8_t  ROUND_COUNT{ 10 };
constexpr uint8_t  GROUP_PRESENCE{ SIDE_COUNT * GROUPS_PER_SIDE };
constexpr uint16_t BANNER_X{ 2 };
constexpr uint16_t BANNER_Y{ 0x90 };
constexpr uint16_t BATTLE_MUSIC_PLAYER{ 7 };
constexpr uint16_t BATTLE_MUSIC_COMPUTER{ 8 };
constexpr uint8_t  POSITION_BYTES{ sizeof(BattlePosition) };

// A general's health drops by 3 at night, else by 2.
void Tire(Game& game, uint16_t person) {
  auto&      data{ Data(game) };
  auto const night{ opeu_hour_is_night(static_cast<uint8_t>(data.hour)) };
  opeu_sub_byte(night ? 3 : 2, game.State().Bytes(Near(data, Record<Person>(data, person).health), 1).data());
}
} // namespace

uint16_t opeu_battle_side_needs_input(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, side, slot) }; unit && !opeu_battle_unit_controlled(game, svc_btlcalc, unit)) return 1;
  return 0;
}

void opeu_battle_run_rounds(Game& game) {
  auto& data{ Data(game) };
  for (data.battle_level_limit = 0; data.battle_level_limit < ROUND_COUNT; ++data.battle_level_limit) {
    if (static_cast<uint8_t>(opeu_battle_computer_turn(game))) {
      opeu_battle_report_outcome(game);
      return;
    }
    for (uint8_t line{ 1 }; line <= 4; ++line) opeu_battle_restore_tile(game, static_cast<uint8_t>(line), static_cast<uint8_t>(9));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(BANNER_X), static_cast<short>(BANNER_Y));
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.battle_round_format)), static_cast<std::uint16_t>(data.battle_level_limit + 1) });
    if (opeu_battle_side_needs_input(game, 0) || opeu_battle_side_needs_input(game, 1)) opeu_battle_select_service(game);
    if (data.battle_level_limit) opeu_battle_announce_ready(game);
    if (!opeu_battle_turn_loop(game)) {
      opeu_battle_report_outcome(game);
      return;
    }
  }
  opeu_battle_report_stalemate(game);
}

void opeu_battle_tire_leaders(Game& game) {
  auto&                            data{ Data(game) };
  std::array<bool, GROUP_PRESENCE> present{};
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (auto const unit{ BattleList(data, side, slot) }; unit && unit != GARRISON_AT) {
        unsigned at{ opeu_battle_unit_side(game, unit) * GROUPS_PER_SIDE };
        at += static_cast<uint8_t>(opeu_unit_group_index(unit));
        if (at < GROUP_PRESENCE) present[at] = true;
      }
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side) {
    if (!data.battle_command[side]) {
      for (uint8_t group{ 0 }; group < GROUPS_PER_SIDE; ++group)
        if (present[side * GROUPS_PER_SIDE + group]) Tire(game, opeu_find_group_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(group)));
    } else {
      Tire(game, opeu_find_side_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(1)));
    }
  }
}

void opeu_battle_start_music(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ opeu_battle_unit_side(game, data.battle_lists[0]) };
  opeu_set_turn_context(game, static_cast<std::uint8_t>(data.side_status[side] == SIDE_PLAYED ? BATTLE_MUSIC_PLAYER : BATTLE_MUSIC_COMPUTER));
}

// side: nonzero for side 1. index: the element's position record.
void opeu_battle_reset_element(Game& game, uint16_t element, uint8_t side, uint8_t index) {
  auto& data{ Data(game) };
  auto& record{ Record<Element>(data, element) };
  if (!side)
    record.flags &= static_cast<uint8_t>(~Bits(ElementFlag::SIDE_1));
  else
    record.flags |= Bits(ElementFlag::SIDE_1);
  record.position = static_cast<uint16_t>(Near(data, data.battle_positions) + index * POSITION_BYTES);
  auto& position{ PositionOf(data, element) };
  position.x         = BATTLE_UNPLACED;
  position.y         = BATTLE_UNPLACED;
  position.facing    = 0xff;
  position.unknown_3 = 0xff;
  position.strength  = 0xff;
  position.state     = std::to_underlying(BattleState::DEFEND);
  position.goal_x    = 0;
  position.goal_y    = 0;
  opeu_battle_set_order(game, element, 0);
}

void opeu_battle_reset_elements(Game& game) {
  auto&   data{ Data(game) };
  uint8_t index{ 0 };
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot) {
      auto const unit{ BattleList(data, side, slot) };
      if (!unit) continue;
      uint8_t element_side;
      if (unit == GARRISON_AT) {
        auto const objective{ opeu_find_objective_at(game, static_cast<uint8_t>(data.battle_x), static_cast<uint8_t>(data.battle_y)) };
        element_side = (Record<Objective>(data, objective).flags & Bits(ObjectiveFlag::SIDE_1)) != 0;
      } else {
        element_side = static_cast<uint8_t>(opeu_unit_side(unit));
      }
      for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) opeu_battle_reset_element(game, it.Near(), element_side, index++);
    }
}

void opeu_notice_one(Game& game) {
  opeu_battle_reset_elements(game);
  opeu_battle_start_music(game);
  opeu_battle_map_setup(game);
  opeu_deploy_battle(game);
  opeu_battle_tire_leaders(game);
  opeu_battle_run_rounds(game);
  opeu_vga_clear();
  opeu_palette_for_hour(game, static_cast<const UiHost*>(port_uiprim_host()));
  opeu_screen_reset(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_daily_context(game, static_cast<std::uint8_t>(Data(game).current_side));
}
