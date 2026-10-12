// The scenario briefings: the six side initializers of the scenarios speak the opening briefing of the current side.
// The side's commander and then its staff officer read the briefing texts; one text is formatted with the player's
// name. While a target city is named, the message hook blinks a marker on its cell of the overview map.
#include "types.hpp"
#include "core/scenbrf.hpp"
#include "common/events.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "map/maperase.hpp"
#include "map/mappixel.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the briefings call through their event.
enum Service : std::uint16_t {
  ACTIVATE        = 0, // daily activate(side)
  CONTROL_MESSAGE = 1, // control message(text, speaker)
  OVERVIEW_PIXEL  = 2, // overview pixel(x pointer, y pointer)
  INVERT          = 3, // invert(x, y, width, height, mode)
  FORMAT          = 4, // format(destination, format, person)
};

// The code segment of the briefings: the message hook points at opeu_brief_blink, offset 0 in it.
constexpr std::uint16_t BRIEFING_CODE_SEGMENT{ 0x0d9b };

// One side's briefing in progress: who speaks, and the 100-byte buffer (`closing`, a near pointer the caller
// supplies) that receives the text formatted with the player's name.
class Briefing {
public:
  Briefing(std::uint8_t* ds, HostEvent event, std::uint16_t closing)
      : ds_{ ds }, event_{ event }, closing_{ closing } {}

  auto Activate(std::uint8_t side) -> void {
    Call(ds_, event_, ACTIVATE, side);
  }
  auto Commander() -> void {
    speaker_ = Data(ds_).initial_template4;
  }
  auto Staff() -> void {
    speaker_ = Data(ds_).initial_template5;
  }

  template <std::size_t N> auto Say(std::array<char, N> const& text) -> void {
    SayAt(Near(Data(ds_), text));
  }
  auto SayClosing() -> void {
    SayAt(closing_);
  }

  // Formats the text with the player's name into the closing buffer.
  template <std::size_t N>
  auto Format(std::array<char, N> const& text) -> void {
    auto const& data{ Data(ds_) };
    Call(ds_, event_, FORMAT, closing_, Near(data, text), data.initial_person);
  }

  auto HookOn() -> void {
    Data(ds_).message_hook = FarPointer{ 0, BRIEFING_CODE_SEGMENT };
  }
  auto HookOff() -> void {
    Data(ds_).message_hook = FarPointer{ 0, 0 };
  }
  auto Mark(std::uint8_t x, std::uint8_t y) -> void {
    opeu_brief_mark(ds_, event_, x, y);
  }

private:
  auto SayAt(std::uint16_t text) -> void {
    Call(ds_, event_, CONTROL_MESSAGE, text, speaker_);
  }

  std::uint8_t* ds_;
  HostEvent     event_;
  std::uint16_t closing_;
  std::uint16_t speaker_{};
};

} // namespace

// The message hook: inverts a 1x8 marker at the marked overview cell.
void opeu_brief_blink(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  Call(ds, event, INVERT, data.brief_mark_x, data.brief_mark_y, 1, 8, 0x107);
}

// Marks an overview-map cell for the blinking hook.
void opeu_brief_mark(uint8_t ds[65536], HostEvent event, uint8_t x, uint8_t y) {
  auto& data{ Data(ds) };
  data.brief_mark_x = x;
  data.brief_mark_y = y;
  Call(ds, event, OVERVIEW_PIXEL, offsetof(DataSegment, brief_mark_x),
       offsetof(DataSegment, brief_mark_y));
}

// France 1940.
void opeu_brief_france(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  Briefing    briefing{ ds, event, scratch };
  auto const& data{ Data(ds) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.text_5bb2);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1d, 0x18);
    briefing.Say(data.text_5c13);
    briefing.Mark(0x01, 0x0e);
    briefing.Say(data.text_5c4c);
    briefing.HookOff();
    briefing.Say(data.text_5c84);
    briefing.Say(data.text_5cab);
    briefing.Format(data.text_5cde);
    briefing.SayClosing();
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.text_5d1c);
    briefing.Say(data.text_5d5e);
    briefing.Say(data.text_5da4);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1d, 0x18);
    briefing.Say(data.text_5dde);
    briefing.Mark(0x01, 0x0e);
    briefing.Say(data.text_5e15);
    briefing.Mark(0x0a, 0x21);
    briefing.Say(data.text_5e56);
    briefing.HookOff();
    briefing.Say(data.text_5e83);
    briefing.Format(data.text_5ea9);
    briefing.SayClosing();
  }
}

