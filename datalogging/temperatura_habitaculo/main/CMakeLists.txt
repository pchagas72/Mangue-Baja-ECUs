idf_component_register(SRCS "temperatura_habitaculo.c"
                    INCLUDE_DIRS "."
                    REQUIRES sd_logging sensors led_controls)
