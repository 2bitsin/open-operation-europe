// The general assignment screens before play: the side's staff and division layout, the reinforcement and play-side
// list dialogs, and the per-side "Use these generals?" confirmation loop.
#include "units/genconf.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/counter.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/divstr.hpp"
#include "core/staffgen.hpp"
#include "core/staffpnl.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>

using namespace opeu::game;

namespace {

// The first of the side's general records.
[[nodiscard]] auto SidePeople(DataSegment& data) -> Person* {
  return &data.people[data.current_side * PEOPLE_PER_SIDE];
}
} // namespace

// Lists the side's generals (up to the first empty record), clears their roles and draws the staff and division
// frames.
void opeu_general_screen(Game& game) {
  auto&        data{ Data(game) };
  auto*        person{ SidePeople(data) };
  std::uint8_t listed{ 0 };
  for (std::uint8_t slot{ 0 }; slot < PEOPLE_PER_SIDE && person->status != PERSON_VACANT; ++person, ++slot) {
    data.general_list[listed++] = Near(data, *person);
    opeu_set_record_counter(*person, 0);
    person->role &= static_cast<std::uint8_t>(~Bits(PersonRole::ROLE_MASK));
  }
  ++listed;
  data.play_dialog.rows      = listed;
  data.reinforce_dialog.rows = listed;
  opeu_backdrop_fill_color(game, static_cast<const UiHost*>(port_uiprim_host()), static_cast<uint8_t>(static_cast<std::uint8_t>(data.current_side == 0 ? 2 : 1)));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x23), static_cast<int16_t>(8));
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), SideTextColor(data.current_side));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.side_title_format)), static_cast<std::uint16_t>(data.side_titles[data.current_side]) });
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), 7);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x100, 0x28, 0x13f, 0x76, 7);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x29), static_cast<int16_t>(0x38));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), data.role_names[0]);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x60, 0x28, 0x9f, 0x76, 7);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x15), static_cast<int16_t>(0x38));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), data.role_names[1]);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x1a0, 0x28, 0x1df, 0x76, 7);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x3d), static_cast<int16_t>(0x38));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), data.role_names[2]);
  for (std::uint8_t division{ 0 }; division < GROUPS_PER_SIDE; ++division) {
    opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), division * 0xa0 + 0x10, 0x88, division * 0xa0 + 0x4f, 0xd6, 7);
    opeu_show_division_strength(game, static_cast<uint8_t>(division));
  }
}

// The reinforcement list with its mouse or keyboard help.
void opeu_reinforce_screen(Game& game) {
  auto& data{ Data(game) };
  data.shown_portrait = 0;
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(4));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(0xe8));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.reinforce_title));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(0x170));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), data.input_mode ? Near(data, data.reinforce_mouse_help) : Near(data, data.reinforce_key_help));
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
  opeu_list_dialog(game, Near(data, data.reinforce_dialog), static_cast<uint8_t>(1), static_cast<uint8_t>(3));
  opeu_staff_restore_bottom(game);
  opeu_staff_restore_left(game);
}

// The list of the generals who take the field.
void opeu_play_screen(Game& game) {
  auto& data{ Data(game) };
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(4));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x15), static_cast<int16_t>(0x170));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.play_prompt));
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
  opeu_commander_list_dialog(game, Near(data, data.play_dialog), static_cast<uint8_t>(7), static_cast<uint8_t>(1));
  opeu_staff_restore_bottom(game);
}

// For each side: backs up its records, registers every listed general and, for a human side, repeats the screens
// until the player accepts the generals.
void opeu_confirm_generals(Game& game) {
  auto& data{ Data(game) };
  for (data.current_side = 0; data.current_side < 2; ++data.current_side) {
    std::copy_n(SidePeople(data), PEOPLE_PER_SIDE, data.people_backup.begin());
    auto* person{ SidePeople(data) };
    for (std::uint8_t slot{ 0 }; slot < PEOPLE_PER_SIDE && person->status != PERSON_VACANT; ++slot, ++person) {
      opeu_staff_reroll_person(game, static_cast<uint8_t>(slot));
      opeu_staff_roll_flag(game, Near(data, *person));
    }
    if (data.side_control[data.current_side] == SIDE_PLAYED) {
      opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(4));
      do {
        opeu_general_screen(game);
        opeu_reinforce_screen(game);
        opeu_play_screen(game);
        opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(4));
        opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x14), static_cast<int16_t>(0x170));
      } while (!static_cast<uint16_t>(opeu_prompt_yes_no(game, Near(data, data.use_generals_prompt))));
      opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
    }
  }
}
