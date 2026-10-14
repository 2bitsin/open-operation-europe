// Staff advice menus. opeu_control_prepare clears the five advice buffers before a command turn. The advice menu
// re-shows a topic's buffered advice or runs the topic's action; command action 5 asks Operations or Intelligence
// until Intelligence reports a result of 1.
#include "core/advmenu.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "types.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/advenemy.hpp"
#include "core/advhints.hpp"
#include "core/advregt.hpp"
#include "core/advtact.hpp"
#include "core/advtide.hpp"
#include "core/dialogs.hpp"
#include "core/vwarms.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

// Near pointers are DS offsets.
namespace {

constexpr std::uint16_t ADVICE_PROMPT{ offsetof(DataSegment, advice_prompt) };
constexpr std::uint16_t ADVICE_MENU{ offsetof(DataSegment, advice_menu) };
constexpr std::uint16_t STAFF_PROMPT{ offsetof(DataSegment, staff_prompt) };
constexpr std::uint16_t STAFF_MENU{ offsetof(DataSegment, staff_menu) };
constexpr std::uint16_t INTELLIGENCE{ 2 }; // the staff menu's second item

// The advice topics' routines, by the enumerator advice_actions holds for the topic; each reads the advice texts.
template <auto ADVISE>
auto Advise(Game& game) -> void {
  ADVISE(game, AdviceTexts(game));
}
constexpr auto ADVICE_ACTIONS = DispatchRows<AdviceAction, auto (*)(Game&)->void>({
    { AdviceAction::TACTICAL,         Advise<opeu_advice_tactical>         },
    { AdviceAction::REGIMENT_STATUS,  Advise<opeu_advice_regiment_status>  },
    { AdviceAction::ENEMY_CONDITIONS, Advise<opeu_advice_enemy_conditions> },
    { AdviceAction::TIDE_OF_WAR,      Advise<opeu_advice_tide_of_war>      },
    { AdviceAction::OPERATION_HINTS,  Advise<opeu_advice_operation_hints>  },
});

// Operations' advice menu, which reports nothing.
auto OperationsAdvice(Game& game) -> std::uint16_t {
  opeu_staff_advice_menu(game);
  return 0;
}

// The staff officers' routines, by the enumerator staff_actions holds for the officer: the result Intelligence reports.
constexpr auto STAFF_ACTIONS = DispatchRows<StaffAction, auto (*)(Game&)->std::uint16_t>({
    { StaffAction::OPERATIONS,   OperationsAdvice    },
    { StaffAction::INTELLIGENCE, opeu_view_menu_loop },
});
} // namespace

void opeu_control_prepare(Game& game) {
  for (auto& message : Data(game).advice_messages) message[0] = 0;
}

// The advice menu: topics 1-5 until cancelled.
void opeu_staff_advice_menu(Game& game) {
  auto& data{ Data(game) };
  for (;;) {
    std::uint16_t topic{ opeu_delay_dialog(game, ADVICE_PROMPT, 5, data.context_record2, ADVICE_MENU) };
    if (topic == MENU_CANCELLED) break;
    topic--;
    if (data.advice_messages[topic][0])
      opeu_control_message(game, Near(data, data.advice_messages[topic]), data.context_record2);
    else
      Dispatch(ADVICE_ACTIONS, data.advice_actions.at(topic), game);
  }
}

// Command action 5: Operations or Intelligence, until Intelligence reports a result of 1.
void opeu_turn_action_5(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t officer;
  std::uint16_t result;
  do {
    if ((officer = opeu_delay_dialog(game, STAFF_PROMPT, 2, data.context_record2, STAFF_MENU)) == MENU_CANCELLED) return;
    result = Dispatch(STAFF_ACTIONS, data.staff_actions.at(officer), game);
  } while (officer != INTELLIGENCE || result != 1);
}
