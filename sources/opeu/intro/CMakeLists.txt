Init_submodule()
Link_dependencies(opeu-game      # MAIN's STRINGS and MEMORY routines, linked into OPEN.EXE unchanged; MAIN's data image the runner loads after it.
                  opeu-glue      # port.hpp: the arena, the trace and the run's options the opening runs with.
                  opeu-services) # The OPEN-only assembly services (open-syslib, open-compos), the FM driver and the event pump.
