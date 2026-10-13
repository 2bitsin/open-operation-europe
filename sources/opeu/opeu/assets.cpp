#include "assets.hpp"

#include "opeu-opeu/resources.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace opeu::opeu::detail::assets {

auto AssetFind(std::string_view name) -> std::optional<std::span<std::uint8_t const>> {
  if (auto const last{ name.find_last_of("\\/:") }; last != std::string_view::npos) {
    name.remove_prefix(last + 1);
  }
  auto upper{ std::string{ name } };
  std::ranges::transform(upper, upper.begin(), [](unsigned char const c) -> char { return static_cast<char>(std::toupper(c)); });
  auto const shipped{ resources::all() };
  auto const found{ std::ranges::find(shipped, std::string_view{ upper }, &resources::Resource::name) };
  if (found == shipped.end()) {
    return std::nullopt;
  }
  return std::span<std::uint8_t const>{ found->data, found->size };
}

}
