// The general assignment screens before play: the side's staff and division layout, the reinforcement and play-side
// list dialogs, and the per-side "Use these generals?" confirmation loop.
#include "units/genconf.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/counter.hpp"
#include "types.hpp"

#include "core/divstr.hpp"
#include "core/staffgen.hpp"
#include "core/staffpnl.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>

using namespace opeu::game;

namespace {
// The services the screens call through their event (svc_genconf).
enum Service : std::uint16_t {
  BACKDROP          = 0,  // (mode)
  POSITION          = 1,  // (x, y)
  PRINT             = 2,  // (format, ...)
  TEXT_COLOR        = 3,  // (colour)
  ATTRIBUTE         = 4,  // (value)
  OUTLINE           = 5,  // (x1, y1, x2, y2, colour)
  UI_TEXT           = 6,  // (text)
  LIST_DIALOG       = 7,  // (dialog, mode, height)
  COMMANDER_DIALOG  = 8,  // (dialog, mode, height)
  ASK_YES_NO        = 9,  // (text) -> answer
  RESTORE_BOTTOM    = 10, // ()
  RESTORE_LEFT      = 11, // ()
  ROLL_FLAG         = 12, // (person)
  REROLL_PERSON     = 13, // (slot)
  DIVISION_STRENGTH = 14, // (division)
};

// side_control: the side a human plays.
constexpr std::uint16_t HUMAN{ 1 };

// The first of the side's general records.
[[nodiscard]] auto SidePeople(DataSegment& data) -> Person* {
  return &data.people[data.current_side * PEOPLE_PER_SIDE];
}
} // namespace

// Lists the side's generals (up to the first empty record), clears their roles and draws the staff and division
// frames.
void opeu_general_screen(uint8_t ds[65536], HostEvent event) {
  auto&        data{ Data(ds) };
  auto*        person{ SidePeople(data) };
  std::uint8_t listed{ 0 };
  for (std::uint8_t slot{ 0 }; slot < PEOPLE_PER_SIDE && person->status != PERSON_VACANT; ++person, ++slot) {
    data.general_list[listed++] = Near(data, *person);
    opeu_set_record_counter(reinterpret_cast<std::uint8_t*>(person), 0);
    person->role &= static_cast<std::uint8_t>(~Bits(PersonRole::ROLE_MASK));
  }
  ++listed;
  data.play_dialog.rows      = listed;
  data.reinforce_dialog.rows = listed;
  Call(ds, event, BACKDROP, static_cast<std::uint8_t>(data.current_side == 0 ? 2 : 1));
  Call(ds, event, POSITION, 0x23, 8);
  Call(ds, event, TEXT_COLOR, data.current_side == 0 ? 6 : 5);
  Call(ds, event, PRINT, Near(data, data.side_title_format), data.side_titles[data.current_side]);
  Call(ds, event, TEXT_COLOR, 7);
  Call(ds, event, OUTLINE, 0x100, 0x28, 0x13f, 0x76, 7);
  Call(ds, event, POSITION, 0x29, 0x38);
  Call(ds, event, UI_TEXT, data.role_names[0]);
  Call(ds, event, OUTLINE, 0x60, 0x28, 0x9f, 0x76, 7);
  Call(ds, event, POSITION, 0x15, 0x38);
  Call(ds, event, UI_TEXT, data.role_names[1]);
  Call(ds, event, OUTLINE, 0x1a0, 0x28, 0x1df, 0x76, 7);
  Call(ds, event, POSITION, 0x3d, 0x38);
  Call(ds, event, UI_TEXT, data.role_names[2]);
  for (std::uint8_t division{ 0 }; division < GROUPS_PER_SIDE; ++division) {
    Call(ds, event, OUTLINE, division * 0xa0 + 0x10, 0x88, division * 0xa0 + 0x4f, 0xd6, 7);
    Call(ds, event, DIVISION_STRENGTH, division);
  }
}

