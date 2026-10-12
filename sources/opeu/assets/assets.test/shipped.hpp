#pragma once
// The shipped data files (the bytes the application embeds), as a source the decoders read. Tests run in the module
// directory, beside the application module that holds them.

#include "opeu/assets/source.hpp"

#include <string_view>

namespace opeu::assets::test {

[[nodiscard]] inline auto Embedded() -> DirectorySource const& {
  static auto const source{ DirectorySource::Open("../opeu/assets.embed").value() };
  return source;
}

[[nodiscard]] inline auto Shipped(std::string_view const name) -> Bytes {
  return Embedded().File(name).value_or(Bytes{});
}

}
