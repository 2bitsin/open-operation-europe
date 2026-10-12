// The map cursor's unit action: a ready, selectable regiment becomes the listed row; the order menu repeats until an
// order is chosen (a repair needs repair crews), then a target cell is picked and checked like in the Orders command.
// The accepted order is announced and stored, the side's daily context refreshed and the screen waits; leaving the
// menu cancels pending orders after confirmation. Both exits but a cancelled pick reset the cached control cell. The
// frame locals the services read and write through their addresses stay in the DS stack frame.
#include "types.hpp"
#include "units/ordact.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "map/support.hpp"
#include "units/order-dialog.hpp"
#include "units/orders.hpp"
#include "units/ordunit.hpp"
#include "units/stats.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "map/mappick.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "units/ordissue.hpp"

using namespace opeu::game;

namespace {
// Frame locals, as offsets below the frame pointer.
enum FrameLocal : std::uint16_t {
  PICKED_X = 0x08,
  PICKED_Y = 0x0c,
  COUNT    = 0x0e, // the menu choice; unit_quantity's count
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

auto Send(std::uint8_t* ds, HostEvent event, OrdersService service,
          auto... words) -> std::uint16_t {
  return Call(ds, event, static_cast<std::uint16_t>(service), words...);
}
} // namespace

// sp is the entry stack pointer; unit the regiment under the cursor.
void opeu_unit_action(uint8_t ds[65536], HostEvent event, uint16_t sp,
                      uint16_t unit, uint16_t unused) {
  (void)unused;
  auto&      data{ Data(ds) };
  auto const frame{ static_cast<std::uint16_t>(sp - 2) };
  auto const x_at{ Local(frame, PICKED_X) };
  auto const y_at{ Local(frame, PICKED_Y) };
  auto const count_at{ Local(frame, COUNT) };
  auto const buffer{ Local(frame, BUFFER) };
  auto&      regiment{ Record<Unit>(data, unit) };
  if (!(regiment.flags & Bits(UnitFlag::READY)) ||
      !opeu_unit_selection_eligible(ds, unit))
    return;
  auto const person{ opeu_find_group_record(ds, data.current_side,
                                            opeu_unit_group_index(unit)) };
  data.list_row = opeu_unit_index(unit);
  Send(ds, event, ORDERS_CONTROL_PANEL);
  auto const&  prompts{ data.action_prompts };
  std::uint8_t kind{};
  auto const   done{ [&] {
    data.overview_side = NO_CELL;
    data.view_side     = NO_CELL;
  } };
  for (;;) {
    Send(ds, event, ORDERS_CONTROL_MESSAGE, Near(data, data.awaiting_orders),
         person);
    Send(ds, event, ORDERS_SPEAKER_PANEL, person, 1);
    auto const menu{ OrderMenuKind(data.list_row) };
    auto const count{ OrderCount(data.action_order_codes[menu]) };
    opeu_order_layout(ds, menu);
    Send(ds, event, ORDERS_ARMY_POSITION, MENU_X, MENU_Y);
    auto const choice{ SetWord(ds, count_at,
                               Send(ds, event, ORDERS_PROMPT,
                                    Near(data, prompts.your_orders), count,
                                    Near(data, data.order_menus[menu]))) };
    Send(ds, event, ORDERS_SPEAKER_PANEL, person, 0);
    if (choice == MENU_CANCEL) {
      opeu_cancel_unit_orders(ds, event, unit, person);
      done();
      return;
    }
    kind = data.action_order_codes[menu][choice - 1];
    if (kind == Kind(OrderKind::REPAIR_CITY) &&
        !opeu_unit_quantity(ds, unit, REPAIR_CREWS, ds + count_at)) {
      Send(ds, event, ORDERS_CONTROL_MESSAGE,
           Near(data, prompts.no_repair_unit), person);
      continue;
    }
    break;
  }
  std::uint16_t target{};
  for (;;) {
    ds[x_at] = regiment.x;
    ds[y_at] = regiment.y;
    Send(ds, event, ORDERS_CONTROL_MESSAGE, Near(data, prompts.select_target),
         person);
    if (!Send(ds, event, ORDERS_PICK_LOCATION, x_at, y_at)) {
      opeu_cancel_unit_orders(ds, event, unit, person);
      return;
    }
    if (CheckOrderTarget(ds, event, prompts, buffer, person, unit, ds[x_at],
                         ds[y_at], kind, target))
      break;
    Send(ds, event, ORDERS_CONTROL_MESSAGE, buffer, person);
    Send(ds, event, ORDERS_CONTROL_POSITION, regiment.x, regiment.y);
    Send(ds, event, ORDERS_ARRIVAL_LOCATION, regiment.x, regiment.y,
         data.current_side);
  }
  AnnounceOrder(ds, event, prompts, buffer, person, kind);
  opeu_set_unit_order(ds, unit, target, ds[x_at], 0, kind, ds[y_at]);
  Send(ds, event, ORDERS_DAILY_CONTEXT, data.current_side);
  Send(ds, event, ORDERS_DELAY_UNITS, ORDER_DELAY);
  done();
}

// The action's calls are order dialog services; confirm_message comes from opeu_cancel_unit_orders, which raises it
// through this event.
uint16_t svc_ordact(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count) {
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
    case ORDERS_DAILY_CONTEXT:    svc_orders(ds, id, args, count, &r); break;
    default:                      port_unhandled("ordact", id, count);
  }
  return r;
}

