// Hourly objective events: each objective's strength grows toward its limit, its communications may be restored and
// an enemy spy in it may be caught; the owner's adviser reports both and the map flashes the objective.
#include "core/evcomms.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/textout.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "link/glue.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t RESTORED_FORMAT{ offsetof(DataSegment, comms_restored_format) };
constexpr std::uint16_t CAPTURED_FORMAT{ offsetof(DataSegment, spy_captured_format) };
constexpr std::uint8_t  ADVISER{ 3 };       // the side record kind of the adviser
constexpr std::uint16_t NEWS_BYTES{ 0x6c }; // the report text the format service writes
} // namespace

// The level is the delivery level for a side the player commands, else 2: a higher level grows strength more slowly
// and makes both events rarer.
void opeu_objective_comms_event(Game& game) {
  ServiceLocals const locals{ NEWS_BYTES };
  auto&               data{ Data(game) };
  auto const          news{ locals.At(0) };
  for (auto& objective : data.objectives) {
    if (!objective.active) continue;
    auto const    camp{ Near(data, objective) };
    auto const    side{ opeu_objective_side(game, camp) };
    std::uint16_t level{ DeliveryLevel(data, side) };
    auto const    speaker{ opeu_find_side_record(game, side, ADVISER) };
    opeu_add_byte(static_cast<std::uint16_t>(RandomBelow(game, static_cast<std::uint16_t>(5 - level)) + 1), objective.strength_limit, &objective.strength);
    if (!(objective.flags & Bits(ObjectiveFlag::COMMUNICATIONS)) && RandomBelow(game, static_cast<std::uint16_t>(level * 10 + 24)) == 6) {
      objective.flags |= Bits(ObjectiveFlag::COMMUNICATIONS);
      opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(news), static_cast<std::uint16_t>(RESTORED_FORMAT), static_cast<std::uint16_t>(camp) });
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), news, speaker);
      opeu_location_flash(game, static_cast<std::uint8_t>(objective.x), static_cast<std::uint8_t>(objective.y), static_cast<std::uint8_t>(side));
    }
    if ((objective.flags & Bits(ObjectiveFlag::MARKED)) && RandomBelow(game, static_cast<std::uint16_t>((level + 6) * 20)) == 8) {
      objective.flags &= static_cast<std::uint8_t>(~Bits(ObjectiveFlag::MARKED));
      opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(news), static_cast<std::uint16_t>(CAPTURED_FORMAT), static_cast<std::uint16_t>(camp) });
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), news, speaker);
      opeu_location_flash(game, static_cast<std::uint8_t>(objective.x), static_cast<std::uint8_t>(objective.y), static_cast<std::uint8_t>(side));
    }
  }
}