// The reinforcement list with its mouse or keyboard help.
void opeu_reinforce_screen(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  data.shown_portrait = 0;
  Call(ds, event, ATTRIBUTE, 4);
  Call(ds, event, POSITION, 1, 0xe8);
  Call(ds, event, UI_TEXT, Near(data, data.reinforce_title));
  Call(ds, event, POSITION, 1, 0x170);
  Call(ds, event, UI_TEXT, data.input_mode ? Near(data, data.reinforce_mouse_help) : Near(data, data.reinforce_key_help));
  Call(ds, event, ATTRIBUTE, 0);
  Call(ds, event, LIST_DIALOG, Near(data, data.reinforce_dialog), 1, 3);
  Call(ds, event, RESTORE_BOTTOM);
  Call(ds, event, RESTORE_LEFT);
}

// The list of the generals who take the field.
void opeu_play_screen(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  Call(ds, event, ATTRIBUTE, 4);
  Call(ds, event, POSITION, 0x15, 0x170);
  Call(ds, event, UI_TEXT, Near(data, data.play_prompt));
  Call(ds, event, ATTRIBUTE, 0);
  Call(ds, event, COMMANDER_DIALOG, Near(data, data.play_dialog), 7, 1);
  Call(ds, event, RESTORE_BOTTOM);
}

// For each side: backs up its records, registers every listed general and, for a human side, repeats the screens
// until the player accepts the generals.
void opeu_confirm_generals(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  for (data.current_side = 0; data.current_side < 2; ++data.current_side) {
    std::copy_n(SidePeople(data), PEOPLE_PER_SIDE, data.people_backup.begin());
    auto* person{ SidePeople(data) };
    for (std::uint8_t slot{ 0 }; slot < PEOPLE_PER_SIDE && person->status != PERSON_VACANT; ++slot, ++person) {
      Call(ds, event, REROLL_PERSON, slot);
      Call(ds, event, ROLL_FLAG, Near(data, *person));
    }
    if (data.side_control[data.current_side] == HUMAN) {
      Call(ds, event, ATTRIBUTE, 4);
      do {
        opeu_general_screen(ds, event);
        opeu_reinforce_screen(ds, event);
        opeu_play_screen(ds, event);
        Call(ds, event, ATTRIBUTE, 4);
        Call(ds, event, POSITION, 0x14, 0x170);
      } while (!Call(ds, event, ASK_YES_NO, Near(data, data.use_generals_prompt)));
      Call(ds, event, ATTRIBUTE, 0);
    }
  }
}

// The services of the screens.
uint16_t svc_genconf(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case BACKDROP:         opeu_backdrop_fill_color(ds, (const UiHost*)port_uiprim_host(), (uint8_t)Arg(args, count, 0)); break;
  case POSITION:         opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:            r = lf_cursor_print(ds, args, count); break;
  case TEXT_COLOR:       opeu_text_color(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case ATTRIBUTE:        opeu_text_paper_set(ds, (TextHost*)port_textout_host(), (uint8_t)Arg(args, count, 0)); break;
  case OUTLINE:          opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case UI_TEXT:          opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case LIST_DIALOG:      r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case COMMANDER_DIALOG: r = opeu_commander_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ASK_YES_NO:
    // the prompt keeps a 16-byte answer buffer below the service's stack
    r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) -> std::uint16_t { return (uint16_t)opeu_prompt_yes_no(ds, svc_keyboard, Arg(args, count, 0), (uint16_t)(sp - 0x10u)); });
    break;
  case RESTORE_BOTTOM:    opeu_staff_restore_bottom(ds, svc_staffpnl); break;
  case RESTORE_LEFT:      opeu_staff_restore_left(ds, svc_staffpnl); break;
  case ROLL_FLAG:         opeu_staff_roll_flag(ds, Arg(args, count, 0)); break;
  case REROLL_PERSON:     opeu_staff_reroll_person(ds, (uint8_t)Arg(args, count, 0)); break;
  case DIVISION_STRENGTH: opeu_show_division_strength(ds, svc_divstr, (uint8_t)Arg(args, count, 0)); break;
  default:                port_unhandled("genconf", id, count);
  }
  return r;
}
