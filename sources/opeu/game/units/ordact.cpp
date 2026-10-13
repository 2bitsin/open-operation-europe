// The map cursor's unit action: a ready, selectable regiment becomes the listed row; the order menu repeats until an
// order is chosen (a repair needs repair crews), then a target cell is picked and checked like in the Orders command.
// The accepted order is announced and stored, the side's daily context refreshed and the screen waits; leaving the
// menu cancels pending orders after confirmation. Both exits but a cancelled pick reset the cached control cell. The
// picked cell and the message text stay in the data segment, where the services read and write them.
#include "units/ordact.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/order-dialog.hpp"
#include "units/orders.hpp"
#include "units/ordunit.hpp"
#include "units/stats.hpp"

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/mappick.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordissue.hpp"

using namespace opeu::game;

namespace {
// The locals the services read and write, as offsets below the top of the routine's reserve.
constexpr std::uint16_t LOCALS_BYTES{ 0x74 };
enum ServiceLocal : std::uint16_t {
  PICKED_X = 0x08,
  PICKED_Y = 0x0c,
  BUFFER   = 0x74, // the message text
};
// Where the order menu's speaker is shown.
constexpr std::uint16_t MENU_X{ 0x36 };
constexpr std::uint16_t MENU_Y{ 0x144 };
// unit_quantity's category of repair crews.
constexpr std::uint8_t REPAIR_CREWS{ 6 };
// The ticks the screen waits after an order.
constexpr std::uint16_t ORDER_DELAY{ 0xf };
// The value of the cached control cell that forces a redraw.
constexpr std::uint8_t NO_CELL{ 0xff };

} // namespace

// unit: the regiment under the cursor.
void opeu_unit_action(Game& game, HostEvent event, uint16_t unit, uint16_t /*unused*/) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ LOCALS_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(LOCALS_BYTES - offset)); } };
  auto const          x_at{ local(PICKED_X) };
  auto const          y_at{ local(PICKED_Y) };
  auto const          buffer{ local(BUFFER) };
  std::uint8_t        crews{};
  auto&               regiment{ Record<Unit>(data, unit) };
  if (!(regiment.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, unit)) return;
  auto const person{ opeu_find_group_record(game, data.current_side, opeu_unit_group_index(unit)) };
  data.list_row = opeu_unit_index(unit);
  Call(game, event, ORDERS_CONTROL_PANEL);
  auto const&  prompts{ data.action_prompts };
  std::uint8_t kind{};
  auto const   done{ [&] {
    data.overview_side = NO_CELL;
    data.view_side     = NO_CELL;
  } };
  for (;;) {
    Call(game, event, ORDERS_CONTROL_MESSAGE, Near(data, data.awaiting_orders), person);
    Call(game, event, ORDERS_SPEAKER_PANEL, person, 1);
    auto const menu{ OrderMenuKind(data.list_row) };
    auto const count{ OrderCount(data.action_order_codes[menu]) };
    opeu_order_layout(game, menu);
    Call(game, event, ORDERS_ARMY_POSITION, MENU_X, MENU_Y);
    auto const choice{ Call(game, event, ORDERS_PROMPT, Near(data, prompts.your_orders), count, Near(data, data.order_menus[menu])) };
    Call(game, event, ORDERS_SPEAKER_PANEL, person, 0);
    if (choice == MENU_CANCEL) {
      opeu_cancel_unit_orders(game, event, unit, person);
      done();
      return;
    }
    kind = data.action_order_codes[menu][choice - 1];
    if (kind == Kind(UnitOrder::REPAIR_CITY) && !opeu_unit_quantity(game, unit, REPAIR_CREWS, &crews)) {
      Call(game, event, ORDERS_CONTROL_MESSAGE, Near(data, prompts.no_repair_unit), person);
      continue;
    }
    break;
  }
  std::uint16_t target{};
  for (;;) {
    Byte(game, x_at) = regiment.x;
    Byte(game, y_at) = regiment.y;
    Call(game, event, ORDERS_CONTROL_MESSAGE, Near(data, prompts.select_target), person);
    if (!Call(game, event, ORDERS_PICK_LOCATION, x_at, y_at)) {
      opeu_cancel_unit_orders(game, event, unit, person);
      return;
    }
    if (CheckOrderTarget(game, event, prompts, buffer, person, unit, Byte(game, x_at), Byte(game, y_at), kind, target)) break;
    Call(game, event, ORDERS_CONTROL_MESSAGE, buffer, person);
    Call(game, event, ORDERS_CONTROL_POSITION, regiment.x, regiment.y);
    Call(game, event, ORDERS_ARRIVAL_LOCATION, regiment.x, regiment.y, data.current_side);
  }
  AnnounceOrder(game, event, prompts, buffer, person, kind);
  opeu_set_unit_order(game, unit, target, Byte(game, x_at), 0, kind, Byte(game, y_at));
  Call(game, event, ORDERS_DAILY_CONTEXT, data.current_side);
  Call(game, event, ORDERS_DELAY_UNITS, ORDER_DELAY);
  done();
}

