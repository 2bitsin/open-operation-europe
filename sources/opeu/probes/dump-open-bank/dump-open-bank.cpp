// Writes the opening's FM song bank (the intro's recovered/data/song-bank.cpp: tracks, voices and events in declaration
// order, which is the bank's layout in the opening's data) to stdout.
#include "dump-open-bank.hpp"

#include "recovered.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <string>

namespace opeu::probes::detail::dump_open_bank {
namespace {

auto Char(std::uint8_t const byte) -> char {
  return static_cast<char>(byte);
}

}

auto DumpOpenBank() -> int {
  namespace song_bank = intro::detail::song_bank;
  auto const word_bytes{ [](auto const word) { return std::array{ static_cast<char>(word & 0xFF), static_cast<char>(word >> 8) }; } }; // least significant byte first
  auto       bank{ std::span{ song_bank::TRACKS } | std::views::join | std::views::transform(word_bytes) | std::views::join | std::ranges::to<std::string>() };
  std::ranges::copy(std::span{ song_bank::VOICES } | std::views::transform(Char), std::back_inserter(bank));
  std::ranges::copy(std::span{ song_bank::EVENTS } | std::views::transform(Char), std::back_inserter(bank));
  std::cout << bank;
  return 0;
}

}
