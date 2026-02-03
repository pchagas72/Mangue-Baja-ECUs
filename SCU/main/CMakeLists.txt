idf_component_register(SRCS "SCU.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_controls server_controls sd_logging can_management)
