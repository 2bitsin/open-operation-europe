// Dated events: for each side the player watches, every dated record of the side's nations that falls in the
// current month is activated and presented. The scan keeps its side and record in the data segment
// (daily_side, daily_record), where the services read them.
#include "core/daily.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "link/glue.hpp"
#include "map/maperase.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum Service : std::uint16_t {
  PREPARE  = 0, // ()
  CONTEXT  = 1, // (side)
  ACTIVATE = 2, // (side)
  SHOW     = 3, // (picture, side, callback offset, callback segment)
};

constexpr std::uint16_t EVENT_PICTURE{ 0x73 };
// The far routine that presents the event (opeu_daily_present), as the segment the link table resolves.
constexpr std::uint16_t PRESENT_SEGMENT{ 0x0e36 };
} // namespace

void opeu_scan_daily_events(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  auto& data{ Data(game) };
  for (data.daily_side = 0; data.daily_side < SIDE_COUNT; ++data.daily_side) {
    if (data.side_status[data.daily_side] != SIDE_PLAYED) continue;
    data.daily_record = ELEMENT_TYPES_AT;
    for (std::uint8_t index{ 0 }; index < ELEMENT_TYPE_COUNT; data.daily_record = static_cast<std::uint16_t>(data.daily_record + sizeof(ElementType)), ++index) {
      auto const&        record{ Record<ElementType>(data, data.daily_record) };
      std::uint8_t const year{ static_cast<std::uint8_t>(record.date >> 4) };
      std::uint8_t const month{ static_cast<std::uint8_t>(record.date & 15) };
      if ((data.side_masks[data.daily_side] & record.sides) && year == data.year && month == data.month) {
        data.daily_active = 1;
        if (data.daily_pending) event(game, CONTEXT, data.daily_side, 0, 0, 0);
        data.daily_active  = 0;
        data.daily_pending = 0;
        event(game, ACTIVATE, data.daily_side, 0, 0, 0);
        event(game, SHOW, EVENT_PICTURE, data.daily_side, 0, PRESENT_SEGMENT);
      }
    }
  }
}

void opeu_turn_daily(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  event(game, PREPARE, 0, 0, 0, 0);
  opeu_scan_daily_events(game, event);
}

uint16_t svc_daily(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3) {
  switch (id) {
  case PREPARE:  opeu_turn_prepare(game, (const DialogHost*)port_dialogs_host()); break;
  case CONTEXT:  opeu_daily_context(game, svc_context, (uint8_t)a0); break;
  case ACTIVATE: opeu_daily_activate(game, svc_maperase, static_cast<std::uint8_t>(a0)); break;
  case SHOW:     opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), a0, (uint8_t)a1, (uint32_t)a3 << 16 | a2); break;
  default:       port_unhandled("daily", id, 4);
  }
  return 0;
}