// The services the order dialogs and their helpers share (OrdersService); 0 when the id is not one of them.
int svc_orders(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count,
               uint16_t* result) {
  uint16_t r = 0;
  switch (id) {
    case ORDERS_REQUEST_FORMAT: r = lf_request_format(ds, args, count); break;
    case ORDERS_CONTROL_MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case ORDERS_CONFIRM_MESSAGE:
      r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(),
                               Arg(args, count, 0), Arg(args, count, 1));
      break;
    case ORDERS_ARRIVAL_LOCATION:
      WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        opeu_arrival_location(
            ds, svc_mappos, static_cast<std::uint16_t>(sp - 2u),
            (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
            (uint8_t)Arg(args, count, 2));
      });
      break;
    case ORDERS_PICK_LOCATION:
      r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        return opeu_map_pick_location(ds, svc_mappick,
                                      static_cast<std::uint16_t>(sp - 2u),
                                      Arg(args, count, 0), Arg(args, count, 1));
      });
      break;
    case ORDERS_CONTROL_PANEL:
      opeu_title_panel_begin(ds, (const DialogHost*)port_dialogs_host());
      break;
    case ORDERS_GROUP_SHOW:
      WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        opeu_group_show(ds, svc_mapunit, static_cast<std::uint16_t>(sp - 2u),
                        Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
      });
      break;
    case ORDERS_LIST_DIALOG:
      r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0),
                           (uint8_t)Arg(args, count, 1),
                           (uint8_t)Arg(args, count, 2));
      break;
    case ORDERS_UNIT_ORDER_MENU:
      WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        opeu_unit_order_menu(ds, svc_ordissue, sp,
                             (uint8_t)Arg(args, count, 0));
      });
      break;
    case ORDERS_SPEAKER_PANEL:
      r = opeu_speaker_panel(ds, (const DialogHost*)port_dialogs_host(),
                             Arg(args, count, 0), Arg(args, count, 1));
      break;
    case ORDERS_UNIT_ANNOUNCE:
      WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        opeu_unit_announce(ds, svc_mapunit, static_cast<std::uint16_t>(sp - 2u),
                           Arg(args, count, 0));
      });
      break;
    case ORDERS_ARMY_POSITION:
      opeu_army_position(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0),
                         (int16_t)Arg(args, count, 1));
      break;
    case ORDERS_PROMPT:
      r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        return opeu_prompt_value_cancel(
            ds, svc_keyboard, Arg(args, count, 0), Arg(args, count, 1),
            Arg(args, count, 2), static_cast<std::uint16_t>(sp - 0x10u));
      });
      break;
    case ORDERS_CONTROL_POSITION:
      opeu_control_position(ds, Arg(args, count, 0), Arg(args, count, 1));
      break;
    case ORDERS_MESSAGE_LOOKUP:
      r = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0));
      break;
    case ORDERS_DELAY_DIALOG:
      r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(),
                            Arg(args, count, 0), Arg(args, count, 1),
                            Arg(args, count, 2), Arg(args, count, 3));
      break;
    case ORDERS_DELAY_UNITS:
      opeu_delay_units(ds, svc_waitkey, (uint8_t)Arg(args, count, 0));
      break;
    case ORDERS_DAILY_CONTEXT:
      opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0));
      break;
    default: return 0;
  }
  *result = r;
  return 1;
}
