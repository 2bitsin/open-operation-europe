Init_submodule()
Link_dependencies(opeu-game      # The recovered MAIN the entry starts (opeu_application_entry, svc_app) and its data image.
                  opeu-intro     # OPEN.EXE, which the launcher ran before MAIN.
                  opeu-render    # RenderDisplay, the game's screen on SDL_Renderer (game-screen.hpp).
                  opeu-assets    # The game's data (GameData), the pack the screen draws from (the embedded classic pack or CataloguePack) and the content index.
                  opeu-services  # The file layer the game's data files are registered with (asset-table.hpp), the FM driver, ShownFrames (the pump presents through the screen).
                  opeu-platform  # The window, input, clock and audio device the entry opens; the screen's software renderer.
                  opeu-glue)     # port.hpp: the arena, the run's options and the trace the entry sets up.
