// Data segment DS:8070..838d (798 bytes). Direction keys, turn/order report formats and unit status text.
// DS:838e..8395 belongs to SELECT.
#include "data/dgturn.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::dgturn {

auto InitialTurnText(DataSegment& data) -> void {
  SetText(data.direction_keys, "12346789");
  SetText(data.turn_label_format, "\033C0\033M4%s Regiment %d %s executing orders.\033C7\033M0");
  SetText(data.percent_text, "    %");
  SetText(data.not_available_text, " N/A");
  SetText(data.text_80b3, "%");
  SetText(data.regiment_position_text, "Here is the regiment's present position, sir.\033S");
  SetText(data.division_position_text, "Here's my division's present position, sir.\033S");
  data.order_glyphs       = { 0xff, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x07, 0xff, 0x08 };
  data.view_clear_pattern = { 0x00, 0x88, 0x00, 0x00, 0x88, 0x00, 0x00, 0x88, 0x00, 0x00, 0x22, 0x00, 0x00, 0x22, 0x00, 0x00, 0x22 };
  data.panel_rows[0]      = 0x0018;
  data.panel_rows[1]      = 0x007a;
  data.panel_rows[2]      = 0x011a;
  data.panel_columns[0]   = 0x0040;
  data.panel_columns[1]   = 0x0040;
  data.panel_columns[2]   = 0x004e;
  SetText(data.hurricane_name, "Hurricane");
  SetText(data.turn_date_format, "\033M4   %d.%d.4%d %02d:00 \033C6%3s");
  SetText(data.turn_panel_text, "\033R\"");
  SetText(data.context_panel_text, "\033RM");
  SetText(data.info_panel_text, "\033RM");
  SetText(data.title_panel_text, "\033RM");
  SetText(data.message_panel_text, "\033RV");
  SetText(data.title_format, "\033C6\033M4%s");
  SetText(data.speaker_text, "\033M4\033C6");
  SetText(data.division_title, "Div %d Cmdr");
  SetText(data.commander_title, "Commander");
  SetText(data.speaker_name_format, ": %s");
  SetText(data.speaker_body_text, "\033M0\033C7\033B1");
  SetText(data.speaker_end_text, "\033C7\033B0");
  SetText(data.menu_prompt_format, "? ");
  SetText(data.confirm_space, " ");
  SetText(data.confirm_ok_text, "Is this OK?");
  SetText(data.delay_hours_format, "We'll wait %d hours.\012Is this correct, sir?");
  SetText(data.unit_report_format, "Report from\012\033C%c%s %s:\033C7\012");
  SetText(data.objective_report_format, "Report from\012\033C%c%s\033C7\012");
  SetText(data.attack_format, "\033RN\033B1\033C7AP/AT\033C6%3d\033C7/\033C6%2d");
  SetText(data.defense_format, "\033C7Dfns\033C6%6d");
  SetText(data.speed_format, "\033C7Speed\033C6%5d");
  SetText(data.move_range_format, "\033C7M.Rng\033C6%5d");
  SetText(data.gun_range_format, "\033C7G.Rng\033C6%5d\033C7\033B0");
  SetText(data.group_element_format, "\033RP\033B1  \033C7Number OK\033C6%4d     \033C7Morale  "
                                     "\033C6%3d\012\033C7Damaged  \033C6%4d     \033C7Training\033C6%3d\012   "
                                     "               \033C7Fatigue \033C6%3d\033C7\033B0");
  SetText(data.element_format, "\033RP\033B1\033C7Number OK\033C6%4d  "
                               "\033C7Damaged\033C6%4d\033C7\033B0");
  SetText(data.cursor_position_format, "\033C0\033M4%3d-%3d : ");
  SetText(data.cursor_unit_format, "  %s %s ");
  SetText(data.cursor_end, "\033C7 \033M0");
  SetText(data.pick_direction_keys, "12346789");
  data.general_wounded = { 0x77, 0x6f, 0x75, 0x6e, 0x64, 0x65, 0x64 };
  SetText(data.general_killed, "killed");
  SetText(data.general_casualty_format, "General %s's\012been %s!");
}
} // namespace opeu::game::detail::dgturn
