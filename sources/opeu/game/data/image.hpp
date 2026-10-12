// The data image: the initialised objects of MAIN's data segment and far data, which the start-up copies into the
// arena at their segment:offset (link/gen/data-image.cpp). Pointers in them are 16-bit DS offsets (near) or
// segment:offset words (far) of that layout.
#pragma once

#include "common/data-segment.hpp"

#include <cstdint>

namespace opeu::game::detail::image {

using NearPointer = std::uint16_t; // a DS offset
using FarCode =
    std::uint32_t; // a routine's segment:offset, segment in the high word
using FarText =
    std::uint32_t; // a far text's segment:offset, segment in the high word
using ElementLink =
    std::uint16_t; // a near pointer to an Element, 0 at the end of a chain

// The near pointer of `target`, an object of the data image at DS offset `offset`.
constexpr auto NearTo(auto const& /*target*/, std::uint16_t offset)
    -> NearPointer {
  return offset;
}

// The far pointer of the routine `function` at segment:offset.
constexpr auto FarCodeTo(auto const& /*function*/, std::uint16_t segment,
                         std::uint16_t offset) -> FarCode {
  return static_cast<FarCode>(segment) << 16 | offset;
}

// The far pointer of the text at segment:offset of a far text segment.
constexpr auto FarTextAt(std::uint16_t segment, std::uint16_t offset)
    -> FarText {
  return static_cast<FarText>(segment) << 16 | offset;
}

// The near pointer of element `index`.
constexpr auto ElementNear(std::uint16_t index) -> ElementLink {
  return static_cast<ElementLink>(ELEMENTS_AT + index * sizeof(Element));
}
} // namespace opeu::game::detail::image

namespace opeu::game {
using detail::image::ElementLink;
using detail::image::ElementNear;
using detail::image::FarCode;
using detail::image::FarCodeTo;
using detail::image::FarText;
using detail::image::FarTextAt;
using detail::image::NearPointer;
using detail::image::NearTo;
} // namespace opeu::game
