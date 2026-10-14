#include "json.hpp"

#include <algorithm>
#include <iterator>

namespace opeu::probes::extract_assets::detail::json {

auto Quoted(std::string_view const text) -> std::string {
  auto out{ std::string{ "\"" } };
  std::ranges::for_each(text, [&out](char const c) {
    switch (c) {
    case '"':  out += "\\\""; break;
    case '\\': out += "\\\\"; break;
    case '\n': out += "\\n"; break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        std::format_to(std::back_inserter(out), "\\u{:04x}", static_cast<unsigned char>(c));
      } else {
        out += c;
      }
    }
  });
  return out + "\"";
}

auto Object::Raw(std::string_view const name, std::string_view const encoded) -> Object& {
  if (!_fields.empty()) _fields += ',';
  _fields += Quoted(name) + ":" + std::string{ encoded };
  return *this;
}

}
