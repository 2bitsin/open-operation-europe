// The request menu (turn action 7): the table of the side's requests and the choice of supplies, a battalion or a
// regiment.
#include "core/rqmenu.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/rqcommon.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

// The table: its left column and header line in pixels, the first row and the row height.
constexpr uint16_t TABLE_X{ 0x2d };
constexpr uint16_t HEADER_Y{ 0xa3 };
constexpr uint16_t FIRST_ROW_Y{ 0xbf };
constexpr uint16_t ROW_HEIGHT{ 0x12 };
constexpr uint16_t CHOICES{ 3 };           // supplies, battalion, regiment
constexpr uint16_t TYPE_RECORD_WORDS{ 7 }; // the words of an equipment type record a filed battalion row prints

} // namespace

// "Request   Dest     Type      Wait" at the top of the request table.
void opeu_request_header(Game& game, HostEvent event) {
  Call(game, event, RequestMenuService::POSITION, TABLE_X, HEADER_Y);
  Call(game, event, RequestMenuService::PRINT, offsetof(DataSegment, rq_header_text));
}

// The request menu. Each kind the side may request now (its booked request is of that kind: the request under
// preparation) or has filed gets a row: the destination, type and wait of the one under preparation, or the filed
// one. A choice that is under preparation is refused; supplies and battalions need a city of the side. The chosen
// request's routine runs, and the menu returns until the side's turn is used up.
void opeu_turn_action_7(Game& game, HostEvent event) {
  auto&       data{ Data(game) };
  auto const& config{ data.request_configs[data.current_side] };
  uint16_t    preparing[CHOICES];
  for (uint16_t kind = 0; kind < CHOICES; ++kind) preparing[kind] = Call(game, event, RequestMenuService::MODE_IS, kind);
  auto const filed{ [&](RequestKind kind) -> RequestSlot const& { return Record<Side>(data, data.current_side_ptr).requests[std::to_underlying(kind)]; } };
  auto const shown{ [&](RequestKind kind) { return Call(game, event, RequestMenuService::PENDING, std::to_underlying(kind)) || preparing[std::to_underlying(kind)]; } };
  do {
    uint16_t rows = 0;
    Call(game, event, RequestMenuService::TITLE, offsetof(DataSegment, rq_title_text));
    if (shown(RequestKind::SUPPLIES)) {
      opeu_request_header(game, event);
      Call(game, event, RequestMenuService::POSITION, TABLE_X, FIRST_ROW_Y);
      if (preparing[0])
        Call(game, event, RequestMenuService::PRINT, offsetof(DataSegment, rq_supply_row_text), config.target, config.delay);
      else
        Call(game, event, RequestMenuService::PRINT, offsetof(DataSegment, rq_supply_filed_text), filed(RequestKind::SUPPLIES).target);
      rows++;
    }
    if (shown(RequestKind::BATTALION)) {
      opeu_request_header(game, event);
      Call(game, event, RequestMenuService::POSITION, TABLE_X, rows * ROW_HEIGHT + FIRST_ROW_Y);
      if (preparing[1]) {
        Call(game, event, RequestMenuService::PRINT, offsetof(DataSegment, rq_battalion_row_text), config.target, data.element_types[config.argument].name, config.delay);
      } else {
        // The row prints the type's name; the whole type record is passed by value, as seven words.
        auto const& request{ filed(RequestKind::BATTALION) };
        auto const  type_at{ Near(data, data.element_types[request.argument]) };
        uint16_t    words[2 + TYPE_RECORD_WORDS]{ static_cast<uint16_t>(offsetof(DataSegment, rq_battalion_filed_text)), request.target };
        for (uint16_t i = 0; i < TYPE_RECORD_WORDS; ++i) words[2 + i] = Word(game, static_cast<uint16_t>(type_at + 2 * i));
        event(game, std::to_underlying(RequestMenuService::PRINT), words, static_cast<uint16_t>(std::size(words)));
      }
      rows++;
    }
    if (shown(RequestKind::REGIMENT)) {
      opeu_request_header(game, event);
      Call(game, event, RequestMenuService::POSITION, TABLE_X, rows * ROW_HEIGHT + FIRST_ROW_Y);
      if (preparing[2])
        Call(game, event, RequestMenuService::PRINT, offsetof(DataSegment, rq_regiment_row_text), data.rq_regiment_kind_names[config.argument], config.delay);
      else
        Call(game, event, RequestMenuService::PRINT, offsetof(DataSegment, rq_regiment_filed_text), data.rq_regiment_kind_names[filed(RequestKind::REGIMENT).argument]);
    }
    uint16_t choice;
    for (;;) {
      choice = Call(game, event, RequestMenuService::DELAY_DIALOG, offsetof(DataSegment, rq_menu_prompt_text), CHOICES, data.context_record2, offsetof(DataSegment, rq_menu));
      if (choice == MENU_CANCELLED) return;
      choice--;
      if (!preparing[choice]) break;
      Call(game, event, RequestMenuService::MESSAGE, offsetof(DataSegment, rq_busy_text), data.context_record3);
    }
    if (choice != std::to_underlying(RequestKind::REGIMENT) && !Call(game, event, RequestMenuService::HAS_CITY)) {
      Call(game, event, RequestMenuService::MESSAGE, offsetof(DataSegment, rq_no_city_text), data.context_record3);
    } else {
      auto const& handler{ data.request_handlers[choice] };
      Call(game, event, RequestMenuService::FAR_CALL, handler.offset, handler.segment);
    }
  } while (data.side_delays[data.current_side] == 0);
}

uint16_t svc_rqmenu(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_request("rqmenu", game, id, args, count);
}

auto port_routine_opeu_turn_action_7(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_7(game, svc_rqmenu);
  return 0;
}
