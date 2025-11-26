#pragma once

#include <pthread.h>

#define BLINK_LED 2

void blink_led(int n, int delay, pthread_mutex_t *led_mutex);
