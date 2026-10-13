// The five-digit numeric entry dialog with a slider bar.
#pragma once

#include "common/events.hpp"

#include <cstdint>

namespace opeu::game::detail::numdlg {
// The values a choice accepts: lo..hi.
struct ValueRange {
  std::uint16_t lo;
  std::uint16_t hi;
};
} // namespace opeu::game::detail::numdlg

namespace opeu::game {
using detail::numdlg::ValueRange;
} // namespace opeu::game

// Runs the dialog until a value in the range is accepted or (when `cancel` allows it) the dialog is cancelled; returns
// the value or 0xffff.
auto opeu_numdlg_run(Game& game, opeu::game::ValueRange range, int cancel) -> std::uint16_t;
