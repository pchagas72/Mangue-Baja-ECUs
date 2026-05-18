idf_component_register(SRCS "server_controls.c"
                       INCLUDE_DIRS "include"
                       REQUIRES mqtt log esp_wifi nvs_flash protocol_examples_common led_controls)
