idf_component_register(SRCS "CAN Reader.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES can_management sd_logging)