// North Africa.
void opeu_brief_africa(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  Briefing    briefing{ ds, event, scratch };
  auto const& data{ Data(ds) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.text_5edb);
    briefing.Say(data.text_5f01);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1c, 0x04);
    briefing.Say(data.text_5f36);
    briefing.HookOff();
    briefing.Say(data.text_5f5c);
    briefing.HookOn();
    briefing.Mark(0x21, 0x0d);
    briefing.Say(data.text_5f81);
    briefing.Mark(0x21, 0x0a);
    briefing.Say(data.text_5fb0);
    briefing.HookOff();
    briefing.Say(data.text_5fdb);
    briefing.Format(data.text_6008);
    briefing.SayClosing();
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.text_603e);
    briefing.Say(data.text_6070);
    briefing.Say(data.text_60b5);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1c, 0x04);
    briefing.Say(data.text_60e2);
    briefing.Mark(0x21, 0x0a);
    briefing.Say(data.text_6115);
    briefing.Mark(0x21, 0x0d);
    briefing.Say(data.text_6149);
    briefing.HookOff();
    briefing.Say(data.text_617f);
    briefing.Format(data.text_61b3);
    briefing.SayClosing();
  }
}

// Kursk (Operation Citadel).
void opeu_brief_kursk(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  Briefing    briefing{ ds, event, scratch };
  auto const& data{ Data(ds) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.text_61d6);
    briefing.Say(data.text_621c);
    briefing.Say(data.text_6263);
    briefing.Say(data.text_62a3);
    briefing.Staff();
    briefing.Format(data.text_62d5);
    briefing.SayClosing();
    briefing.HookOn();
    briefing.Mark(0x1f, 0x24);
    briefing.Say(data.text_6300);
    briefing.Mark(0x13, 0x04);
    briefing.Say(data.text_6330);
    briefing.HookOff();
    briefing.Say(data.text_636d);
    briefing.Say(data.text_63ad);
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.text_63e7);
    briefing.Say(data.text_6420);
    briefing.Say(data.text_6454);
    briefing.Say(data.text_6492);
    briefing.Say(data.text_64cd);
    briefing.Staff();
    briefing.HookOn();
    briefing.Format(data.text_6507);
    briefing.SayClosing();
    briefing.Say(data.text_6534);
    briefing.HookOff();
    briefing.Say(data.text_656b);
    briefing.Say(data.text_659d);
    briefing.Say(data.text_65d7);
  }
}

// Normandy.
void opeu_brief_normandy(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  Briefing    briefing{ ds, event, scratch };
  auto const& data{ Data(ds) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.text_6612);
    briefing.Say(data.text_6639);
    briefing.Say(data.text_6658);
    briefing.Say(data.text_668c);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1c, 0x0d);
    briefing.Say(data.text_66cb);
    briefing.HookOff();
    briefing.Say(data.text_6708);
    briefing.Mark(0x0c, 0x02);
    briefing.Say(data.text_674a);
    briefing.HookOff();
    briefing.Format(data.text_6780);
    briefing.SayClosing();
    briefing.Say(data.text_67b7);
    briefing.Say(data.text_67e8);
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.text_682a);
    briefing.Say(data.text_686b);
    briefing.Say(data.text_68b6);
    briefing.Say(data.text_68cd);
    briefing.Staff();
    briefing.Say(data.text_68ff);
    briefing.HookOn();
    briefing.Mark(0x1c, 0x0d);
    briefing.Say(data.text_693a);
    briefing.Mark(0x0c, 0x02);
    briefing.Say(data.text_6982);
    briefing.Mark(0x12, 0x02);
    briefing.Say(data.text_69db);
    briefing.HookOff();
    briefing.Format(data.text_6a14);
    briefing.SayClosing();
  }
}

