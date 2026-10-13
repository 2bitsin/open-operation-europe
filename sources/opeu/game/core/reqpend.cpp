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

#include <utility>

using namespace opeu::game;

namespace {

// The services the pending-request routines call through their event.
enum class PendingService : uint16_t {
  SHOW         = 0, // opeu_daily_show(id, side, present offset, present segment)
  MESSAGE_TEXT = 1, // opeu_message_lookup(id) -> near pointer of the text
  FORMAT       = 2, // request_format(destination, format, name)
  MESSAGE      = 3, // opeu_control_message(text, person)
  PREPARE      = 4, // opeu_pending_prepare
  FINISH       = 5, // opeu_pending_finish(value)
};

// The far addresses of opeu_pending_start_present and opeu_pending_fallback_present, which SHOW calls back.
constexpr uint16_t PRESENT_SEGMENT{ 0x1270 };
constexpr uint16_t START_PRESENT_OFFSET{ 0x430 };
constexpr uint16_t FALLBACK_PRESENT_OFFSET{ 0x4f0 };
// The daily dialog of the request reports.
constexpr uint16_t REQUEST_SHOW{ 0x73 };
// The message that a request was accepted, and the first of the seven that it was turned down.
constexpr uint16_t ACCEPTED_MESSAGE{ 64 };
constexpr uint16_t REFUSED_MESSAGE{ 65 };
constexpr uint16_t REFUSAL_COUNT{ 7 };

// Formats message `id` with the name of the pending request's kind into the request buffer.
void FormatPending(Game& game, PendingEvent event, uint16_t id) {
  auto&      data{ Data(game) };
  auto const name{ data.request_kind_names[Record<RequestSlot>(data, data.pending_request).kind] };
  auto const text{ opeu_message_lookup(game, static_cast<short>(id)) };
  CallWords(game, event, PendingService::FORMAT, offsetof(DataSegment, request_buffer), text, name);
}

// Shows the request buffer from the side's chief of staff; it closes by itself when no side is played by a person.
void ShowPending(Game& game) {
  auto& data{ Data(game) };
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, request_buffer), data.request_template);
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_COMPUTER) opeu_pending_finish(game, static_cast<uint8_t>(3));
}

} // namespace

// The daily dialog's report that the pending request was accepted.
void opeu_pending_start_present(Game& game, PendingEvent event) {
  opeu_pending_prepare(game, svc_soundcue);
  FormatPending(game, event, ACCEPTED_MESSAGE);
  ShowPending(game);
}

// Accepts the side's request: reports it, books it in the side's request configuration with a delay of one to three
// times its kind's base in days, and empties the request slot.
void opeu_request_start(Game& game, uint8_t side, uint16_t request) {
  auto& data{ Data(game) };
  auto& slot{ Record<RequestSlot>(data, request) };
  data.pending_request = request;
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), REQUEST_SHOW, static_cast<uint8_t>(side), static_cast<uint32_t>(PRESENT_SEGMENT) << 16 | START_PRESENT_OFFSET);
  auto& config{ data.request_configs[side] };
  config.target   = slot.target;
  config.argument = slot.argument;
  config.kind     = slot.kind;
  auto const base{ data.request_delay_bases[config.kind] };
  config.delay = static_cast<uint8_t>(RandomBelow(game, base * 2) + base);
  slot.kind    = std::to_underlying(RequestKind::NONE);
}

// The daily dialog's report that the pending request was turned down, in one of seven wordings.
void opeu_pending_fallback_present(Game& game, PendingEvent event) {
  FormatPending(game, event, REFUSED_MESSAGE + RandomBelow(game, REFUSAL_COUNT));
  opeu_pending_prepare(game, svc_soundcue);
  ShowPending(game);
}

// Turns the side's request down: reports it, drops the marks of a requested regiment and empties the request slot.
void opeu_request_fallback(Game& game, uint8_t side, uint16_t request) {
  auto& data{ Data(game) };
  auto& slot{ Record<RequestSlot>(data, request) };
  data.pending_request = request;
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), REQUEST_SHOW, static_cast<uint8_t>(side), static_cast<uint32_t>(PRESENT_SEGMENT) << 16 | FALLBACK_PRESENT_OFFSET);
  if (slot.kind == std::to_underlying(RequestKind::REGIMENT)) opeu_request_complete(game, side);
  slot.kind = std::to_underlying(RequestKind::NONE);
}

uint16_t svc_reqpend(Game& game, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  uint16_t r = 0;
  switch (static_cast<PendingService>(id)) {
  case PendingService::SHOW:         opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c); break;
  case PendingService::MESSAGE_TEXT: r = opeu_message_lookup(game, (short)a); break;
  case PendingService::FORMAT:       {
    uint16_t const words[]{ a, b, c };
    opeu_request_format(game, { words, 3 });
    break;
  }
  case PendingService::MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), a, b); break;
  case PendingService::PREPARE: opeu_pending_prepare(game, svc_soundcue); break;
  case PendingService::FINISH:  opeu_pending_finish(game, (uint8_t)a); break;
  default:                      port_unhandled("reqpend", id, 4);
  }
  return r;
}

auto port_routine_opeu_pending_fallback_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_pending_fallback_present(game, svc_reqpend);
  return 0;
}

auto port_routine_opeu_pending_start_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_pending_start_present(game, svc_reqpend);
  return 0;
}
