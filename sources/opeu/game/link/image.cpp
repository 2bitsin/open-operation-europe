// The initial data image: MAIN's data objects and module data placed at their addresses, the data modules' fields of
// the data segment written. What lies in the data segment goes to the state the game starts from; the far data goes to
// the arena the glue has reset (glue/core/memory.cpp) until B7 gives it buffers of its own.
#include "data/image.hpp"
#include "common/data-segment.hpp"
#include "common/game-state.hpp"
#include "link/image-block.hpp"
#include "link/image.hpp"
#include "opeu/contract.hpp"
#include "port.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

namespace {
using opeu::Expects;
using opeu::game::detail::image_block::ImageBlock;

constexpr std::size_t DATA_SEGMENT_LINEAR{ std::size_t{ PORT_DS_SEGMENT } * 16 }; // where the data segment starts in the arena

auto InDataSegment(ImageBlock const& block) -> bool {
  return block.Linear() >= DATA_SEGMENT_LINEAR && block.Linear() - DATA_SEGMENT_LINEAR < opeu::game::SEGMENT_SIZE;
}

// Every block of the image: the far data objects, then the code modules' initialised data.
auto Blocks() -> std::vector<ImageBlock> {
  auto blocks{ opeu::game::detail::image_block::FarDataBlocks() };
  std::ranges::copy(opeu::game::detail::image_block::ModuleDataBlocks(), std::back_inserter(blocks));
  return blocks;
}

// Copies a block into `memory` at `address`.
auto Place(std::span<std::uint8_t> memory, std::size_t address, ImageBlock const& block) -> void {
  Expects(address + block.Bytes().size() <= memory.size(), "a data block lies inside its memory");
  std::ranges::transform(block.Bytes(), memory.subspan(address).begin(), [](std::byte byte) { return std::to_integer<std::uint8_t>(byte); });
}
} // namespace

auto opeu::game::GameState::Initial() -> GameState {
  GameState state;
  std::ranges::for_each(Blocks() | std::views::filter(InDataSegment), [&state](ImageBlock const& block) { Place(state.Bytes(0, SEGMENT_SIZE), block.Linear() - DATA_SEGMENT_LINEAR, block); });
  InitialDataSegment(state.Data());
  return state;
}

auto opeu::game::detail::image_dump::InitialArenaImage() -> std::vector<std::uint8_t> {
  port_memory_init();
  std::vector<std::uint8_t> image(port_arena, port_arena + PORT_ARENA_SIZE); // NOLINT(*-pointer-arithmetic): the glue's arena
  std::ranges::copy(GameState::Initial().Bytes(0, SEGMENT_SIZE), std::span{ image }.subspan(DATA_SEGMENT_LINEAR).begin());
  return image;
}

void port_memory_init() {
  port_memory_reset();
  std::ranges::for_each(Blocks() | std::views::filter(std::not_fn(InDataSegment)), [](ImageBlock const& block) { Place({ port_arena, PORT_ARENA_SIZE }, block.Linear(), block); });
}
