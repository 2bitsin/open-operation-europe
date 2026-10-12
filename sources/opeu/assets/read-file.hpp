#pragma once
// A whole file read into memory, for the sources that read the shipped files or a pack from disk.

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace opeu::assets::detail::read_file {

// The bytes of the file, or why it cannot be read.
[[nodiscard]] auto ReadFile(std::filesystem::path const& path) -> std::expected<std::vector<std::byte>, std::string>;

}

namespace opeu::assets {
using detail::read_file::ReadFile;
}
