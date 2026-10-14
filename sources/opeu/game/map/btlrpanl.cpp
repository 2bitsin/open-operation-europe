// Ranged-fire messages.
//
// The wiped-out notice reports "<type> wiped out!" for a destroyed element in its side's colour; the ranged panel
// fills the battle panel with "<shooter> vs <target>" (or "<objective>'s bunker" when the fire is aimed at a
// fortified cell) and prints "<shooter> firing.".
#include "map/btlrpanl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/btlcmdr.hpp"
#include "map/btllist.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

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

void opeu_battle_wiped_notice(Game& game, uint16_t element) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ MESSAGE_BYTES };
  auto const          message{ locals.At(0) };
  auto const          side{ opeu_battle_element_side(game, element) };
  opeu_request_format(game, message, std::array<FormatArgument, 3>{ Near(data, data.battle_fmt_wiped), SideColorCode(side), ElementTypeOf(data, element).name });
  opeu_battle_commander_wait(game, message, static_cast<uint8_t>(side), static_cast<uint8_t>(2));
}

void opeu_battle_ranged_panel(Game& game, uint16_t shooter, uint16_t target) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NAMES_RESERVE };
  auto const          target_name{ locals.At(TARGET_AT) };
  auto const          shooter_name{ locals.At(SHOOTER_AT) };
  auto const          side{ opeu_battle_element_side(game, shooter) };
  opeu_battle_panel_begin(game);
  opeu_text_paper_set(game, static_cast<uint8_t>(PANEL_PAPER));
  opeu_army_position(game, static_cast<short>(NAME_COLUMN), static_cast<short>(SHOOTER_ROW));
  opeu_text_color(game, SideTextColor(side));
  opeu_ui_text(game, opeu_element_name(game, shooter, shooter_name));
  opeu_army_position(game, static_cast<short>(VERSUS_COLUMN), static_cast<short>(VERSUS_ROW));
  opeu_text_color(game, WHITE);
  opeu_ui_text(game, Near(data, data.battle_text_vs));
  opeu_army_position(game, static_cast<short>(NAME_COLUMN), static_cast<short>(TARGET_ROW));
  opeu_text_color(game, SideTextColor(side == 0 ? 1 : 0));
  if (target) {
    opeu_ui_text(game, opeu_element_name(game, target, target_name));
  } else {
    auto const objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
    auto const copied{ opeu_string_copy(game, target_name, objective) };
    opeu_ui_text(game, opeu_string_append(game, copied, Near(data, data.battle_text_s)));
    opeu_army_position(game, static_cast<short>(NAME_COLUMN), static_cast<short>(BUNKER_ROW));
    opeu_cursor_print(game, std::array<FormatArgument, 1>{ Near(data, data.battle_text_bunker) });
  }
  opeu_battle_commander_panel(game, static_cast<uint8_t>(side), 1);
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ side == 0 ? Near(data, data.battle_fmt_firing0) : Near(data, data.battle_fmt_firing1), shooter_name });
  opeu_text_color(game, WHITE);
  opeu_text_paper_set(game, static_cast<uint8_t>(0));
}
