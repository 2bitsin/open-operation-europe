// DS:7f6d..7fdf: command prompts and eleven far action pointers.
#include "data/ctrltext.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::ctrltext {

auto InitialControlText(DataSegment& data) -> void {
  SetText(data.control_prompt, "Commander %s,\012your orders, sir");
  SetText(data.control_ready, "The troops are ready and\012waiting, sir.");
  data.control_actions = { { { 0x012c, 0x1389 },
                             { 0x0e86, 0x1c23 },
                             { 0x01c2, 0x1d1e },
                             { 0x01d2, 0x1d42 },
                             { 0x04c8, 0x1d82 },
                             { 0x20fc, 0x1dd2 },
                             { 0x1f4e, 0x20b3 },
                             { 0x0970, 0x22f9 },
                             { 0x10da, 0x23b2 },
                             { 0x0b26, 0x24d5 },
                             { 0x0d72, 0x2597 } } };
}
} // namespace opeu::game::detail::ctrltext
