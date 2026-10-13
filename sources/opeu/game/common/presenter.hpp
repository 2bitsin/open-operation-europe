// The presenters a daily show (core/dialogs opeu_daily_show) runs after its picture.
#pragma once

#include <cstdint>

namespace opeu::game::detail::presenter {

// What a daily show runs after its picture (opeu_daily_show; core/dialogs). Its values are the presenters' original far
// addresses, segment << 16 | offset, like the code slots' enums (common/data-segment.hpp) until M4 renumbers them.
enum class Presenter : std::uint32_t {
  DAILY_EVENT       = 0x0e360000,
  MISSION_FINISH    = 0x0e8e0042,
  MISSION_DELAY     = 0x0e8e004a,
  AIR_STRIKE        = 0x0f430406,
  PARATROOPERS      = 0x0f430d72,
  CARGO_DROP        = 0x0f431042,
  VISIT             = 0x108c02ac,
  HURRICANE         = 0x108c0396,
  PARTISAN_LOSSES   = 0x108c06ac,
  NORTHERN_LOSSES   = 0x108c0830,
  BOMBING           = 0x108c0aae,
  ANGEL             = 0x108c0ef6,
  SPY_GREETING      = 0x108c10b8,
  SPY_ENDING        = 0x108c1132,
  REPORT            = 0x108c1302,
  DEATH             = 0x11e50004,
  REQUEST_ERROR     = 0x127002e4,
  REQUEST_START     = 0x12700430,
  REQUEST_FALLBACK  = 0x127004f0,
  ARRIVAL           = 0x12700816,
  BLAST_PAUSE       = 0x17c40866,
  TRANSFER          = 0x1d4201ca,
  PERSONNEL_FINISH  = 0x1d820004,
  ALLOCATION_FINISH = 0x20b31f46,
  REQUEST_FINISH    = 0x22f9009a,
  SPECIAL_FORCES    = 0x24d50000,
  ATTACK            = 0x313100f6,
  OCCUPIED          = 0x31310282,
  BRIDGE_DESTROYED  = 0x3131060c,
  MINES_LAID        = 0x319e00a0,
  MINES_REMOVED     = 0x319e016a,
  BRIDGE_BUILT      = 0x319e02b8,
  ARMS_REPAIRED     = 0x319e03ba,
  TRANSFER_STEP     = 0x31f405a8,
  MINES_HIT         = 0x32e605a0,
  NOTICE_VIEW       = 0x33ad05fe,
  END_OCCUPY        = 0x34240082,
};

} // namespace opeu::game::detail::presenter

namespace opeu::game {
using detail::presenter::Presenter;
} // namespace opeu::game
