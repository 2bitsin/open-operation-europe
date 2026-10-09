#include "assets.hpp"

#include "opeu-opeu/resources.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

auto opeu_asset_find(char const* const dos_name, unsigned char const** const data, std::size_t* const size) -> int {
  if (dos_name == nullptr || data == nullptr || size == nullptr) {
    return 0;
  }
  auto name{ std::string_view{ dos_name } };
  if (auto const last{ name.find_last_of("\\/:") }; last != std::string_view::npos) {
    name.remove_prefix(last + 1);
  }
  auto upper{ std::string{ name } };
  std::ranges::transform(upper, upper.begin(), [](unsigned char const c) { return char(std::toupper(c)); });
  auto const* const found{ opeu::resources::find(upper) };
  if (found == nullptr) {
    return 0;
  }
  *data = found->data;
  *size = found->size;
  return 1;
}
