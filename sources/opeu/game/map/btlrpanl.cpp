// Ranged-fire messages.
//
// The wiped-out notice reports "<type> wiped out!" for a destroyed element in its side's colour; the ranged panel
// fills the battle panel with "<shooter> vs <target>" (or "<objective>'s bunker" when the fire is aimed at a
// fortified cell) and prints "<shooter> firing.".
#include "map/btlrpanl.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/strings.hpp"
#include "map/btllist.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The DS reserves the services fill: the notice's message buffer, and the panel's shooter and target names.
constexpr uint16_t MESSAGE_BYTES{ 0x66 };
constexpr uint16_t NAMES_RESERVE{ 0x42 };
constexpr uint16_t SHOOTER_AT{ 0 };
constexpr uint16_t TARGET_AT{ 0x22 };
// Text colours and the colour escape characters of the two sides.
constexpr uint16_t SIDE_0_COLOR{ 6 };
constexpr uint16_t SIDE_1_COLOR{ 5 };
constexpr uint16_t WHITE{ 7 };
constexpr uint16_t SIDE_0_ESCAPE{ '6' };
constexpr uint16_t SIDE_1_ESCAPE{ '5' };
constexpr uint16_t PANEL_PAPER{ 4 };
// The panel's text cells: the columns of the names and of "vs", the rows of the lines.
constexpr uint16_t NAME_COLUMN{ 0x39 };
constexpr uint16_t VERSUS_COLUMN{ 0x3d };
constexpr uint16_t SHOOTER_ROW{ 0x10 };
constexpr uint16_t VERSUS_ROW{ 0x20 };
constexpr uint16_t TARGET_ROW{ 0x30 };
constexpr uint16_t BUNKER_ROW{ 0x40 };
} // namespace

void opeu_battle_wiped_notice(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t /*frame*/) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ MESSAGE_BYTES };
  auto const          message{ locals.At(0) };
  auto const          side{ opeu_battle_element_side(ds, event, element) };
  Call(ds, event, BattleRoutine::REQUEST_FORMAT, message, Near(data, data.battle_fmt_wiped), side == 0 ? SIDE_0_ESCAPE : SIDE_1_ESCAPE, data.element_types[Record<Element>(data, element).type].name);
  Call(ds, event, BattleRoutine::BATTLE_COMMANDER_WAIT, side, 2, message);
}

void opeu_battle_ranged_panel(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target, uint16_t /*frame*/) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ NAMES_RESERVE };
  auto const          target_name{ locals.At(TARGET_AT) };
  auto const          shooter_name{ locals.At(SHOOTER_AT) };
  auto const          side{ opeu_battle_element_side(ds, event, shooter) };
  Call(ds, event, BattleRoutine::BATTLE_PANEL_BEGIN);
  Call(ds, event, BattleRoutine::TEXT_PAPER_SET, PANEL_PAPER);
  Call(ds, event, BattleRoutine::ARMY_POSITION, NAME_COLUMN, SHOOTER_ROW);
  Call(ds, event, BattleRoutine::TEXT_COLOR, side == 0 ? SIDE_0_COLOR : SIDE_1_COLOR);
  Call(ds, event, BattleRoutine::UI_TEXT, opeu_element_name(ds, shooter, shooter_name));
  Call(ds, event, BattleRoutine::ARMY_POSITION, VERSUS_COLUMN, VERSUS_ROW);
  Call(ds, event, BattleRoutine::TEXT_COLOR, WHITE);
  Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.battle_text_vs));
  Call(ds, event, BattleRoutine::ARMY_POSITION, NAME_COLUMN, TARGET_ROW);
  Call(ds, event, BattleRoutine::TEXT_COLOR, side == 0 ? SIDE_1_COLOR : SIDE_0_COLOR);
  if (target) {
    Call(ds, event, BattleRoutine::UI_TEXT, opeu_element_name(ds, target, target_name));
  } else {
    auto const objective{ opeu_find_objective_at(ds, data.battle_x, data.battle_y) };
    auto const copied{ opeu_string_copy(ds, target_name, objective) };
    Call(ds, event, BattleRoutine::UI_TEXT, opeu_string_append(ds, copied, Near(data, data.battle_text_s)));
    Call(ds, event, BattleRoutine::ARMY_POSITION, NAME_COLUMN, BUNKER_ROW);
    Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_text_bunker));
  }
  Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL, side, 1);
  Call(ds, event, BattleRoutine::CURSOR_PRINT, side == 0 ? Near(data, data.battle_fmt_firing0) : Near(data, data.battle_fmt_firing1), shooter_name);
  Call(ds, event, BattleRoutine::TEXT_COLOR, WHITE);
  Call(ds, event, BattleRoutine::TEXT_PAPER_SET, 0);
}

uint16_t svc_btlrpanl(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_wiped_notice(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) { opeu_battle_wiped_notice(ds, svc_btlrpanl, Arg(args, count, 0), BattleFrame(sp)); });
  return 0;
}
