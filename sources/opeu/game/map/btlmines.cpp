// Mines on the battle field: the report of an element that ran into them, the men they hit, and the hit itself.
#include "map/btlmines.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/limits.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The report's DS reserve for its message buffer; the hit runs the report below its own stack pointer.
constexpr uint16_t REPORT_TEXT_BYTES{ 0x68 };
constexpr uint16_t COMMANDER_REPORT{ 2 };
constexpr uint16_t HIT_PERCENT{ 10 };
constexpr uint16_t BLAST_ANIMATION{ 0x6fb8 }; // in the icon segment

// Appends the string at DS offset source to the one at destination.
void Append(Game& game, uint16_t destination, uint16_t source) {
  while (Byte(game, destination)) ++destination;
  while ((Byte(game, destination++) = Byte(game, source++)) != 0) {
  }
}
} // namespace

// "<side> <type>: We discovered mines when we retreated!" for a side a player commands.
void opeu_battle_mine_report(Game& game, BattleEvent event, uint16_t element) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ REPORT_TEXT_BYTES };
  auto const          buffer{ locals.At(0) };
  auto const          side{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_ELEMENT_SIDE, element)) };
  if (data.side_status[side] == SIDE_COMPUTER) return;
  Call(game, event, BattleRoutine::REQUEST_FORMAT, buffer, Near(data, data.mine_text), SideColorCode(side), data.side_names[side], ElementTypeOf(data, element).name);
  Append(game, buffer, Near(data, data.retreat_mines_text));
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_WAIT, buffer, side, COMMANDER_REPORT);
}

// One draw per man (the quantity read as signed); each hits one time in ten.
uint16_t opeu_battle_mine_damage(Game& game, BattleEvent event, uint16_t element) {
  Call(game, event, BattleRoutine::BATTLE_ELEMENT_UNIT, element);
  auto const count{ static_cast<int16_t>(Record<Element>(Data(game), element).quantity) };
  int16_t    hits{ 0 };
  for (int16_t man{ 0 }; man < count; ++man)
    if (Call(game, event, BattleRoutine::RAND_BOUNDED, 100) < HIT_PERCENT) ++hits;
  return static_cast<uint16_t>(hits);
}

// The element is revealed, reported and blasted; its losses spend its supplies, and a destroyed element leaves the
// field.
void opeu_battle_mine_hit(Game& game, BattleEvent event, uint16_t element) {
  auto& data{ Data(game) };
  auto& hit{ Record<Element>(data, element) };
  hit.flags |= Bits(ElementFlag::SPOTTED);
  auto const x{ PositionOf(data, element).x };
  auto const y{ PositionOf(data, element).y };
  opeu_battle_mine_report(game, event, element);
  Call(game, event, BattleRoutine::BATTLE_ANIMATE_ELEMENT, x, y, 0, 1, BLAST_ANIMATION, element);
  auto const damage{ static_cast<int16_t>(opeu_battle_mine_damage(game, event, element)) };
  Call(game, event, BattleRoutine::BATTLE_PANEL_BEGIN);
  data.battle_element_destroyed = 0;
  if (damage > 0) {
    auto const old{ hit.quantity };
    hit.quantity = static_cast<uint16_t>(hit.quantity - opeu_min16(hit.quantity, static_cast<uint16_t>(damage)));
    Call(game, event, BattleRoutine::BATTLE_SPEND_FUEL, old, element);
    Call(game, event, BattleRoutine::BATTLE_SPEND_AMMO, old, element);
    Call(game, event, BattleRoutine::BATTLE_SPEND_STORES, old, element);
    if (hit.quantity == 0) {
      Call(game, event, BattleRoutine::BATTLE_DRAW_CELL, x, y);
      hit.damage = 0;
      Call(game, event, BattleRoutine::BATTLE_WIPED_NOTICE, element);
      data.battle_element_destroyed = 1;
      Call(game, event, BattleRoutine::BATTLE_SET_ORDER, element, 0);
      return;
    }
    if (auto const category{ data.element_types[hit.type].category }; category != 0) {
      auto const amount{ Call(game, event, BattleRoutine::RAND_BOUNDED, damage / 2) };
      auto const sum{ static_cast<uint32_t>(amount) + hit.damage };
      auto const limit{ data.request_capacities[category] };
      hit.damage = static_cast<uint16_t>(sum > limit ? limit : sum);
    }
  }
  auto const mode{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_MODE, element)) };
  Call(game, event, BattleRoutine::BATTLE_REDRAW_CELL, x, y, element, mode);
  Call(game, event, BattleRoutine::BATTLE_SET_ORDER, element, 0);
}

// The battle services the mine routines reach, their words in prototype order: the order setter and the commander
// report take theirs reordered.
uint16_t svc_battle_mines(Game& game, uint32_t address, const uint16_t* args, unsigned count) {
  static constexpr unsigned char SET_ORDER_WORDS[]{ 1, 0 }; // element, order
  static constexpr unsigned char REPORT_WORDS[]{ 1, 2, 0 }; // text, side, value
  switch (static_cast<BattleRoutine>(address)) {
  case BattleRoutine::BATTLE_SET_ORDER:      return btl_reorder(game, address, args, count, SET_ORDER_WORDS, 2);
  case BattleRoutine::BATTLE_COMMANDER_WAIT: return btl_reorder(game, address, args, count, REPORT_WORDS, 3);
  default:                                   return port_service(game, address, args, count);
  }
}

uint16_t svc_btlmines(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_mines(game, service, args, count);
}

auto port_routine_opeu_battle_mine_damage(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_mine_damage(game, svc_btlmines, Arg(args, count, 0));
}

auto port_routine_opeu_battle_mine_hit(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_mine_hit(game, svc_btlmines, Arg(args, count, 0));
  return 0;
}
