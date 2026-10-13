// The request menu (turn action 7): the table of the side's requests and the choice of supplies, a battalion or a
// regiment.
#include "core/rqmenu.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/dialogs.hpp"
#include "core/rqcommon.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "link/link.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
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
void opeu_request_header(Game& game) {
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(HEADER_Y));
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(offsetof(DataSegment, rq_header_text)) });
}

// The request menu. Each kind the side may request now (its booked request is of that kind: the request under
// preparation) or has filed gets a row: the destination, type and wait of the one under preparation, or the filed
// one. A choice that is under preparation is refused; supplies and battalions need a city of the side. The chosen
// request's routine runs, and the menu returns until the side's turn is used up.
void opeu_turn_action_7(Game& game) {
  auto&       data{ Data(game) };
  auto const& config{ data.request_configs[data.current_side] };
  uint16_t    preparing[CHOICES];
  for (uint16_t kind = 0; kind < CHOICES; ++kind) preparing[kind] = opeu_request_mode_is(game, static_cast<uint8_t>(kind));
  auto const filed{ [&](RequestKind kind) -> RequestSlot const& { return Record<Side>(data, data.current_side_ptr).requests[std::to_underlying(kind)]; } };
  auto const shown{ [&](RequestKind kind) { return opeu_request_pending(game, static_cast<uint8_t>(std::to_underlying(kind))) || preparing[std::to_underlying(kind)]; } };
  do {
    uint16_t rows = 0;
    opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, rq_title_text));
    if (shown(RequestKind::SUPPLIES)) {
      opeu_request_header(game);
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(FIRST_ROW_Y));
      if (preparing[0])
        opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(offsetof(DataSegment, rq_supply_row_text)), static_cast<std::uint16_t>(config.target),
                                                              static_cast<std::uint16_t>(config.delay) });
      else
        opeu_cursor_print(game,
                          std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, rq_supply_filed_text)), static_cast<std::uint16_t>(filed(RequestKind::SUPPLIES).target) });
      rows++;
    }
    if (shown(RequestKind::BATTALION)) {
      opeu_request_header(game);
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(rows * ROW_HEIGHT + FIRST_ROW_Y));
      if (preparing[1]) {
        opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(offsetof(DataSegment, rq_battalion_row_text)), static_cast<std::uint16_t>(config.target),
                                                              static_cast<std::uint16_t>(data.element_types[config.argument].name), static_cast<std::uint16_t>(config.delay) });
      } else {
        // The row prints the type's name; the whole type record is passed by value, as seven words.
        auto const&                                 request{ filed(RequestKind::BATTALION) };
        auto const                                  type_at{ Near(data, data.element_types[request.argument]) };
        std::array<uint16_t, 2 + TYPE_RECORD_WORDS> words{ static_cast<uint16_t>(offsetof(DataSegment, rq_battalion_filed_text)), request.target };
        for (uint16_t i = 0; i < TYPE_RECORD_WORDS; ++i) words[2 + i] = Word(game, static_cast<uint16_t>(type_at + 2 * i));
        opeu_cursor_print(game, words);
      }
      rows++;
    }
    if (shown(RequestKind::REGIMENT)) {
      opeu_request_header(game);
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(rows * ROW_HEIGHT + FIRST_ROW_Y));
      if (preparing[2])
        opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(offsetof(DataSegment, rq_regiment_row_text)),
                                                              static_cast<std::uint16_t>(data.rq_regiment_kind_names[config.argument]), static_cast<std::uint16_t>(config.delay) });
      else
        opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, rq_regiment_filed_text)),
                                                              static_cast<std::uint16_t>(data.rq_regiment_kind_names[filed(RequestKind::REGIMENT).argument]) });
    }
    uint16_t choice;
    for (;;) {
      choice = opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, rq_menu_prompt_text), CHOICES, data.context_record2, offsetof(DataSegment, rq_menu));
      if (choice == MENU_CANCELLED) return;
      choice--;
      if (!preparing[choice]) break;
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, rq_busy_text), data.context_record3);
    }
    if (choice != std::to_underlying(RequestKind::REGIMENT) && !opeu_side_has_city(game)) {
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, rq_no_city_text), data.context_record3);
    } else {
      auto const& handler{ data.request_handlers[choice] };
      port_call(game, static_cast<uint32_t>(static_cast<std::uint16_t>(handler.segment)) * 16u + static_cast<std::uint16_t>(handler.offset), 0, 0);
    }
  } while (data.side_delays[data.current_side] == 0);
}

auto port_routine_opeu_turn_action_7(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_7(game);
  return 0;
}
