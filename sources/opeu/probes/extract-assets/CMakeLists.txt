Init_submodule()
Link_dependencies(opeu-assets    # The decoders and the catalogue of every shipped asset.
                  opeu-intro     # The opening's FM song bank (recovered/data/song-bank.hpp) and, through opeu-game, MAIN's (data/fmusic.hpp).
                  opeu-services  # The FM driver whose register stream checks each music table against its bank.
                  stb::stb)      # stb_image_write, the PNG writer.
