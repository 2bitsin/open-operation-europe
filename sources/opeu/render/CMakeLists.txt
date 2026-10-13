Init_submodule()
Link_dependencies(opeu-platform  # The renderer's owners (a window, or a surface for headless frames) and SDL's handle owners.
                  opeu-assets    # The asset pack: images by id, index maps, palettes, animations and fonts; the decoders and the content index.
                  opeu-services  # machine-display.hpp, the interface RenderDisplay implements, and the cell-buffer routines it shares.
                  SDL3::SDL3)    # SDL_Renderer: textures, blits, fills, lines, blend modes, render targets, readback.
