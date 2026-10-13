// DS:7f6d..7fdf: command prompts and eleven far action pointers.
#include "data/ctrltext.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::ctrltext {

auto InitialControlText(DataSegment& data) -> void {
  SetText(data.control_prompt, "Commander %s,\012your orders, sir");
  SetText(data.control_ready, "The troops are ready and\012waiting, sir.");
  // clang-format off
  data.control_actions = { {
    ControlAction::DELAY,
    ControlAction::ORDERS,
    ControlAction::STOCKS,
    ControlAction::TRANSFER,
    ControlAction::PERSONNEL,
    ControlAction::STAFF,
    ControlAction::ORGANIZE,
    ControlAction::REQUEST,
    ControlAction::AIR_FORCE,
    ControlAction::SPECIAL_FORCES,
    ControlAction::SETTINGS,
  } };
  // clang-format on
}
} // namespace opeu::game::detail::ctrltext
