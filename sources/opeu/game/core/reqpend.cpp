// A side's request reaching headquarters: accepted and booked with its delay, or turned down.
#include "core/reqpend.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/reqcheck.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <utility>

using namespace opeu::game;

namespace {

// The daily dialog of the request reports.
constexpr uint16_t REQUEST_SHOW{ 0x73 };
// The message that a request was accepted, and the first of the seven that it was turned down.
constexpr uint16_t ACCEPTED_MESSAGE{ 64 };
constexpr uint16_t REFUSED_MESSAGE{ 65 };
constexpr uint16_t REFUSAL_COUNT{ 7 };

// Formats message `id` with the name of the pending request's kind into the request buffer.
void FormatPending(Game& game, uint16_t id) {
  auto&                         data{ Data(game) };
  auto const                    name{ data.request_kind_names[Record<RequestSlot>(data, data.pending_request).kind] };
  auto const                    text{ opeu_message_lookup(game, id) };
  std::array<uint16_t, 3> const words{ static_cast<uint16_t>(offsetof(DataSegment, request_buffer)), text, name };
  opeu_request_format(game, words);
}

// Shows the request buffer from the side's chief of staff; it closes by itself when no side is played by a person.
void ShowPending(Game& game) {
  auto& data{ Data(game) };
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, request_buffer), data.request_template);
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_COMPUTER) opeu_pending_finish(game, 3);
}

} // namespace

// The daily dialog's report that the pending request was accepted.
void opeu_pending_start_present(Game& game) {
  opeu_pending_prepare(game);
  FormatPending(game, ACCEPTED_MESSAGE);
  ShowPending(game);
}

// Accepts the side's request: reports it, books it in the side's request configuration with a delay of one to three
// times its kind's base in days, and empties the request slot.
void opeu_request_start(Game& game, uint8_t side, uint16_t request) {
  auto& data{ Data(game) };
  auto& slot{ Record<RequestSlot>(data, request) };
  data.pending_request = request;
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), REQUEST_SHOW, side, Presenter::REQUEST_START);
  auto& config{ data.request_configs[side] };
  config.target   = slot.target;
  config.argument = slot.argument;
  config.kind     = slot.kind;
  auto const base{ data.request_delay_bases[config.kind] };
  config.delay = static_cast<uint8_t>(RandomBelow(game, base * 2) + base);
  slot.kind    = std::to_underlying(RequestKind::NONE);
}

// The daily dialog's report that the pending request was turned down, in one of seven wordings.
void opeu_pending_fallback_present(Game& game) {
  FormatPending(game, REFUSED_MESSAGE + RandomBelow(game, REFUSAL_COUNT));
  opeu_pending_prepare(game);
  ShowPending(game);
}

// Turns the side's request down: reports it, drops the marks of a requested regiment and empties the request slot.
void opeu_request_fallback(Game& game, uint8_t side, uint16_t request) {
  auto& data{ Data(game) };
  auto& slot{ Record<RequestSlot>(data, request) };
  data.pending_request = request;
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), REQUEST_SHOW, side, Presenter::REQUEST_FALLBACK);
  if (slot.kind == std::to_underlying(RequestKind::REGIMENT)) opeu_request_complete(game, side);
  slot.kind = std::to_underlying(RequestKind::NONE);
}
