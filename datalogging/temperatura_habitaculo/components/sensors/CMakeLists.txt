idf_component_register(SRCS "sensors.c"
                       INCLUDE_DIRS "include"
                       REQUIRES log esp-idf-lib__dht sd_logging led_controls driver)
