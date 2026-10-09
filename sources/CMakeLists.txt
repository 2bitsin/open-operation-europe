# Project dependencies — the single source of truth. buildutil.cmake's
# Require() runs find_package at configure time, and conanfile.py parses
# the SAME calls to build the conan graph, so the two can never disagree.
# TEST/BENCH deps only materialize when tests/benches actually build;
# CONAN names the conan package when it differs from the cmake name.
Require(GTest VERSION ">=1.17.0" TEST CONAN gtest COMPONENTS gtest gtest_main gmock gmock_main)
Require(benchmark VERSION ">=1.9.0" BENCH CONAN benchmark)

# The platform layer's window, renderer, audio device, input and clock.
# Pinned to the 3.4 series the Emscripten SDK's own port carries, so desktop
# and browser run the same SDL minor; the browser takes the SDK's port.
Require(SDL3 VERSION "3.4.16" CONAN sdl PLATFORM Linux Windows Macos)
Require(SDL3 VERSION ">=3.4" SYSTEM PLATFORM Emscripten)

# stb_image_write: the headless --png frame writer (one header, one call).
Require(stb VERSION "cci.20240531" CONAN stb)

# Every subdirectory holding a CMakeLists.txt becomes a module; a
# directory without one is a group and is scanned recursively.
Scan_subdirectories()
