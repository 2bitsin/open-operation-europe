// The battle element information panel: the unit or garrison, the element's condition and supplies, its order and
// target, and its type's ratings beside its picture.
#include "map/btlpanel.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/labels.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>

using namespace opeu::game;

namespace {
// The DS reserve of the element name and the unit name buffers the naming routines fill and the print service reads.
constexpr uint16_t NAMES_RESERVE{ 0x4c };
constexpr uint16_t ELEMENT_NAME_AT{ 0 };
constexpr uint16_t UNIT_NAME_AT{ 0x20 };
// The panel's text cells.
constexpr uint16_t TITLE_COLUMN{ 0x2e };
constexpr uint16_t STATS_COLUMN{ 0x45 };
constexpr uint16_t ORDER_COLUMN{ 0x34 };
constexpr uint16_t TARGET_COLUMN{ 0x3c };
constexpr uint16_t LINE{ 0x10 };
constexpr uint16_t FUEL_ROW{ 0x56 };
constexpr uint16_t AMMO_ROW{ 0x66 };
constexpr uint16_t ORDER_ROW{ 0x30 };
constexpr uint16_t TARGET_ROW{ 0x40 };
// The type card on the side panel: its columns for side 0 and 1, the rows of the ratings, the picture.
constexpr uint16_t CARD_COLUMN_0{ 0x44 };
constexpr uint16_t CARD_COLUMN_1{ 1 };
constexpr uint16_t CARD_ROW{ 0x100 };
constexpr uint16_t PICTURE_X_0{ 0x38 };
constexpr uint16_t PICTURE_X_1{ 0 };
constexpr uint16_t PICTURE_Y{ 0x140 };

// opeu_unit_name reports its formatter as its service 2; this forwards it to the battle's request formatter.
BattleEvent name_event;
uint16_t    NameForward(Game& game, uint16_t, const uint16_t* args, uint16_t count) {
  return name_event(game, std::to_underlying(BattleRoutine::REQUEST_FORMAT), args, count);
}
} // namespace

// orders_hidden: the enemy's element, whose order the panel does not show.
void opeu_battle_element_panel(Game& game, BattleEvent event, uint16_t element, uint16_t orders_hidden) {
  auto&               data{ Data(game) };
  auto const&         record{ Record<Element>(data, element) };
  ServiceLocals const locals{ NAMES_RESERVE };
  auto const          unit_name{ locals.At(UNIT_NAME_AT) };
  auto const          element_name{ locals.At(ELEMENT_NAME_AT) };
  auto                side{ opeu_battle_element_side(game, event, element) };
  auto const          unit{ opeu_battle_element_unit(game, event, element) };
  auto const          at{ [&](uint16_t column, uint16_t row) { Call(game, event, BattleRoutine::ARMY_POSITION, column, row); } };
  auto const          print{ [&](auto const& format, auto... words) { opeu_cursor_print(game, std::array<uint16_t, 1 + sizeof...(words)>{ Near(data, format), static_cast<uint16_t>(words)... }); } };
  Call(game, event, BattleRoutine::BATTLE_PANEL_BEGIN);
  at(TITLE_COLUMN, 0);
  uint16_t name;
  if (unit == GARRISON_AT) {
    auto const objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
    print(data.battle_fmt_garrison, SideColorCode(opeu_objective_side(game, objective)), objective);
    at(TITLE_COLUMN, LINE);
    name = opeu_type_name(game, Near(data, data.element_types[record.type]), element_name);
  } else {
    name_event = event;
    name       = opeu_unit_name(game, NameForward, unit, unit_name);
    print(data.battle_fmt_title, SideColorCode(side), data.side_names[side], name);
    at(TITLE_COLUMN, LINE);
    name = opeu_element_name(game, element, element_name);
  }
  Call(game, event, BattleRoutine::UI_TEXT, name);
  at(STATS_COLUMN, 0);
  print(data.battle_fmt_ok, record.quantity);
  at(STATS_COLUMN, LINE);
  print(data.battle_fmt_damage, record.damage);
  at(STATS_COLUMN, 2 * LINE);
  print(data.battle_fmt_morale, record.morale);
  at(STATS_COLUMN, 3 * LINE);
  print(data.battle_fmt_training, record.training);
  at(STATS_COLUMN, 4 * LINE);
  print(data.battle_fmt_fatigue, record.experience);
  // The cell shown as the target: the target's (the element's own once the target is gone), or the goal.
  uint8_t target_x;
  uint8_t target_y;
  if (opeu_battle_engaged(game, event, element)) {
    auto const  target{ PositionOf(data, element).target };
    auto const& position{ PositionOf(data, opeu_battle_element_active(game, event, target) ? target : element) };
    target_x = position.x;
    target_y = position.y;
  } else {
    auto const& position{ PositionOf(data, element) };
    target_x = position.goal_x;
    target_y = position.goal_y;
  }
  auto const& supplies{ Record<Unit>(data, unit) };
  at(STATS_COLUMN, FUEL_ROW);
  print(data.battle_fmt_fuel, supplies.resources[1]);
  at(STATS_COLUMN, AMMO_ROW);
  print(data.battle_fmt_ammo, supplies.resources[0]);
  at(ORDER_COLUMN, ORDER_ROW);
  if (orders_hidden)
    print(data.battle_fmt_no_orders);
  else
    print(data.battle_fmt_orders, data.battle_order_names[opeu_battle_state(game, event, element)]);
  at(TARGET_COLUMN, TARGET_ROW);
  if (!orders_hidden) print(data.battle_fmt_target, target_x, target_y);
  if (orders_hidden) side = opeu_opposite_side(side);
  Call(game, event, side == 0 ? BattleRoutine::BATTLE_PANEL_SIDE0 : BattleRoutine::BATTLE_PANEL_SIDE1);
  auto const& type{ data.element_types[record.type] };
  auto const  card{ side ? CARD_COLUMN_1 : CARD_COLUMN_0 };
  at(card, CARD_ROW);
  print(data.battle_fmt_apat, type.anti_personnel, type.anti_tank);
  at(card, CARD_ROW + LINE);
  print(data.battle_fmt_defense, type.defense);
  at(card, CARD_ROW + 2 * LINE);
  print(data.battle_fmt_speed, type.speed);
  at(card, CARD_ROW + 3 * LINE);
  print(data.battle_fmt_range, type.gun_range);
  Call(game, event, BattleRoutine::FACE_DRAW, side ? PICTURE_X_1 : PICTURE_X_0, PICTURE_Y, 0, record.type);
  Call(game, event, BattleRoutine::UI_TEXT, Near(data, data.battle_fmt_end));
}

uint16_t svc_btlpanel(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_element_panel(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_element_panel(game, svc_btlpanel, Arg(args, count, 1), Arg(args, count, 0));
  return 0;
}
