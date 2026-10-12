// Hourly objective events: each objective's strength grows toward its limit, its communications may be restored and
// an enemy spy in it may be caught; the owner's adviser reports both and the map flashes the objective.
#include "core/evcomms.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the event calls through its event.
enum Service : std::uint16_t {
  MESSAGE = 4, // (text, person) the adviser's report
  FORMAT  = 5, // (buffer, format, objective)
  FLASH   = 6, // (x, y, side) the objective's location
};

constexpr std::uint16_t RESTORED_FORMAT{ offsetof(DataSegment, comms_restored_format) };
constexpr std::uint16_t CAPTURED_FORMAT{ offsetof(DataSegment, spy_captured_format) };
constexpr std::uint16_t COMPUTER_LEVEL{ 2 }; // the level of a computer side
constexpr std::uint8_t  ADVISER{ 3 };        // the side record kind of the adviser
constexpr std::uint16_t NEWS_BELOW_SP{ 2 + 0x6c }; // the report text's place in the caller's frame
} // namespace

// The level is the delivery level for a side the player commands, else 2: a higher level grows strength more slowly
// and makes both events rarer.
void opeu_objective_comms_event(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const news{ static_cast<std::uint16_t>(sp - NEWS_BELOW_SP) };
  for (auto& objective : data.objectives) {
    if (!objective.active) continue;
    auto const    camp{ Near(data, objective) };
    auto const    side{ opeu_objective_side(ds, camp) };
    std::uint16_t level{ data.side_control[side] == SIDE_PLAYED ? data.delivery_level : COMPUTER_LEVEL };
    auto const    speaker{ opeu_find_side_record(ds, side, ADVISER) };
    opeu_add_byte(static_cast<std::uint16_t>(RandomBelow(ds, static_cast<std::uint16_t>(5 - level)) + 1),
                  objective.strength_limit, &objective.strength);
    if (!(objective.flags & Bits(ObjectiveFlag::COMMUNICATIONS))
        && RandomBelow(ds, static_cast<std::uint16_t>(level * 10 + 24)) == 6) {
      objective.flags |= Bits(ObjectiveFlag::COMMUNICATIONS);
      Call(ds, event, FORMAT, news, RESTORED_FORMAT, camp);
      Call(ds, event, MESSAGE, news, speaker);
      Call(ds, event, FLASH, objective.x, objective.y, side);
    }
    if ((objective.flags & Bits(ObjectiveFlag::MARKED))
        && RandomBelow(ds, static_cast<std::uint16_t>((level + 6) * 20)) == 8) {
      objective.flags &= static_cast<std::uint8_t>(~Bits(ObjectiveFlag::MARKED));
      Call(ds, event, FORMAT, news, CAPTURED_FORMAT, camp);
      Call(ds, event, MESSAGE, news, speaker);
      Call(ds, event, FLASH, objective.x, objective.y, side);
    }
  }
}

uint16_t svc_evcomms(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case MESSAGE:
    opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1));
    break;
  case FORMAT: c1_request_format(ds, args, count); break;
  case FLASH:  {
    auto const frame{ port_frame_enter() };
    opeu_location_flash(ds, svc_mapunit, frame, static_cast<std::uint8_t>(Arg(args, count, 0)),
                        static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    port_frame_leave();
    break;
  }
  default: port_unhandled("evcomms", id, count);
  }
  return 0;
}

auto port_routine_opeu_objective_comms_event(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME, [&](std::uint16_t sp) { opeu_objective_comms_event(ds, svc_evcomms, sp); });
  return 0;
}
