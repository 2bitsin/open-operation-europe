Init_submodule()
Link_dependencies(opeu-platform  # The renderer's owners (a window, or a surface for headless frames) and SDL's handle owners.
                  opeu-assets    # The asset pack: images by id, index maps, palettes, animations and fonts.
                  SDL3::SDL3)    # SDL_Renderer: textures, blits, fills, lines, blend modes, render targets, readback.
