// The half-day dispatch: every turn prepares the turn; at midnight the sides receive their daily resources, at noon
// the record counters count down and the noon events run.
#include "core/halfday.hpp"
#include "core/periodic.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/noonevt.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr std::uint16_t FAR_TEXT_SEGMENT{ 0x3895 };  // the far text the noon events read
constexpr std::uint16_t NOON_CODE_SEGMENT{ 0x0e57 }; // the records' far actions in this code segment are noon events
} // namespace

void opeu_turn_half_day(Game& game) {
  auto const& data{ Data(game) };
  opeu_turn_prepare(game, static_cast<const DialogHost*>(port_dialogs_host()));
  if (data.hour == 0)
    opeu_apply_daily_side_resources(game);
  else if (data.hour == 12) {
    opeu_decrement_record_counters(game);
    opeu_turn_noon_events(game, port_far(FAR_TEXT_SEGMENT, 0), svc_noonevt, NOON_CODE_SEGMENT);
  }
}
