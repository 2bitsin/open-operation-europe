// The initial data image: MAIN's data objects and module data placed at their addresses, the data modules' fields of
// the data segment written. What lies in the data segment goes to the state the game starts from; the far data goes to
// the arena the glue has reset (glue/core/memory.cpp) until B7 gives it buffers of its own.
#include "data/image.hpp"
#include "common/data-segment.hpp"
#include "common/game-state.hpp"
#include "link/image.hpp"
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
constexpr std::size_t DATA_SEGMENT_LINEAR{ std::size_t{ PORT_DS_SEGMENT } * 16 }; // where the data segment starts in the arena

// A block of the image: its linear address, its bytes and its name for a fatal message.
struct ImageBlock {
  std::size_t                   linear;
  std::span<std::uint8_t const> bytes;
  char const*                   name;
};

auto BlockOf(PortImageObject const& object) -> ImageBlock {
  return {
    std::size_t{ object.seg }
    * 16 + object.off, { static_cast<std::uint8_t const*>(object.object), object.size },
    object.name
  };
}

auto BlockOf(PortImageBytes const& block) -> ImageBlock {
  return {
    std::size_t{ block.seg }
    * 16 + block.off, { block.bytes, block.size },
    block.module
  };
}

auto InDataSegment(ImageBlock const& block) -> bool {
  return block.linear >= DATA_SEGMENT_LINEAR && block.linear - DATA_SEGMENT_LINEAR < opeu::game::SEGMENT_SIZE;
}

// Every block of the image: the far data objects, then the code modules' initialised data.
auto Blocks() -> std::vector<ImageBlock> {
  auto blocks{
    std::span{ port_image_objects, port_image_object_count }
    | std::views::transform([](PortImageObject const& object) { return BlockOf(object); }
    ) | std::ranges::to<std::vector>()
  };
  std::ranges::copy(std::span{ port_module_data, port_module_data_count } | std::views::transform([](PortImageBytes const& block) { return BlockOf(block); }), std::back_inserter(blocks));
  return blocks;
}

// Copies a block into `memory` at `address` (fatal when it passes the end).
auto Place(std::span<std::uint8_t> memory, std::size_t address, ImageBlock const& block) -> void {
  if (address + block.bytes.size() > memory.size()) port_fatal("data block %s outside its memory", block.name);
  std::ranges::copy(block.bytes, memory.subspan(address).begin());
}
} // namespace

auto opeu::game::GameState::Initial() -> GameState {
  GameState state;
  std::ranges::for_each(Blocks() | std::views::filter(InDataSegment), [&state](ImageBlock const& block) { Place(state.Bytes(0, SEGMENT_SIZE), block.linear - DATA_SEGMENT_LINEAR, block); });
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
  std::ranges::for_each(Blocks() | std::views::filter(std::not_fn(InDataSegment)), [](ImageBlock const& block) { Place({ port_arena, PORT_ARENA_SIZE }, block.linear, block); });
}
