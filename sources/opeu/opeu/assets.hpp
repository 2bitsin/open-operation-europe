// The original data files compiled into the executable (assets.embed/), as
// the lookup the services' file layer takes (services/asset-table.h).
#pragma once

#include <cstddef>

// A DOS file name -> the shipped bytes: the drive and directories are dropped
// and the name upper-cased (DOS names are case-blind); 1 when found, else 0.
auto opeu_asset_find(char const* dos_name, unsigned char const** data, std::size_t* size) -> int;
