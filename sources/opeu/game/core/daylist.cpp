// The equipment types in service, and an objective's level. A type is in service when (year << 4) + month reaches its
// packed date byte; kind 255 matches every category; the paratroop types are never listed.
#include "core/daylist.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"

#include <cstddef>
#include <ranges>
#include <vector>

using namespace opeu::game;

namespace {

constexpr std::uint8_t ANY_KIND{ 255 };
} // namespace

// The indices of the types in service that the nations in mask field, of category kind.
auto opeu_list_due_records(Game const& game, uint8_t mask, uint8_t kind) -> std::vector<std::uint8_t> {
  auto const& data{ Data(game) };
  auto const  today{ (data.year << 4) + data.month };
  auto const  due{ [&](ElementType const& type) { return (type.sides & mask) && (type.category == kind || kind == ANY_KIND) && today >= type.date && !(type.flags & Bits(TypeFlag::AIRBORNE)); } };
  return std::views::iota(std::size_t{ 0 }, data.element_types.size()) | std::views::filter([&](std::size_t index) { return due(data.element_types[index]); })
         | std::views::transform([](std::size_t index) { return static_cast<std::uint8_t>(index); }) | std::ranges::to<std::vector>();
}

uint8_t opeu_objective_level(Game const& game, uint16_t objective) {
  return Record<Objective>(Data(game), objective).flags & Bits(ObjectiveFlag::LEVEL_MASK);
}
