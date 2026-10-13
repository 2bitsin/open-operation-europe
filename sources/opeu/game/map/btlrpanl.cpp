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
#include "common/side-color.hpp"
#include "core/strings.hpp"
#include "map/btllist.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The DS reserves the services fill: the notice's message buffer, and the panel's shooter and target names.
constexpr uint16_t MESSAGE_BYTES{ 0x66 };
constexpr uint16_t NAMES_RESERVE{ 0x42 };
constexpr uint16_t SHOOTER_AT{ 0 };
constexpr uint16_t TARGET_AT{ 0x22 };
// The panel's white text and its paper colour.
constexpr uint16_t WHITE{ 7 };
constexpr uint16_t PANEL_PAPER{ 4 };
// The panel's text cells: the columns of the names and of "vs", the rows of the lines.
constexpr uint16_t NAME_COLUMN{ 0x39 };
constexpr uint16_t VERSUS_COLUMN{ 0x3d };
constexpr uint16_t SHOOTER_ROW{ 0x10 };
constexpr uint16_t VERSUS_ROW{ 0x20 };
constexpr uint16_t TARGET_ROW{ 0x30 };
constexpr uint16_t BUNKER_ROW{ 0x40 };
} // namespace

void opeu_battle_wiped_notice(Game& game, BattleEvent event, uint16_t element) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ MESSAGE_BYTES };
  auto const          message{ locals.At(0) };
  auto const          side{ opeu_battle_element_side(game, event, element) };
  Call(game, event, BattleRoutine::REQUEST_FORMAT, message, Near(data, data.battle_fmt_wiped), SideColorCode(side), ElementTypeOf(data, element).name);
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_WAIT, side, 2, message);
}

void opeu_battle_ranged_panel(Game& game, BattleEvent event, uint16_t shooter, uint16_t target) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NAMES_RESERVE };
  auto const          target_name{ locals.At(TARGET_AT) };
  auto const          shooter_name{ locals.At(SHOOTER_AT) };
  auto const          side{ opeu_battle_element_side(game, event, shooter) };
  Call(game, event, BattleRoutine::BATTLE_PANEL_BEGIN);
  Call(game, event, BattleRoutine::TEXT_PAPER_SET, PANEL_PAPER);
  Call(game, event, BattleRoutine::ARMY_POSITION, NAME_COLUMN, SHOOTER_ROW);
  Call(game, event, BattleRoutine::TEXT_COLOR, SideTextColor(side));
  Call(game, event, BattleRoutine::UI_TEXT, opeu_element_name(game, shooter, shooter_name));
  Call(game, event, BattleRoutine::ARMY_POSITION, VERSUS_COLUMN, VERSUS_ROW);
  Call(game, event, BattleRoutine::TEXT_COLOR, WHITE);
  Call(game, event, BattleRoutine::UI_TEXT, Near(data, data.battle_text_vs));
  Call(game, event, BattleRoutine::ARMY_POSITION, NAME_COLUMN, TARGET_ROW);
  Call(game, event, BattleRoutine::TEXT_COLOR, SideTextColor(side == 0 ? 1 : 0));
  if (target) {
    Call(game, event, BattleRoutine::UI_TEXT, opeu_element_name(game, target, target_name));
  } else {
    auto const objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
    auto const copied{ opeu_string_copy(game, target_name, objective) };
    Call(game, event, BattleRoutine::UI_TEXT, opeu_string_append(game, copied, Near(data, data.battle_text_s)));
    Call(game, event, BattleRoutine::ARMY_POSITION, NAME_COLUMN, BUNKER_ROW);
    Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_text_bunker));
  }
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_PANEL, side, 1);
  Call(game, event, BattleRoutine::CURSOR_PRINT, side == 0 ? Near(data, data.battle_fmt_firing0) : Near(data, data.battle_fmt_firing1), shooter_name);
  Call(game, event, BattleRoutine::TEXT_COLOR, WHITE);
  Call(game, event, BattleRoutine::TEXT_PAPER_SET, 0);
}

uint16_t svc_btlrpanl(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_wiped_notice(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_wiped_notice(game, svc_btlrpanl, Arg(args, count, 0));
  return 0;
}
