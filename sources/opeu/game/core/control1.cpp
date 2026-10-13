// Turn control of a side the player commands: the command menu of the current unit's side, and the map inspection
// mode the menu's cancel toggles to, until the side's delay ends the turn.
#include "core/control1.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/context.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/advmenu.hpp"
#include "core/armysum.hpp"
#include "core/coords.hpp"
#include "core/dialogs.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t PROMPT_FORMAT{ offsetof(DataSegment, control_prompt) };
constexpr std::uint16_t READY_TEXT{ offsetof(DataSegment, control_ready) };
constexpr std::uint16_t COMMAND_MENU{ offsetof(DataSegment, control_menu) };
// The command prompt the format service writes, and the inspected cell's x and y words the coordinates dialog moves.
constexpr std::uint16_t PROMPT_BYTES{ 50 };
constexpr std::uint16_t CELL_X_AT{ 0 };
constexpr std::uint16_t CELL_Y_AT{ 2 };
constexpr std::uint16_t CELL_BYTES{ 4 };
} // namespace

void opeu_turn_control_one(Game& game) {
  auto& data{ Data(game) };
  bool  inspecting{ false };
  data.context_side  = NO_SIDE;
  data.view_side     = NO_SIDE;
  data.overview_side = NO_SIDE;
  opeu_control_prepare(game);
  opeu_control_begin(game, svc_soundcue);
  while (!data.side_delays[data.current_side]) {
    {
      auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
      opeu_control_position(game, unit.x, unit.y);
      opeu_arrival_location(game, static_cast<std::uint8_t>(unit.x), static_cast<std::uint8_t>(unit.y), static_cast<std::uint8_t>(data.current_side));
    }
    opeu_daily_context(game, data.current_side);
    for (;;) {
      opeu_army_summary(game);
      data.control_selection = 0;
      opeu_turn_active_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
      opeu_turn_pre_context(game);
      if (!inspecting) {
        ServiceLocals const prompt{ PROMPT_BYTES };
        opeu_request_format(game,
                            std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(prompt.At(0)), static_cast<std::uint16_t>(PROMPT_FORMAT), static_cast<std::uint16_t>(data.context_record1) });
        auto const choice{ opeu_control_choose(game, static_cast<const DialogHost*>(port_dialogs_host()), prompt.At(0), 0, 10, 1, data.context_record2, COMMAND_MENU) };
        if (choice != MENU_CANCELLED) {
          auto const action{ data.control_actions[choice] };
          port_call(game, static_cast<std::uint32_t>(static_cast<std::uint16_t>(action.segment)) * 16u + static_cast<std::uint16_t>(action.offset), nullptr, 0);
          break;
        }
        inspecting             = true;
        data.control_selection = 0;
      } else {
        inspecting             = false;
        data.control_selection = 0;
        auto const&         unit{ Record<Unit>(data, data.current_unit_ptr) };
        ServiceLocals const cell{ CELL_BYTES };
        auto const          at_x{ cell.At(CELL_X_AT) };
        auto const          at_y{ cell.At(CELL_Y_AT) };
        SetWord(game, at_x, unit.x);
        SetWord(game, at_y, unit.y);
        do opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), READY_TEXT, data.context_record2);
        while (opeu_control_coordinates(game, at_x, at_y));
      }
    }
  }
  data.control_selection = 0;
}
