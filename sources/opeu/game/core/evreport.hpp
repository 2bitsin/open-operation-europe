// The hourly unit reports (rested veterans, regiments out of fuel) and the second hourly event round.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

#include <array>
#include <cstdint>

// A side's regiments to report, by division * 8 + slot; 0 where none.
using ReportList = std::array<std::uint16_t, 32>;

void opeu_report_present(uint8_t* ds, HostEvent event);
void opeu_report_units_a(uint8_t* ds, HostEvent event, ReportList const& list);
void opeu_report_units_b(uint8_t* ds, HostEvent event, ReportList const& list);
void opeu_report_event(uint8_t* ds, HostEvent event);
void opeu_turn_hourly_b(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evreport(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
