// The half-day dispatch: every turn prepares the turn; at midnight the sides receive their daily resources, at noon
// the record counters count down and the noon events run.
#include "core/halfday.hpp"
#include "core/periodic.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "core/dialogs.hpp"
#include "core/noonevt.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
enum Service : std::uint16_t {
  PREPARE     = 0,
  NOON_EVENTS = 1,
};
constexpr std::uint16_t FAR_TEXT_SEGMENT{ 0x3895 };  // the far text the noon events read
constexpr std::uint16_t NOON_CODE_SEGMENT{ 0x0e57 }; // the records' far actions in this code segment are noon events
} // namespace

void opeu_turn_half_day(uint8_t ds[65536], uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  auto const& data{ Data(ds) };
  event(ds, PREPARE, 0, 0, 0, 0);
  if (data.hour == 0)
    opeu_apply_daily_side_resources(ds);
  else if (data.hour == 12) {
    opeu_decrement_record_counters(ds);
    event(ds, NOON_EVENTS, 0, 0, 0, 0);
  }
}

// The services of the half-day dispatch as direct calls.
uint16_t svc_halfday(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3) {
  (void)a0;
  (void)a1;
  (void)a2;
  (void)a3;
  switch (id) {
  case PREPARE:     opeu_turn_prepare(ds, (const DialogHost*)port_dialogs_host()); break;
  case NOON_EVENTS: {
    opeu_turn_noon_events(ds, port_far(FAR_TEXT_SEGMENT, 0), svc_noonevt, NOON_CODE_SEGMENT);
    break;
  }
  default: port_unhandled("halfday", id, 4);
  }
  return 0;
}
