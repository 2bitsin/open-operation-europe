// The general assignment screens before play: the side's staff and division layout, the reinforcement and play-side
// list dialogs, and the per-side "Use these generals?" confirmation loop.
#include "units/genconf.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
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
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {

// The current side's general records up to the first vacant one.
[[nodiscard]] auto ListedGenerals(DataSegment& data) {
  return SideGenerals(data, data.current_side) | std::views::take_while([](Person const& person) { return person.status != PERSON_VACANT; });
}
} // namespace

// Lists the side's generals (up to the first empty record), clears their roles and draws the staff and division
// frames.
void opeu_general_screen(Game& game) {
  auto&        data{ Data(game) };
  std::uint8_t listed{ 0 };
  for (auto& person : ListedGenerals(data)) {
    data.general_list[listed++] = Near(data, person);
    opeu_set_record_counter(person, 0);
    person.role &= static_cast<std::uint8_t>(~Bits(PersonRole::ROLE_MASK));
  }
  ++listed;
  data.play_dialog.rows      = listed;
  data.reinforce_dialog.rows = listed;
  opeu_backdrop_fill_color(game, data.current_side == 0 ? 2 : 1);
  opeu_army_position(game, 0x23, 8);
  opeu_text_color(game, SideTextColor(data.current_side));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.side_title_format), data.side_titles[data.current_side] });
  opeu_text_color(game, 7);
  opeu_ui_outline(game, 0x100, 0x28, 0x13f, 0x76, 7);
  opeu_army_position(game, 0x29, 0x38);
  opeu_ui_text(game, data.role_names[0]);
  opeu_ui_outline(game, 0x60, 0x28, 0x9f, 0x76, 7);
  opeu_army_position(game, 0x15, 0x38);
  opeu_ui_text(game, data.role_names[1]);
  opeu_ui_outline(game, 0x1a0, 0x28, 0x1df, 0x76, 7);
  opeu_army_position(game, 0x3d, 0x38);
  opeu_ui_text(game, data.role_names[2]);
  for (auto const division : std::views::iota(std::uint8_t{ 0 }, GROUPS_PER_SIDE)) {
    opeu_ui_outline(game, division * 0xa0 + 0x10, 0x88, division * 0xa0 + 0x4f, 0xd6, 7);
    opeu_show_division_strength(game, division);
  }
}

// The reinforcement list with its mouse or keyboard help.
void opeu_reinforce_screen(Game& game) {
  auto& data{ Data(game) };
  data.shown_portrait = 0;
  opeu_text_paper_set(game, 4);
  opeu_army_position(game, 1, 0xe8);
  opeu_ui_text(game, Near(data, data.reinforce_title));
  opeu_army_position(game, 1, 0x170);
  opeu_ui_text(game, data.input_mode ? Near(data, data.reinforce_mouse_help) : Near(data, data.reinforce_key_help));
  opeu_text_paper_set(game, 0);
  opeu_list_dialog(game, Near(data, data.reinforce_dialog), 1, 3);
  opeu_staff_restore_bottom(game);
  opeu_staff_restore_left(game);
}

// The list of the generals who take the field.
void opeu_play_screen(Game& game) {
  auto& data{ Data(game) };
  opeu_text_paper_set(game, 4);
  opeu_army_position(game, 0x15, 0x170);
  opeu_ui_text(game, Near(data, data.play_prompt));
  opeu_text_paper_set(game, 0);
  opeu_commander_list_dialog(game, Near(data, data.play_dialog), 7, 1);
  opeu_staff_restore_bottom(game);
}

// For each side: backs up its records, registers every listed general and, for a human side, repeats the screens
// until the player accepts the generals.
void opeu_confirm_generals(Game& game) {
  auto& data{ Data(game) };
  for (data.current_side = 0; data.current_side < 2; ++data.current_side) {
    std::ranges::copy(SideGenerals(data, data.current_side), data.people_backup.begin());
    for (auto const [slot, person] : std::views::zip(std::views::iota(std::uint8_t{ 0 }), ListedGenerals(data))) {
      opeu_staff_reroll_person(game, slot);
      opeu_staff_roll_flag(game, Near(data, person));
    }
    if (data.side_control[data.current_side] == SIDE_PLAYED) {
      opeu_text_paper_set(game, 4);
      do {
        opeu_general_screen(game);
        opeu_reinforce_screen(game);
        opeu_play_screen(game);
        opeu_text_paper_set(game, 4);
        opeu_army_position(game, 0x14, 0x170);
      } while (!static_cast<uint16_t>(opeu_prompt_yes_no(game, Near(data, data.use_generals_prompt))));
      opeu_text_paper_set(game, 0);
    }
  }
}
