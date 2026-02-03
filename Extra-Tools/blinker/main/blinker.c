#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define RELAY_GPIO  GPIO_NUM_25
#define BUTTON_GPIO GPIO_NUM_0

#define HOLD_TIME_MS 500
#define BLINK_TIME_MS 50

void app_main(void)
{
    // Relay output
    gpio_config_t relay_conf = {
        .pin_bit_mask = 1ULL << RELAY_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&relay_conf);

    // Button input (active LOW)
    gpio_config_t button_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_conf);

    bool farol_on = false;
    bool blinking = false;

    int64_t press_start = 0;
    bool relay_level = 0;

    while (1) {

        bool button_pressed = (gpio_get_level(BUTTON_GPIO) == 0);

        if (button_pressed) {
            if (press_start == 0) {
                press_start = esp_timer_get_time(); // µs
            }

            int64_t held_ms =
                (esp_timer_get_time() - press_start) / 1000;

            if (held_ms >= HOLD_TIME_MS) {
                blinking = true;
            }
        } else {
            // Button released
            if (press_start != 0) {
                int64_t press_time =
                    (esp_timer_get_time() - press_start) / 1000;

                if (press_time < HOLD_TIME_MS) {
                    // Short click → toggle
                    farol_on = !farol_on;
                }
            }

            press_start = 0;
            blinking = false;
        }

        if (blinking) {
            relay_level = !relay_level;
            gpio_set_level(RELAY_GPIO, relay_level);
            vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_MS));
        } else {
            gpio_set_level(RELAY_GPIO, farol_on);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

