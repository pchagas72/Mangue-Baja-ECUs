#pragma once

#include <stdbool.h>
#include <pthread.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi_types_generic.h"

typedef struct server_state{
    bool connected_to_internet;
    bool connected_to_mqtt;
} Server_State;

void server_connect_internet(void);
void server_check_connection_internet(Server_State *state, pthread_mutex_t *led_mutex);
