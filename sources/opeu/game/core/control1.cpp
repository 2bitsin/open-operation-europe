// Turn control of a side the player commands: the command menu of the current unit's side, and the map inspection
// mode the menu's cancel toggles to, until the side's delay ends the turn.
#include "core/control1.hpp"
#include "common/events.hpp"
#include "core/context.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/advmenu.hpp"
#include "core/armysum.hpp"
#include "core/coords.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the loop calls through its event; the context panel runs through context_event.
enum Service : std::uint16_t {
  PREPARE     = 0,  // ()
  BEGIN       = 1,  // ()
  POSITION    = 2,  // (x, y) the map view
  LOCATION    = 3,  // (x, y, side)
  SUMMARY     = 4,  // () the army summary
  ACTIVE      = 5,  // ()
  FORMAT      = 6,  // (buffer, format, general)
  MENU        = 7,  // (prompt, x, y, columns, general, menu) -> the entry chosen, 0xffff on cancel
  MESSAGE     = 8,  // (text, general)
  COORDINATES = 9,  // (&x, &y) -> nonzero while the player keeps inspecting
  ACTION      = 10, // (offset, segment) the far routine of a menu entry
};

constexpr std::uint16_t PROMPT_FORMAT{ offsetof(DataSegment, control_prompt) };
constexpr std::uint16_t READY_TEXT{ offsetof(DataSegment, control_ready) };
constexpr std::uint16_t COMMAND_MENU{ offsetof(DataSegment, control_menu) };
constexpr std::uint16_t CANCELED{ 0xffff };
constexpr std::uint8_t  NO_SIDE{ 0xff };
// The scratch buffer the caller reserves: the prompt text, then the inspected cell's y and x words.
constexpr std::uint16_t SCRATCH_Y{ 50 };
constexpr std::uint16_t SCRATCH_X{ 52 };
} // namespace

void opeu_turn_control_one(uint8_t ds[65536], HostEvent event, HostEvent context_event, uint16_t scratch) {
  auto&      data{ Data(ds) };
  auto const at_x{ static_cast<std::uint16_t>(scratch + SCRATCH_X) };
  auto const at_y{ static_cast<std::uint16_t>(scratch + SCRATCH_Y) };
  bool       inspecting{ false };
  data.context_side  = NO_SIDE;
  data.view_side     = NO_SIDE;
  data.overview_side = NO_SIDE;
  Call(ds, event, PREPARE);
  Call(ds, event, BEGIN);
  while (!data.side_delays[data.current_side]) {
    {
      auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
      Call(ds, event, POSITION, unit.x, unit.y);
      Call(ds, event, LOCATION, unit.x, unit.y, data.current_side);
    }
    opeu_daily_context(ds, context_event, data.current_side);
    for (;;) {
      Call(ds, event, SUMMARY);
      data.control_selection = 0;
      Call(ds, event, ACTIVE);
      opeu_turn_pre_context(ds, context_event);
      if (!inspecting) {
        Call(ds, event, FORMAT, scratch, PROMPT_FORMAT, data.context_record1);
        auto const choice{ Call(ds, event, MENU, scratch, 0, 10, 1, data.context_record2, COMMAND_MENU) };
        if (choice != CANCELED) {
          auto const action{ data.control_actions[choice] };
          Call(ds, event, ACTION, action.offset, action.segment);
          break;
        }
        inspecting             = true;
        data.control_selection = 0;
      } else {
        inspecting             = false;
        data.control_selection = 0;
        auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
        SetWord(ds, at_x, unit.x);
        SetWord(ds, at_y, unit.y);
        do Call(ds, event, MESSAGE, READY_TEXT, data.context_record2);
        while (Call(ds, event, COORDINATES, at_x, at_y));
      }
    }
  }
  data.control_selection = 0;
}

uint16_t svc_control1(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case PREPARE:  WithStack(PORT_FRAME, [&](std::uint16_t sp) { opeu_control_prepare(ds, svc_advmenu, sp); }); break;
  case BEGIN:    opeu_control_begin(ds, svc_soundcue); break;
  case POSITION: opeu_control_position(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOCATION:
    WithStack(PORT_FRAME, [&](std::uint16_t sp) {
      opeu_arrival_location(ds, svc_mappos, static_cast<std::uint16_t>(sp - 2), static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)),
                            static_cast<std::uint8_t>(Arg(args, count, 2)));
    });
    break;
  case SUMMARY: WithStack(PORT_FRAME, [&](std::uint16_t sp) { opeu_army_summary(ds, svc_armysum, static_cast<std::uint16_t>(sp - 2 - 0x50)); }); break;
  case ACTIVE:  opeu_turn_active_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case FORMAT:  opeu_request_format(ds, { args, count }); break;
  case MENU:
    result = opeu_control_choose(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4),
                                 Arg(args, count, 5));
    break;
  case MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case COORDINATES:
    result = WithStack(PORT_FRAME, [&](std::uint16_t sp) { return opeu_control_coordinates(ds, svc_coords, static_cast<std::uint16_t>(sp - 2), Arg(args, count, 0), Arg(args, count, 1)); });
    break;
  case ACTION: result = static_cast<uint16_t>(port_call(ds, static_cast<std::uint32_t>(Arg(args, count, 1)) * 16u + Arg(args, count, 0), nullptr, 0)); break;
  default:     port_unhandled("control1", id, count);
  }
  return result;
}
