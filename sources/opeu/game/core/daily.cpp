// Dated events: for each side the player watches, every dated record of the side's nations that falls in the
// current month is activated and presented. The scan keeps its side and record in the data segment
// (daily_side, daily_record), where the services read them.
#include "core/daily.hpp"
#include "common/events.hpp"
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

constexpr std::uint16_t EVENT_PICTURE{ 0x73 };
} // namespace

void opeu_scan_daily_events(Game& game) {
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
        if (data.daily_pending) opeu_daily_context(game, data.daily_side);
        data.daily_active  = 0;
        data.daily_pending = 0;
        opeu_daily_activate(game, data.daily_side);
        opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), EVENT_PICTURE, data.daily_side, Presenter::DAILY_EVENT);
      }
    }
  }
}

void opeu_turn_daily(Game& game) {
  opeu_turn_prepare(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_scan_daily_events(game);
}
