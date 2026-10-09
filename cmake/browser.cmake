# The browser build (`./buildutil build --compiler emscripten`, Linux hosts):
# the Emscripten SDK's SDL3 port and the application's link options. After
# scooby-engine's cmake/browser.cmake (SDK SDL3 port, shell page, memory
# growth). Why here and not in a configure.py: configure.py can link
# libraries and give sources flags, but cannot set an executable's link
# options (-s settings, --pre-js, --shell-file), its LINK_DEPENDS, or the
# find-module path FindSDL3 needs. buildutil itself gives the application
# its .html suffix and installs opeu.{html,js,wasm}; the data files are
# #embed-ed into the wasm, so there is no preload file.
if(NOT EMSCRIPTEN)
  return()
endif()

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")

# Builds the SDK's SDL3 port into its cache, headers and SDL_version.h too.
execute_process(COMMAND "${CMAKE_CXX_COMPILER}" --use-port=sdl3
  --cflags RESULT_VARIABLE SDL_PORT_RESULT OUTPUT_QUIET)
if(NOT SDL_PORT_RESULT EQUAL 0)
  message(FATAL_ERROR "Cannot prepare the Emscripten SDL3 port")
endif()

function(opeu_browser)
  # The game polls in loops that never return to a main loop; ASYNCIFY lets
  # plat_present() and plat_sleep_ms() yield to the browser. Every routine
  # reaches the pump through function pointers, so the whole module is
  # instrumented. The unwound stack of the deepest wait (turn processing ->
  # battle -> dialog -> key wait) fits 1 MiB with a wide margin, the native
  # stack of the same chains 4 MiB. FS and the run dependencies serve
  # web/pre.js (the IDBFS mount of /saves, export and import); NODEFS lets a
  # headless run under node read its script and write its output.
  target_link_options(opeu-opeu PRIVATE
    -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=1048576
    -sENVIRONMENT=web,node
    -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sSTACK_SIZE=4194304
    -sEXIT_RUNTIME=1 -sFORCE_FILESYSTEM=1
    -sEXPORTED_RUNTIME_METHODS=FS,addRunDependency,removeRunDependency
    -lidbfs.js -lnodefs.js
    "SHELL:--pre-js \"${CMAKE_SOURCE_DIR}/web/pre.js\""
    "SHELL:--shell-file \"${CMAKE_SOURCE_DIR}/web/shell.html\"")
  set_property(TARGET opeu-opeu APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_SOURCE_DIR}/web/pre.js" "${CMAKE_SOURCE_DIR}/web/shell.html")
endfunction()

cmake_language(DEFER CALL opeu_browser)
