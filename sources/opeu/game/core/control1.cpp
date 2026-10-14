// Turn control of a side the player commands: the command menu of the current unit's side, and the map inspection
// mode the menu's cancel toggles to, until the side's delay ends the turn.
#include "core/control1.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/context.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/advmenu.hpp"
#include "core/airforce.hpp"
#include "core/armysum.hpp"
#include "core/coords.hpp"
#include "core/delay.hpp"
#include "core/dialogs.hpp"
#include "core/persmenu.hpp"
#include "core/rqmenu.hpp"
#include "core/settings.hpp"
#include "core/specfrc.hpp"
#include "core/stockord.hpp"
#include "io/soundcue.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/cmdxfer.hpp"
#include "units/ordlist.hpp"
#include "units/organize.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t PROMPT_FORMAT{ offsetof(DataSegment, control_prompt) };
constexpr std::uint16_t READY_TEXT{ offsetof(DataSegment, control_ready) };
constexpr std::uint16_t COMMAND_MENU{ offsetof(DataSegment, control_menu) };
// The command prompt the format service writes.
constexpr std::uint16_t PROMPT_BYTES{ 50 };

// The command menu's routines, by the enumerator control_actions holds for the choice.
constexpr auto CONTROL_ACTIONS = DispatchRows<ControlAction, auto (*)(Game&)->void>({
    { ControlAction::DELAY,          opeu_choose_turn_delay },
    { ControlAction::ORDERS,         opeu_turn_action_1     },
    { ControlAction::STOCKS,         opeu_turn_action_2     },
    { ControlAction::TRANSFER,       opeu_turn_action_3     },
    { ControlAction::PERSONNEL,      opeu_turn_action_4     },
    { ControlAction::STAFF,          opeu_turn_action_5     },
    { ControlAction::ORGANIZE,       opeu_turn_action_6     },
    { ControlAction::REQUEST,        opeu_turn_action_7     },
    { ControlAction::AIR_FORCE,      opeu_turn_action_8     },
    { ControlAction::SPECIAL_FORCES, opeu_turn_action_9     },
    { ControlAction::SETTINGS,       opeu_turn_action_10    },
});
} // namespace

void opeu_turn_control_one(Game& game) {
  auto& data{ Data(game) };
  bool  inspecting{ false };
  data.context_side  = NO_SIDE;
  data.view_side     = NO_SIDE;
  data.overview_side = NO_SIDE;
  opeu_control_prepare(game);
  opeu_control_begin(game);
  while (!data.side_delays[data.current_side]) {
    {
      auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
      opeu_control_position(game, unit.x, unit.y);
      opeu_arrival_location(game, unit.x, unit.y, data.current_side);
    }
    opeu_daily_context(game, data.current_side);
    for (;;) {
      opeu_army_summary(game);
      data.control_selection = 0;
      opeu_turn_active_begin(game);
      opeu_turn_pre_context(game);
      if (!inspecting) {
        TextBuffer<PROMPT_BYTES> prompt;
        opeu_request_format(game, prompt, std::array<FormatArgument, 2>{ PROMPT_FORMAT, data.context_record1 });
        auto const choice{ opeu_control_choose(game, prompt, 0, 10, 1, data.context_record2, COMMAND_MENU) };
        if (choice != MENU_CANCELLED) {
          Dispatch(CONTROL_ACTIONS, data.control_actions.at(choice), game);
          break;
        }
        inspecting             = true;
        data.control_selection = 0;
      } else {
        inspecting             = false;
        data.control_selection = 0;
        auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
        Inspection  inspection{
          .cell = { unit.x, unit.y },
            .picked = false
        };
        do {
          opeu_control_message(game, READY_TEXT, data.context_record2);
          inspection = InspectMap(game, inspection.cell);
        } while (inspection.picked);
      }
    }
  }
  data.control_selection = 0;
}
