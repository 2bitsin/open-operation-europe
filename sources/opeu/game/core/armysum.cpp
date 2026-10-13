// The army summary of the current side's panel: element totals, supply percentages, influence, cities, air strength
// and the headquarters regiment.
#include "core/armysum.hpp"
#include "common/events.hpp"
#include "core/objcount.hpp"
#include "core/selunit.hpp"
#include "map/cellflag.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/aggpct.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

// The services the summary calls through its event (FORMAT is raised by opeu_unit_name with this event).
enum Service : std::uint16_t {
  TITLE    = 0, // (text)
  TEXT     = 1, // (text)
  FORMAT   = 2, // (destination, format, words...)
  PRINT    = 3, // (format, words...)
  POSITION = 4, // (x, y)
};

constexpr std::uint16_t OUT_OF_SUPPLY{ offsetof(DataSegment, army_out) };
constexpr std::uint16_t ROW0{ offsetof(DataSegment, army_row0) };
constexpr std::uint16_t ROW1{ offsetof(DataSegment, army_row1) };
constexpr std::uint16_t ROW2{ offsetof(DataSegment, army_row2) };
constexpr std::uint16_t ROW3{ offsetof(DataSegment, army_row3) };
constexpr std::uint16_t ROW4{ offsetof(DataSegment, army_row4) };
constexpr std::uint16_t ROW5{ offsetof(DataSegment, army_row5) };
constexpr std::uint16_t HQ_FORMAT{ offsetof(DataSegment, army_hq) };
constexpr std::uint8_t  CATEGORIES{ 8 };
constexpr std::uint16_t ROW_X{ 45 };

constexpr auto Low(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value);
}
constexpr auto High(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value >> 16);
}
constexpr std::uint16_t NAME_BYTES{ 0x50 }; // the headquarters regiment's name
} // namespace

// Shown on a side the player controls (or when neither side is player-controlled). The headquarters regiment's name
// is written into the routine's reserve for the print service.
void opeu_army_summary(Game& game, HostEvent event) {
  ServiceLocals const locals{ NAME_BYTES };
  auto&               data{ Data(game) };
  auto const          side{ data.current_side };
  if ((data.side_control[1] || data.side_control[0]) && data.side_control[side] != SIDE_PLAYED) return;
  opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), data.side_names[side]);
  if (!opeu_unit_support_condition(game, opeu_find_side_selected_unit(game, side))) {
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), OUT_OF_SUPPLY);
    return;
  }
  std::array<std::uint32_t, CATEGORIES> totals{};
  std::uint8_t                          counted{};
  for (std::uint8_t category{ 0 }; category < CATEGORIES; ++category) totals[category] = opeu_side_quantity(game, side, category, &counted);
  std::uint16_t ammunition{};
  std::uint16_t fuel{};
  std::uint16_t food{};
  opeu_side_resource_percentages(game, side, &ammunition, &fuel, &food, 1);
  std::uint16_t influence{ 0 };
  for (std::uint8_t y{ 0 }; y < MAP_HEIGHT; y += 4)
    for (std::uint8_t x{ 0 }; x < MAP_WIDTH; x += 4)
      if (opeu_cell_influence_side(game, x, y) == side) ++influence;
  auto const cities{ opeu_count_active_objectives(game, side) };
  auto const row{ [&](std::uint16_t y) { opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ROW_X), static_cast<int16_t>(y)); } };
  row(163);
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(ROW0), static_cast<std::uint16_t>(Low(totals[3])), static_cast<std::uint16_t>(High(totals[3])),
                                                        static_cast<std::uint16_t>(Low(totals[0])), static_cast<std::uint16_t>(High(totals[0])), static_cast<std::uint16_t>(fuel) });
  row(181);
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(ROW1), static_cast<std::uint16_t>(Low(totals[2])), static_cast<std::uint16_t>(High(totals[2])),
                                                        static_cast<std::uint16_t>(Low(totals[5])), static_cast<std::uint16_t>(High(totals[5])), static_cast<std::uint16_t>(ammunition) });
  row(199);
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(ROW2), static_cast<std::uint16_t>(Low(totals[1])), static_cast<std::uint16_t>(High(totals[1])),
                                                        static_cast<std::uint16_t>(Low(totals[7])), static_cast<std::uint16_t>(High(totals[7])), static_cast<std::uint16_t>(food) });
  row(217);
  opeu_cursor_print(game, std::array<std::uint16_t, 5>{ static_cast<std::uint16_t>(ROW3), static_cast<std::uint16_t>(Low(totals[4])), static_cast<std::uint16_t>(High(totals[4])),
                                                        static_cast<std::uint16_t>(Low(totals[6])), static_cast<std::uint16_t>(High(totals[6])) });
  auto const& own{ Record<Side>(data, data.current_side_ptr) };
  row(245);
  opeu_cursor_print(
      game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(ROW4), static_cast<std::uint16_t>(own.bombers), static_cast<std::uint16_t>(cities), static_cast<std::uint16_t>(influence) });
  row(263);
  opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(ROW5), static_cast<std::uint16_t>(own.planes), static_cast<std::uint16_t>(own.paratroops),
                                                        static_cast<std::uint16_t>(own.strength) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(54), static_cast<int16_t>(138));
  opeu_cursor_print(
      game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(HQ_FORMAT), static_cast<std::uint16_t>(opeu_unit_name(game, event, opeu_find_side_selected_unit(game, side), locals.At(0))) });
}

uint16_t svc_armysum(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case TITLE:    opeu_army_title(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case TEXT:     opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case FORMAT:   opeu_request_format(game, { args, count }); break;
  case PRINT:    opeu_cursor_print(game, { args, count }); break;
  case POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  default:       port_unhandled("armysum", id, count);
  }
  return 0;
}