// The Ardennes (Operation Autumn Mist).
void opeu_brief_ardennes(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  Briefing    briefing{ ds, event, scratch };
  auto const& data{ Data(ds) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.text_6a5c);
    briefing.Say(data.text_6a91);
    briefing.Say(data.text_6ad5);
    briefing.Say(data.text_6b1a);
    briefing.Say(data.text_6b52);
    briefing.Staff();
    briefing.Say(data.text_6b9f);
    briefing.Say(data.text_6be5);
    briefing.HookOn();
    briefing.Mark(0x05, 0x11);
    briefing.Say(data.text_6c17);
    briefing.HookOff();
    briefing.Say(data.text_6c50);
    briefing.Format(data.text_6cab);
    briefing.SayClosing();
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.text_6ce6);
    briefing.Say(data.text_6d30);
    briefing.Say(data.text_6d52);
    briefing.Say(data.text_6d8f);
    briefing.Staff();
    briefing.Say(data.text_6db6);
    briefing.HookOn();
    briefing.Mark(0x05, 0x11);
    briefing.Say(data.text_6def);
    briefing.HookOff();
    briefing.Say(data.text_6e31);
    briefing.Say(data.text_6e76);
    briefing.Say(data.text_6ea7);
  }
}

// Berlin.
void opeu_brief_berlin(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  Briefing    briefing{ ds, event, scratch };
  auto const& data{ Data(ds) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.text_6ed5);
    briefing.Say(data.text_6ef1);
    briefing.Say(data.text_6f1f);
    briefing.Say(data.text_6f60);
    briefing.Say(data.text_6f99);
    briefing.Staff();
    briefing.Say(data.text_6fc2);
    briefing.HookOn();
    briefing.Mark(0x19, 0x15);
    briefing.Say(data.text_7008);
    briefing.Mark(0x03, 0x0a);
    briefing.Say(data.text_703f);
    briefing.HookOff();
    briefing.Say(data.text_707e);
    briefing.Format(data.text_709d);
    briefing.SayClosing();
    briefing.Say(data.text_70d8);
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.text_7113);
    briefing.Say(data.text_7140);
    briefing.Say(data.text_716c);
    briefing.Say(data.text_719d);
    briefing.Say(data.text_71e2);
    briefing.Staff();
    briefing.Format(data.text_7217);
    briefing.SayClosing();
    briefing.Say(data.text_7235);
    briefing.HookOn();
    briefing.Mark(0x19, 0x15);
    briefing.Say(data.text_726e);
    briefing.Mark(0x03, 0x0a);
    briefing.Say(data.text_728e);
    briefing.HookOff();
    briefing.Say(data.text_72bc);
    briefing.Say(data.text_730e);
  }
}

// The services in the native build.
uint16_t svc_scenbrf(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  uint16_t r = 0, f;
  switch (id) {
    case ACTIVATE:
      f = b1_frame_enter();
      opeu_daily_activate(ds, svc_maperase, f, (uint8_t)Arg(args, count, 0));
      port_frame_leave();
      break;
    case CONTROL_MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case OVERVIEW_PIXEL:
      opeu_overview_pixel(ds, Arg(args, count, 0), Arg(args, count, 1));
      break;
    case INVERT:
      r = (uint16_t)opeu_ui_highlight(Arg(args, count, 0), Arg(args, count, 1),
                                      Arg(args, count, 2), Arg(args, count, 3),
                                      Arg(args, count, 4));
      break;
    case FORMAT: b1_request_format(ds, args, count); break;
    default:     port_unhandled("scenbrf", id, count);
  }
  return r;
}

auto port_routine_opeu_brief_africa(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return port_run_briefing(ds, opeu_brief_africa);
}

auto port_routine_opeu_brief_ardennes(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return port_run_briefing(ds, opeu_brief_ardennes);
}

auto port_routine_opeu_brief_berlin(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return port_run_briefing(ds, opeu_brief_berlin);
}

auto port_routine_opeu_brief_blink(std::uint8_t* ds, std::uint16_t const* args,
                                   unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_blink(ds, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_france(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return port_run_briefing(ds, opeu_brief_france);
}

auto port_routine_opeu_brief_kursk(std::uint8_t* ds, std::uint16_t const* args,
                                   unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return port_run_briefing(ds, opeu_brief_kursk);
}

auto port_routine_opeu_brief_normandy(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return port_run_briefing(ds, opeu_brief_normandy);
}
