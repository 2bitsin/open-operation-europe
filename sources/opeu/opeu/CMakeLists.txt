Init_submodule()
Link_dependencies(opeu-game      # The recovered MAIN the entry starts (opeu_application_entry, svc_app) and its data image.
                  opeu-intro     # OPEN.EXE, which the launcher ran before MAIN.
                  opeu-services  # The file layer the embedded data files are registered with (asset-table.hpp), the FM driver.
                  opeu-platform  # The window, input, clock and audio device the entry opens.
                  opeu-glue)     # port.hpp: the arena, the run's options and the trace the entry sets up.
