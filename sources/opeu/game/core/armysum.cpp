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
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
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
constexpr std::uint8_t  MAP_SIZE{ 40 };
constexpr std::uint16_t ROW_X{ 45 };

constexpr auto Low(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value);
}
constexpr auto High(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value >> 16);
}
} // namespace

// Shown on a side the player controls (or when neither side is player-controlled). The headquarters regiment's name
// is formatted into the caller's buffer at `scratch`.
void opeu_army_summary(uint8_t ds[65537], HostEvent event, uint16_t scratch) {
  auto&      data{ Data(ds) };
  auto const side{ data.current_side };
  if ((data.side_control[1] || data.side_control[0]) && data.side_control[side] != 1) return;
  Call(ds, event, TITLE, data.side_names[side]);
  if (!opeu_unit_support_condition(ds, opeu_find_side_selected_unit(ds, side))) {
    Call(ds, event, TEXT, OUT_OF_SUPPLY);
    return;
  }
  std::array<std::uint32_t, CATEGORIES> totals{};
  std::uint8_t                          counted{};
  for (std::uint8_t category{ 0 }; category < CATEGORIES; ++category) totals[category] = opeu_side_quantity(ds, side, category, &counted);
  std::uint16_t ammunition{};
  std::uint16_t fuel{};
  std::uint16_t food{};
  opeu_side_resource_percentages(ds, side, &ammunition, &fuel, &food, 1);
  std::uint16_t influence{ 0 };
  for (std::uint8_t y{ 0 }; y < MAP_SIZE; y += 4)
    for (std::uint8_t x{ 0 }; x < MAP_SIZE; x += 4)
      if (opeu_cell_influence_side(ds, x, y) == side) ++influence;
  auto const cities{ opeu_count_active_objectives(ds, side) };
  auto const row{ [&](std::uint16_t y) { Call(ds, event, POSITION, ROW_X, y); } };
  row(163);
  Call(ds, event, PRINT, ROW0, Low(totals[3]), High(totals[3]), Low(totals[0]), High(totals[0]), fuel);
  row(181);
  Call(ds, event, PRINT, ROW1, Low(totals[2]), High(totals[2]), Low(totals[5]), High(totals[5]), ammunition);
  row(199);
  Call(ds, event, PRINT, ROW2, Low(totals[1]), High(totals[1]), Low(totals[7]), High(totals[7]), food);
  row(217);
  Call(ds, event, PRINT, ROW3, Low(totals[4]), High(totals[4]), Low(totals[6]), High(totals[6]));
  auto const& own{ Record<Side>(data, data.current_side_ptr) };
  row(245);
  Call(ds, event, PRINT, ROW4, own.bombers, cities, influence);
  row(263);
  Call(ds, event, PRINT, ROW5, own.planes, own.paratroops, own.strength);
  Call(ds, event, POSITION, 54, 138);
  Call(ds, event, PRINT, HQ_FORMAT, opeu_unit_name(ds, event, opeu_find_side_selected_unit(ds, side), scratch));
}

uint16_t svc_armysum(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case TITLE:    opeu_army_title(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case TEXT:     opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case FORMAT:   b2_request_format(ds, args, count); break;
  case PRINT:    b2_cursor_print(ds, args, count); break;
  case POSITION: opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  default:       port_unhandled("armysum", id, count);
  }
  return 0;
}
