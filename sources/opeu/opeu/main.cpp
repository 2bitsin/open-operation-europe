// Operation Europe, native: the application. The shipped data files are
// compiled in (assets.embed/) and handed to the services' file layer; the
// entry is then opeu_main (entry.cpp, beside this file), crt0's role
// for the recovered MAIN.
#include "assets.hpp"

#include "opeu/services/asset-table.hpp"

auto opeu_main(int argc, char** argv) -> int;

auto main(int argc, char** argv) -> int {
  opeu_files_use_assets(&opeu_asset_find);
  return opeu_main(argc, argv);
}
