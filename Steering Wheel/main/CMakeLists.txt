idf_component_register(SRCS "firmware-volante.c"
                    INCLUDE_DIRS "."
                    REQUIRES can_management ssd1309_interface sd_logging)
