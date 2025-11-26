#pragma once
#include <dht.h>
#include <pthread.h>

// Simplified signature
int read_dht_data(const char *TAG,
        dht_sensor_type_t sensor_type,
        int gpio_pin,
        pthread_mutex_t *led_mutex);
