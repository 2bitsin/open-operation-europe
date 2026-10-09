cmake_minimum_required(VERSION 3.28)
# C++ for the modules under sources/; C stays for the recovered game until
# it is moved in (every source lives in sources/<submodule>).
project(opeurptv C CXX)

# the machinery buildutil renders at build time; universal policy
# (compile DB, CTest/GoogleTest, coverage/gc-sections/benchmark
# options) lives there, not here
include(buildutil)

# The modules (buildutil submodules, presence-driven).
add_subdirectory(sources)
