Init_submodule()
Link_dependencies(opeu-glue      # port.hpp: the arena, the run's options, the trace and the fatal exit every service reports through.
                  opeu-platform  # The frame, the input events, the clock and the audio device behind the services and the pump.
                  opeu-ymfm)     # The YM3812 the FM driver's register writes play on.
