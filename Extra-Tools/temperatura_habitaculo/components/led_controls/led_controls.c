#include "led_controls.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>

void blink_led(int n, int delay, pthread_mutex_t *led_mutex)
{
    pthread_mutex_lock(led_mutex);
    gpio_reset_pin(BLINK_LED);
    gpio_set_direction(BLINK_LED, GPIO_MODE_OUTPUT);

    for (int i = 0; i < n; i++) {
        gpio_set_level(BLINK_LED, 1);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_LED, 0);
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }

    pthread_mutex_unlock(led_mutex);
}

