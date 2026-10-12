// census: raii — main's argument vector is the operating system's; it becomes string views at once.
// Diagnostic: extract-assets <assets dir> <out dir> writes every shipped asset as PNG under <out dir>/images and
// prints one JSON record per line (tools/assets/extract.py turns them into manifests).
#include "extractor.hpp"

#include "opeu/assets/source.hpp"

#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace opeu::probes::extract_assets {

auto Run(std::span<std::string_view const> const args) -> int {
  if (args.size() != 3) {
    std::println(stderr, "usage: extract-assets <assets dir> <out dir>");
    return 2;
  }
  auto const source{ assets::DirectorySource::Open(args[1]) };
  if (!source) {
    std::println(stderr, "{}", source.error());
    return 1;
  }
  auto const extractor{
    Extractor{ *source, args[2], [](std::string_view const record) { std::println("{}", record); } }
  };
  if (auto const done{ extractor.Run() }; !done) {
    std::println(stderr, "{}", done.error());
    return 1;
  }
  return 0;
}

}

auto main(int const argc, char const* const* const argv) -> int {
  auto const args{
    std::span{ argv, static_cast<std::size_t>(argc) }
    | std::views::transform([](char const* const arg) { return std::string_view{ arg }; }
    )
    | std::ranges::to<std::vector>()
  };
  return opeu::probes::extract_assets::Run(args);
}
