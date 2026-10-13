// The transfer table's columns of the supply transfer between a regiment and its neighbour (units/xferpnl,
// units/xferstep, units/xferunit).
#pragma once

#include <cstdint>

namespace opeu::game::detail::transfer_service {

// The transfer table's columns (DataSegment::transfer_table[side][resource][column]).
enum TransferColumn : std::uint8_t {
  AMOUNT   = 0,
  CAPACITY = 1,
  TARGET   = 2, // what the computer fills the side to
};
inline constexpr std::uint8_t RESOURCES{ 3 }; // ammunition, fuel, food
} // namespace opeu::game::detail::transfer_service

namespace opeu::game {
using detail::transfer_service::AMOUNT;
using detail::transfer_service::CAPACITY;
using detail::transfer_service::RESOURCES;
using detail::transfer_service::TARGET;
using detail::transfer_service::TransferColumn;
} // namespace opeu::game