// The action's calls are order dialog services; confirm_message comes from opeu_cancel_unit_orders, which raises it
// through this event.
uint16_t svc_ordact(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case ORDERS_REQUEST_FORMAT:
  case ORDERS_CONTROL_MESSAGE:
  case ORDERS_CONFIRM_MESSAGE:
  case ORDERS_ARRIVAL_LOCATION:
  case ORDERS_PICK_LOCATION:
  case ORDERS_CONTROL_PANEL:
  case ORDERS_SPEAKER_PANEL:
  case ORDERS_ARMY_POSITION:
  case ORDERS_PROMPT:
  case ORDERS_CONTROL_POSITION:
  case ORDERS_MESSAGE_LOOKUP:
  case ORDERS_DELAY_DIALOG:
  case ORDERS_DELAY_UNITS:
  case ORDERS_DAILY_CONTEXT:    svc_orders(game, id, args, count, &r); break;
  default:                      port_unhandled("ordact", id, count);
  }
  return r;
}

// The services the order dialogs and their helpers share (OrdersService); 0 when the id is not one of them.
int svc_orders(Game& game, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result) {
  uint16_t r = 0;
  switch (id) {
  case ORDERS_REQUEST_FORMAT:   opeu_request_format(game, { args, count }); break;
  case ORDERS_CONTROL_MESSAGE:  opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ORDERS_CONFIRM_MESSAGE:  r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ORDERS_ARRIVAL_LOCATION: opeu_arrival_location(game, svc_mappos, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ORDERS_PICK_LOCATION:    r = opeu_map_pick_location(game, svc_mappick, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ORDERS_CONTROL_PANEL:    opeu_title_panel_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case ORDERS_GROUP_SHOW:       opeu_group_show(game, svc_mapunit, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case ORDERS_LIST_DIALOG:      r = opeu_list_dialog(game, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ORDERS_UNIT_ORDER_MENU:  opeu_unit_order_menu(game, svc_ordissue, (uint8_t)Arg(args, count, 0)); break;
  case ORDERS_SPEAKER_PANEL:    r = opeu_speaker_panel(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ORDERS_UNIT_ANNOUNCE:    opeu_unit_announce(game, svc_mapunit, Arg(args, count, 0)); break;
  case ORDERS_ARMY_POSITION:    opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case ORDERS_PROMPT:           r = opeu_prompt_value_cancel(game, svc_keyboard, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case ORDERS_CONTROL_POSITION: opeu_control_position(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ORDERS_MESSAGE_LOOKUP:   r = opeu_message_lookup(game, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case ORDERS_DELAY_DIALOG:     r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case ORDERS_DELAY_UNITS:      opeu_delay_units(game, (uint8_t)Arg(args, count, 0)); break;
  case ORDERS_DAILY_CONTEXT:    opeu_daily_context(game, svc_context, (uint8_t)Arg(args, count, 0)); break;
  default:                      return 0;
  }
  *result = r;
  return 1;
}
