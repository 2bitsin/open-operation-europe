// A block of the initial data image: the bytes of a data object or of a code module's initialised data, at the
// segment:offset the original program had them.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace opeu::game::detail::image_block {

// A segment:offset address of the original program, as the linear address it names.
class FarAddress {
public:
  constexpr FarAddress(std::uint16_t segment, std::uint16_t offset) noexcept : _linear{ (std::size_t{ segment } * PARAGRAPH) + offset } { }

  [[nodiscard]] constexpr auto Linear() const noexcept -> std::size_t {
    return _linear;
  }

private:
  static constexpr std::size_t PARAGRAPH{ 16 };

  std::size_t _linear;
};

class ImageBlock {
public:
  ImageBlock(FarAddress address, std::span<std::byte const> bytes) noexcept : _address{ address }, _bytes{ bytes } { }

  [[nodiscard]] auto Linear() const noexcept -> std::size_t {
    return _address.Linear();
  }
  [[nodiscard]] auto Bytes() const noexcept -> std::span<std::byte const> {
    return _bytes;
  }

private:
  FarAddress                 _address;
  std::span<std::byte const> _bytes;
};

// A typed data object of the data headers at its address; its bytes become a block.
template <class _Object>
class FarObject {
public:
  constexpr FarObject(FarAddress address, _Object const& object) noexcept : _address{ address }, _object{ object } { }

  [[nodiscard]] auto Block() const -> ImageBlock {
    return { _address, std::as_bytes(std::span{ _object }) };
  }

private:
  FarAddress     _address;
  _Object const& _object;
};

// The data modules' far objects (link/gen/data-image.cpp).
[[nodiscard]] auto FarDataBlocks() -> std::vector<ImageBlock>;
// The code modules' initialised data in the data segment (link/gen/module-data.cpp).
[[nodiscard]] auto ModuleDataBlocks() -> std::vector<ImageBlock>;
} // namespace opeu::game::detail::image_block
