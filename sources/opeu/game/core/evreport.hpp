// The hourly unit reports (rested veterans, regiments out of fuel) and the second hourly event round.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <array>
#include <cstdint>

// A side's regiments to report, by division * 8 + slot; 0 where none.
using ReportList = std::array<std::uint16_t, 32>;

void opeu_report_present(Game& game);
void opeu_report_units_a(Game& game, ReportList const& list);
void opeu_report_units_b(Game& game, ReportList const& list);
void opeu_report_event(Game& game);
void opeu_turn_hourly_b(Game& game);
