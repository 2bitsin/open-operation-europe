// Mines on the battle field: the report of an element that ran into them, the men they hit, and the hit itself.
#include "map/btlmines.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/limits.hpp"
#include "core/textout.hpp"
#include "map/btlcell.hpp"
#include "map/btlcmdr.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlrpanl.hpp"
#include "map/btlspend.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <algorithm>
#include <array>
#include <ranges>

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
void opeu_battle_mine_report(Game& game, uint16_t element) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ REPORT_TEXT_BYTES };
  auto const          buffer{ locals.At(0) };
  auto const          side{ static_cast<uint8_t>(opeu_battle_element_side(game, element)) };
  if (data.side_status[side] == SIDE_COMPUTER) return;
  opeu_request_format(game, std::array<std::uint16_t, 5>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.mine_text)), static_cast<std::uint16_t>(SideColorCode(side)),
                                                          static_cast<std::uint16_t>(data.side_names[side]), static_cast<std::uint16_t>(ElementTypeOf(data, element).name) });
  Append(game, buffer, Near(data, data.retreat_mines_text));
  opeu_battle_commander_wait(game, buffer, static_cast<uint8_t>(side), static_cast<uint8_t>(COMMANDER_REPORT));
}

// One draw per man (the quantity read as signed); each hits one time in ten.
uint16_t opeu_battle_mine_damage(Game& game, uint16_t element) {
  opeu_battle_element_unit(game, element);
  auto const count{ static_cast<int16_t>(Record<Element>(Data(game), element).quantity) };
  auto const hit{ [&](int16_t) { return port_rand_bounded(game, 100) < HIT_PERCENT; } };
  return static_cast<uint16_t>(std::ranges::count_if(std::views::iota(int16_t{ 0 }, std::max(count, int16_t{ 0 })), hit));
}

// The element is revealed, reported and blasted; its losses spend its supplies, and a destroyed element leaves the
// field.
void opeu_battle_mine_hit(Game& game, uint16_t element) {
  auto& data{ Data(game) };
  auto& hit{ Record<Element>(data, element) };
  hit.flags |= Bits(ElementFlag::SPOTTED);
  auto const x{ PositionOf(data, element).x };
  auto const y{ PositionOf(data, element).y };
  opeu_battle_mine_report(game, element);
  opeu_battle_animate_element(game, { x, y }, { BLAST_ANIMATION, element, true, false });
  auto const damage{ static_cast<int16_t>(opeu_battle_mine_damage(game, element)) };
  opeu_battle_panel_begin(game);
  data.battle_element_destroyed = 0;
  if (damage > 0) {
    auto const old{ hit.quantity };
    hit.quantity = static_cast<uint16_t>(hit.quantity - opeu_min16(hit.quantity, static_cast<uint16_t>(damage)));
    opeu_battle_spend_fuel(game, element, old);
    opeu_battle_spend_ammo(game, element, old);
    opeu_battle_spend_stores(game, element, old);
    if (hit.quantity == 0) {
      opeu_battle_draw_cell(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
      hit.damage = 0;
      opeu_battle_wiped_notice(game, element);
      data.battle_element_destroyed = 1;
      opeu_battle_set_order(game, element, static_cast<uint8_t>(0));
      return;
    }
    if (auto const category{ data.element_types[hit.type].category }; category != 0) {
      auto const amount{ port_rand_bounded(game, damage / 2) };
      auto const sum{ static_cast<uint32_t>(amount) + hit.damage };
      auto const limit{ data.request_capacities[category] };
      hit.damage = static_cast<uint16_t>(sum > limit ? limit : sum);
    }
  }
  auto const mode{ static_cast<uint8_t>(opeu_battle_mode(game, element)) };
  opeu_battle_redraw_cell(game, { x, y }, { element, mode });
  opeu_battle_set_order(game, element, static_cast<uint8_t>(0));
}
