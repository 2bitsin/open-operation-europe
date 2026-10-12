// Data segment DS:7e28..7f21: request labels and commander text.
#include "data/uitext.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::uitext {

auto InitialUiText(DataSegment& data) -> void {
  SetText(data.request_supplies_name, "supplies");
  SetText(data.request_battalions_name, "battalions");
  SetText(data.request_regiments_name, "regiments");
  data.request_kind_names[0] = 0x7e28;
  data.request_kind_names[1] = 0x7e31;
  data.request_kind_names[2] = 0x7e3c;
  data.request_delay_bases   = { { 0x03, 0x08, 0x10 } };
  SetText(data.equipment_delivery_format, "%s %u(+%u)\033S");
  data.request_weights2 = { { 0x64, 0x50, 0x3c, 0x32, 0x64, 0x64 } };
  SetText(data.context_name_format, "\033C6\033M4Army Cmdr: %.26s");
  SetText(data.context_stats_format,
          "\033B1\033C7Leader  \033C6%2d \033C7Inf   \033C6%2d\012\033C7Charm  "
          " \033C6%2d \033C7Arty  \033C6%2d\012\033C7Brvy    \033C6%2d "
          "\033C7Tanks \033C6%2d\012\033C7Health  \033C%c%2d \033C7Exp   "
          "\033C6%2d\033C7\033B0");
  SetText(data.context_blank_format, "\033B1         \033B0");
  data.scenario_contexts = { { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
}
} // namespace opeu::game::detail::uitext
