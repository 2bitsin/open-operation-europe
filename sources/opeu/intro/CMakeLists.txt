Init_submodule()
Link_dependencies(opeu-game      # MAIN's data image (port_memory_init), which the runner loads when OPEN returns.
                  opeu-glue      # port.hpp: the arena, the trace and the run's options the opening runs with.
                  opeu-services) # The OPEN-only assembly services (open-syslib, open-compos), the FM driver and the event pump.
