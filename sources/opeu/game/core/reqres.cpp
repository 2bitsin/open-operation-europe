// Delivering a supplies request to an objective, and the services every delivery report uses.
#include "core/reqres.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/numeric.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr uint16_t SUPPLY_LIMIT{ 60000 }; // what an objective can stock of each supply
// The report's header and detail messages.
constexpr uint16_t HEADER_MESSAGE{ 72 };
constexpr uint16_t DETAIL_MESSAGE{ 73 };
// The header's first word for side 0 and side 1.
constexpr uint16_t HEADER_SIDE_WORD[]{ 54, 53 };

// 10000..40000 of a supply, up to the limit.
void Deliver(uint8_t* ds, uint16_t* stock) {
  opeu_add_word(RandomBelow(ds, 30001) + 10000, SUPPLY_LIMIT, stock);
}

} // namespace

// Delivers ammunition, fuel and food to the requested objective and reports the new stocks and what was added when a
// person plays the side.
void opeu_request_action0(uint8_t ds[65536], HostEvent event, uint8_t side, uint16_t request) {
  auto&          data{ Data(ds) };
  auto&          objective{ Record<Objective>(data, Record<RequestSlot>(data, request).target) };
  uint16_t const before[]{ objective.resources[0], objective.resources[1], objective.resources[2] };
  for (auto i = 0; i < 3; ++i) Deliver(ds, &objective.resources[i]);
  if (data.side_status[side] != SIDE_PLAYED) return;
  auto const  buffer{ static_cast<uint16_t>(offsetof(DataSegment, request_buffer)) };
  auto const* now{ objective.resources };
  Call(ds, event, DeliveryService::LOCATION, objective.x, objective.y, side);
  // The words are taken before the text is looked up.
  uint16_t const header_words[]{ HEADER_SIDE_WORD[side == 0 ? 0 : 1], Near(data, objective) };
  Call(ds, event, DeliveryService::FORMAT, buffer, Call(ds, event, DeliveryService::MESSAGE_TEXT, HEADER_MESSAGE), header_words[0], header_words[1]);
  Call(ds, event, DeliveryService::MESSAGE, buffer, data.request_person);
  Call(ds, event, DeliveryService::WAIT);
  uint16_t const detail_words[]{ now[1], static_cast<uint16_t>(now[1] - before[1]), now[0], static_cast<uint16_t>(now[0] - before[0]), now[2], static_cast<uint16_t>(now[2] - before[2]) };
  Call(ds, event, DeliveryService::FORMAT, buffer, Call(ds, event, DeliveryService::MESSAGE_TEXT, DETAIL_MESSAGE), detail_words[0], detail_words[1], detail_words[2], detail_words[3], detail_words[4],
       detail_words[5]);
  Call(ds, event, DeliveryService::MESSAGE, buffer, data.request_person);
}

uint16_t svc_delivery(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<DeliveryService>(id)) {
  case DeliveryService::LOCATION: {
    auto const frame{ port_frame_enter() };
    opeu_arrival_location(ds, svc_mappos, frame, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    port_frame_leave();
    break;
  }
  case DeliveryService::MESSAGE_TEXT: r = opeu_message_lookup(ds, svc_msglook, (short)Arg(args, count, 0)); break;
  case DeliveryService::FORMAT:       opeu_request_format(ds, { args, count }); break;
  case DeliveryService::MESSAGE:      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case DeliveryService::WAIT:         opeu_request_wait(ds, svc_soundcue); break;
  default:                            port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_reqres(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_delivery("reqres", ds, id, args, count);
}
